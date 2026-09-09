#include "youtube_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#include "network_manager.h"
#include "storage_fs.h"

#define TAG "APP08_YOUTUBE"
#define YT_NVS_NAMESPACE "youtube"
#define YT_NVS_CHANNEL "channel"
#define YT_NVS_API_KEY "api_key"
#define YT_HISTORY_PATH STORAGE_FS_BASE "/youtube-history.bin"
#define YT_HTTP_BUF_MAX 12288
#define YT_TASK_STACK 8192
#define YT_REFRESH_INTERVAL_MS (60 * 60 * 1000)
#define YT_RETRY_INTERVAL_MS (30 * 1000)

typedef struct {
    uint32_t epoch_day;
    uint64_t subscribers;
    uint64_t views;
    uint64_t videos;
} youtube_sample_t;

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    esp_err_t error;
} http_ctx_t;

static SemaphoreHandle_t s_lock;
static youtube_status_t s_status;
static youtube_sample_t *s_history;
static size_t s_history_count;
static char s_api_key[96];
static bool s_refresh_requested;
static bool s_started;

static void status_message_locked(const char *state, const char *message)
{
    strlcpy(s_status.state, state ? state : "", sizeof(s_status.state));
    strlcpy(s_status.message, message ? message : "", sizeof(s_status.message));
}

static bool safe_token(const char *s, size_t max_len)
{
    if (!s || !s[0] || strlen(s) > max_len) return false;
    for (size_t i = 0; s[i]; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!(isalnum(c) || c == '_' || c == '-')) return false;
    }
    return true;
}

static void format_count(uint64_t value, char *out, size_t out_len)
{
    if (value >= 1000000000ULL) snprintf(out, out_len, "%.2fB", (double)value / 1000000000.0);
    else if (value >= 1000000ULL) snprintf(out, out_len, "%.2fM", (double)value / 1000000.0);
    else if (value >= 1000ULL) snprintf(out, out_len, "%.1fK", (double)value / 1000.0);
    else snprintf(out, out_len, "%llu", (unsigned long long)value);
}

static void load_config(void)
{
    nvs_handle_t nvs;
    if (nvs_open(YT_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return;
    size_t channel_len = sizeof(s_status.channel_id);
    size_t key_len = sizeof(s_api_key);
    esp_err_t a = nvs_get_str(nvs, YT_NVS_CHANNEL, s_status.channel_id, &channel_len);
    esp_err_t b = nvs_get_str(nvs, YT_NVS_API_KEY, s_api_key, &key_len);
    nvs_close(nvs);
    s_status.configured = a == ESP_OK && b == ESP_OK && s_status.channel_id[0] && s_api_key[0];
}

static void load_history(void)
{
    if (!s_history) return;
    FILE *f = fopen(YT_HISTORY_PATH, "rb");
    if (!f) return;
    size_t n = fread(s_history, sizeof(s_history[0]), YOUTUBE_HISTORY_MAX, f);
    fclose(f);
    s_history_count = n;
    s_status.history_count = n;
}

static void save_history(void)
{
    if (!s_history) return;
    FILE *f = fopen(YT_HISTORY_PATH, "wb");
    if (!f) return;
    fwrite(s_history, sizeof(s_history[0]), s_history_count, f);
    fflush(f);
    fclose(f);
}

static uint32_t current_epoch_day(void)
{
    time_t now = time(NULL);
    if (now < 1700000000) return 0;
    return (uint32_t)((uint64_t)now / 86400ULL);
}

static void record_sample_locked(uint64_t subscribers, uint64_t views, uint64_t videos)
{
    if (!s_history) return;
    uint32_t day = current_epoch_day();
    if (!day) return;
    youtube_sample_t sample = {.epoch_day = day, .subscribers = subscribers, .views = views, .videos = videos};
    if (s_history_count && s_history[s_history_count - 1].epoch_day == day) {
        s_history[s_history_count - 1] = sample;
    } else {
        if (s_history_count == YOUTUBE_HISTORY_MAX) {
            memmove(&s_history[0], &s_history[1], (YOUTUBE_HISTORY_MAX - 1) * sizeof(s_history[0]));
            s_history_count--;
        }
        s_history[s_history_count++] = sample;
    }
    s_status.history_count = s_history_count;
    save_history();
}

static size_t first_index_for_period_locked(youtube_period_t period)
{
    if (!s_history || !s_history_count || period == YOUTUBE_PERIOD_ALL) return 0;
    uint32_t day = current_epoch_day();
    if (!day) return 0;
    uint32_t min_day = day >= (uint32_t)period - 1 ? day - ((uint32_t)period - 1) : 0;
    size_t i = 0;
    while (i + 1 < s_history_count && s_history[i].epoch_day < min_day) i++;
    return i;
}

static void recompute_period_locked(void)
{
    if (!s_status.has_data || !s_history || !s_history_count) {
        s_status.period_views_delta = 0;
        s_status.period_subscribers_delta = 0;
        return;
    }
    size_t first = first_index_for_period_locked(s_status.period);
    const youtube_sample_t *base = &s_history[first];
    s_status.period_views_delta = s_status.views >= base->views ? s_status.views - base->views : 0;
    s_status.period_subscribers_delta = (int64_t)s_status.subscribers - (int64_t)base->subscribers;
}

static esp_err_t http_event(esp_http_client_event_t *evt)
{
    http_ctx_t *ctx = (http_ctx_t *)evt->user_data;
    if (!ctx) return ESP_OK;
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        if (ctx->len + (size_t)evt->data_len >= ctx->cap) {
            ctx->error = ESP_ERR_NO_MEM;
            return ctx->error;
        }
        memcpy(ctx->buf + ctx->len, evt->data, (size_t)evt->data_len);
        ctx->len += (size_t)evt->data_len;
        ctx->buf[ctx->len] = '\0';
    }
    return ESP_OK;
}

static esp_err_t fetch_channel(uint64_t *subscribers, uint64_t *views, uint64_t *videos,
                               char *title, size_t title_len)
{
    char channel[64], key[96];
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(channel, s_status.channel_id, sizeof(channel));
    strlcpy(key, s_api_key, sizeof(key));
    xSemaphoreGive(s_lock);
    if (!channel[0] || !key[0]) return ESP_ERR_INVALID_STATE;

    char url[512];
    snprintf(url, sizeof(url),
             "https://www.googleapis.com/youtube/v3/channels?part=snippet%%2Cstatistics&id=%s&key=%s",
             channel, key);
    char *buf = heap_caps_calloc(1, YT_HTTP_BUF_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = calloc(1, YT_HTTP_BUF_MAX);
    if (!buf) return ESP_ERR_NO_MEM;
    http_ctx_t ctx = {.buf = buf, .cap = YT_HTTP_BUF_MAX, .error = ESP_OK};
    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 20000,
        .event_handler = http_event,
        .user_data = &ctx,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 4096,
        .buffer_size_tx = 1024,
        .keep_alive_enable = false,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) { free(buf); return ESP_ERR_NO_MEM; }
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (err == ESP_OK && ctx.error != ESP_OK) err = ctx.error;
    if (err == ESP_OK && status != 200) err = status == 403 ? ESP_ERR_INVALID_STATE : ESP_FAIL;
    if (err != ESP_OK) { free(buf); return err; }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return ESP_ERR_INVALID_RESPONSE;
    const cJSON *items = cJSON_GetObjectItemCaseSensitive(root, "items");
    const cJSON *item = cJSON_IsArray(items) ? cJSON_GetArrayItem(items, 0) : NULL;
    const cJSON *snippet = item ? cJSON_GetObjectItemCaseSensitive(item, "snippet") : NULL;
    const cJSON *stats = item ? cJSON_GetObjectItemCaseSensitive(item, "statistics") : NULL;
    const cJSON *jtitle = snippet ? cJSON_GetObjectItemCaseSensitive(snippet, "title") : NULL;
    const cJSON *jsubs = stats ? cJSON_GetObjectItemCaseSensitive(stats, "subscriberCount") : NULL;
    const cJSON *jviews = stats ? cJSON_GetObjectItemCaseSensitive(stats, "viewCount") : NULL;
    const cJSON *jvideos = stats ? cJSON_GetObjectItemCaseSensitive(stats, "videoCount") : NULL;
    bool ok = cJSON_IsString(jtitle) && cJSON_IsString(jviews) && cJSON_IsString(jvideos);
    if (ok) {
        strlcpy(title, jtitle->valuestring, title_len);
        *views = strtoull(jviews->valuestring, NULL, 10);
        *videos = strtoull(jvideos->valuestring, NULL, 10);
        *subscribers = cJSON_IsString(jsubs) ? strtoull(jsubs->valuestring, NULL, 10) : 0;
    }
    cJSON_Delete(root);
    return ok ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static void do_refresh(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_status.configured || s_status.busy) { xSemaphoreGive(s_lock); return; }
    s_status.busy = true;
    status_message_locked("FETCHING", "Reading YouTube channel statistics");
    xSemaphoreGive(s_lock);

    uint64_t subs = 0, views = 0, videos = 0;
    char title[96] = {0};
    esp_err_t err = fetch_channel(&subs, &views, &videos, title, sizeof(title));

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status.busy = false;
    if (err == ESP_OK) {
        s_status.subscribers = subs;
        s_status.views = views;
        s_status.videos = videos;
        s_status.has_data = true;
        s_status.last_update = time(NULL);
        strlcpy(s_status.channel_title, title, sizeof(s_status.channel_title));
        record_sample_locked(subs, views, videos);
        recompute_period_locked();
        status_message_locked("READY", "YouTube statistics updated");
        ESP_LOGI(TAG, "YOUTUBE FETCH PASS title=%s subscribers=%llu views=%llu videos=%llu history=%u",
                 title, (unsigned long long)subs, (unsigned long long)views,
                 (unsigned long long)videos, (unsigned)s_history_count);
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "YouTube fetch failed: %s", esp_err_to_name(err));
        status_message_locked("ERROR", msg);
        ESP_LOGW(TAG, "%s", msg);
    }
    xSemaphoreGive(s_lock);
}

static void youtube_task(void *arg)
{
    (void)arg;
    TickType_t last = 0;
    for (;;) {
        bool online = network_manager_state() == NETWORK_STATE_STA_ONLINE;
        bool configured;
        bool requested;
        xSemaphoreTake(s_lock, portMAX_DELAY);
        configured = s_status.configured;
        requested = s_refresh_requested;
        if (requested) s_refresh_requested = false;
        xSemaphoreGive(s_lock);
        TickType_t now = xTaskGetTickCount();
        if (online && configured && (requested || last == 0 || now - last >= pdMS_TO_TICKS(YT_REFRESH_INTERVAL_MS))) {
            do_refresh();
            last = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(online ? 1000 : YT_RETRY_INTERVAL_MS));
    }
}

esp_err_t youtube_service_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    memset(&s_status, 0, sizeof(s_status));
    s_status.period = YOUTUBE_PERIOD_30D;
    status_message_locked("UNCONFIGURED", "Set Channel ID and YouTube Data API key");

    size_t history_bytes = YOUTUBE_HISTORY_MAX * sizeof(*s_history);
    s_history = heap_caps_calloc(YOUTUBE_HISTORY_MAX, sizeof(*s_history), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_history) {
        ESP_LOGI(TAG, "YouTube history buffer allocated in PSRAM bytes=%u", (unsigned)history_bytes);
    } else {
        ESP_LOGW(TAG, "YouTube history PSRAM allocation failed; current statistics remain available without local history");
    }

    load_config();
    load_history();
    if (s_status.configured) status_message_locked("WAITING", "Waiting for network");
    return ESP_OK;
}

esp_err_t youtube_service_start(void)
{
    if (s_started) return ESP_OK;
    BaseType_t ok = xTaskCreate(youtube_task, "youtube_data", YT_TASK_STACK, NULL, 5, NULL);
    if (ok != pdPASS) return ESP_ERR_NO_MEM;
    s_started = true;
    return ESP_OK;
}

esp_err_t youtube_service_save_config(const char *channel_id, const char *api_key)
{
    if (!safe_token(channel_id, 63) || !safe_token(api_key, 95)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(YT_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, YT_NVS_CHANNEL, channel_id);
    if (err == ESP_OK) err = nvs_set_str(nvs, YT_NVS_API_KEY, api_key);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err != ESP_OK) return err;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(s_status.channel_id, channel_id, sizeof(s_status.channel_id));
    strlcpy(s_api_key, api_key, sizeof(s_api_key));
    s_status.configured = true;
    s_refresh_requested = true;
    status_message_locked("WAITING", "Configuration saved; refresh queued");
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t youtube_service_clear_config(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(YT_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    nvs_erase_key(nvs, YT_NVS_CHANNEL);
    nvs_erase_key(nvs, YT_NVS_API_KEY);
    err = nvs_commit(nvs);
    nvs_close(nvs);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status.configured = false;
    s_status.channel_id[0] = '\0';
    s_api_key[0] = '\0';
    status_message_locked("UNCONFIGURED", "Set Channel ID and YouTube Data API key");
    xSemaphoreGive(s_lock);
    return err;
}

esp_err_t youtube_service_request_refresh(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_status.configured) { xSemaphoreGive(s_lock); return ESP_ERR_INVALID_STATE; }
    s_refresh_requested = true;
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

void youtube_service_set_period(youtube_period_t period)
{
    if (!(period == YOUTUBE_PERIOD_7D || period == YOUTUBE_PERIOD_30D ||
          period == YOUTUBE_PERIOD_90D || period == YOUTUBE_PERIOD_ALL)) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status.period = period;
    recompute_period_locked();
    xSemaphoreGive(s_lock);
}

void youtube_service_get_status(youtube_status_t *out)
{
    if (!out || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_status;
    xSemaphoreGive(s_lock);
}

void youtube_service_format_binding(const char *binding, char *out, size_t out_len)
{
    if (!out || !out_len) return;
    youtube_status_t s; youtube_service_get_status(&s);
    if (!binding) { out[0] = '\0'; return; }
    if (strcmp(binding, "youtube.subscribers") == 0) format_count(s.subscribers, out, out_len);
    else if (strcmp(binding, "youtube.views") == 0) format_count(s.views, out, out_len);
    else if (strcmp(binding, "youtube.videos") == 0) format_count(s.videos, out, out_len);
    else if (strcmp(binding, "youtube.views_delta") == 0) format_count(s.period_views_delta, out, out_len);
    else if (strcmp(binding, "youtube.subscribers_delta") == 0) snprintf(out, out_len, "%+lld", (long long)s.period_subscribers_delta);
    else if (strcmp(binding, "youtube.channel") == 0) strlcpy(out, s.channel_title[0] ? s.channel_title : "YouTube", out_len);
    else if (strcmp(binding, "youtube.state") == 0) strlcpy(out, s.state, out_len);
    else if (strcmp(binding, "youtube.period") == 0) {
        if (s.period == YOUTUBE_PERIOD_ALL) strlcpy(out, "ALL", out_len);
        else snprintf(out, out_len, "%dD", (int)s.period);
    } else strlcpy(out, "unsupported", out_len);
}

size_t youtube_service_get_chart(const char *binding, int32_t *values, size_t max_values,
                                 int32_t *min_out, int32_t *max_out)
{
    if (!binding || !values || !max_values || !s_history) return 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_history_count) { xSemaphoreGive(s_lock); return 0; }
    size_t first = first_index_for_period_locked(s_status.period);
    size_t count = s_history_count - first;
    if (count > max_values) { first += count - max_values; count = max_values; }
    uint64_t base_u = strcmp(binding, "youtube.subscribers_history") == 0 ? s_history[first].subscribers : s_history[first].views;
    int32_t minv = 0, maxv = 0;
    for (size_t i = 0; i < count; ++i) {
        uint64_t raw = strcmp(binding, "youtube.subscribers_history") == 0 ? s_history[first + i].subscribers : s_history[first + i].views;
        int64_t delta = (int64_t)raw - (int64_t)base_u;
        if (delta > INT32_MAX) delta = INT32_MAX;
        if (delta < INT32_MIN) delta = INT32_MIN;
        values[i] = (int32_t)delta;
        if (i == 0 || values[i] < minv) minv = values[i];
        if (i == 0 || values[i] > maxv) maxv = values[i];
    }
    if (min_out) *min_out = minv;
    if (max_out) *max_out = maxv;
    xSemaphoreGive(s_lock);
    return count;
}
