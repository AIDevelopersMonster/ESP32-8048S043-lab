#include "ota_manager.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_app_format.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"

#define TAG "APP06_OTA"

#define OTA_BOARD_ID "esp32-8048s043-lab-n16r8"
#define OTA_APP_ID "app06-ota-recovery"
#define OTA_PROJECT_NAME "app06_ota_recovery"
#define OTA_CHANNEL "stable"
#define OTA_MANIFEST_URL "https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json"
#define OTA_FIRMWARE_PREFIX "https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/"
#define OTA_MANIFEST_MAX_BYTES 8192
#define OTA_TASK_STACK_BYTES 16384
#define OTA_HTTP_TIMEOUT_MS 30000

typedef struct {
    char version[32];
    char firmware_url[384];
    char sha256[65];
    size_t size;
} ota_manifest_t;

typedef enum {
    OTA_ACTION_NONE = 0,
    OTA_ACTION_CHECK,
    OTA_ACTION_INSTALL,
    OTA_ACTION_ROLLBACK,
    OTA_ACTION_RECOVERY,
} ota_action_t;

typedef struct {
    char *buffer;
    size_t len;
    size_t cap;
    esp_err_t error;
} manifest_http_ctx_t;

typedef struct {
    esp_ota_handle_t handle;
    size_t received;
    esp_err_t error;
    mbedtls_sha256_context sha;
} firmware_http_ctx_t;

static SemaphoreHandle_t s_lock;
static ota_status_t s_status;
static ota_action_t s_action;
static TaskHandle_t s_task;

static const char *image_state_name(esp_ota_img_states_t state)
{
    switch (state) {
        case ESP_OTA_IMG_NEW: return "NEW";
        case ESP_OTA_IMG_PENDING_VERIFY: return "PENDING_VERIFY";
        case ESP_OTA_IMG_VALID: return "VALID";
        case ESP_OTA_IMG_INVALID: return "INVALID";
        case ESP_OTA_IMG_ABORTED: return "ABORTED";
        case ESP_OTA_IMG_UNDEFINED: return "UNDEFINED";
        default: return "UNKNOWN";
    }
}

static void status_lock(void)
{
    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
    }
}

static void status_unlock(void)
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

static void set_message_locked(const char *state, const char *message)
{
    strlcpy(s_status.state, state, sizeof(s_status.state));
    strlcpy(s_status.message, message ? message : "", sizeof(s_status.message));
}

static void set_message(const char *state, const char *message)
{
    status_lock();
    set_message_locked(state, message);
    status_unlock();
}

static void set_progress(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    status_lock();
    s_status.progress_percent = percent;
    status_unlock();
}

static void refresh_running_status_locked(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        strlcpy(s_status.running_partition, "unknown", sizeof(s_status.running_partition));
        strlcpy(s_status.current_version, "unknown", sizeof(s_status.current_version));
        strlcpy(s_status.image_state, "UNKNOWN", sizeof(s_status.image_state));
        s_status.pending_verify = false;
        return;
    }

    strlcpy(s_status.running_partition, running->label, sizeof(s_status.running_partition));

    esp_app_desc_t desc = {0};
    if (esp_ota_get_partition_description(running, &desc) == ESP_OK) {
        strlcpy(s_status.current_version, desc.version, sizeof(s_status.current_version));
    } else {
        strlcpy(s_status.current_version, "unknown", sizeof(s_status.current_version));
    }

    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    esp_err_t err = esp_ota_get_state_partition(running, &state);
    if (err == ESP_OK) {
        strlcpy(s_status.image_state, image_state_name(state), sizeof(s_status.image_state));
        s_status.pending_verify = (state == ESP_OTA_IMG_PENDING_VERIFY);
    } else if (running->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        strlcpy(s_status.image_state, "FACTORY", sizeof(s_status.image_state));
        s_status.pending_verify = false;
    } else {
        strlcpy(s_status.image_state, "UNDEFINED", sizeof(s_status.image_state));
        s_status.pending_verify = false;
    }
}

static bool parse_version(const char *version, int *major, int *minor, int *patch)
{
    if (!version || !major || !minor || !patch) return false;
    int consumed = 0;
    if (sscanf(version, "%d.%d.%d%n", major, minor, patch, &consumed) != 3) return false;
    if (version[consumed] != '\0') return false;
    return *major >= 0 && *minor >= 0 && *patch >= 0;
}

static int compare_versions(const char *a, const char *b)
{
    int amaj = 0, amin = 0, apat = 0;
    int bmaj = 0, bmin = 0, bpat = 0;
    if (!parse_version(a, &amaj, &amin, &apat) || !parse_version(b, &bmaj, &bmin, &bpat)) {
        return strcmp(a ? a : "", b ? b : "");
    }
    if (amaj != bmaj) return amaj < bmaj ? -1 : 1;
    if (amin != bmin) return amin < bmin ? -1 : 1;
    if (apat != bpat) return apat < bpat ? -1 : 1;
    return 0;
}

static bool is_sha256_hex(const char *value)
{
    if (!value || strlen(value) != 64) return false;
    for (size_t i = 0; i < 64; ++i) {
        if (!isxdigit((unsigned char)value[i])) return false;
    }
    return true;
}

static esp_err_t manifest_http_event(esp_http_client_event_t *evt)
{
    manifest_http_ctx_t *ctx = (manifest_http_ctx_t *)evt->user_data;
    if (!ctx) return ESP_OK;

    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        if (ctx->len + (size_t)evt->data_len >= ctx->cap) {
            ctx->error = ESP_ERR_NO_MEM;
            return ctx->error;
        }
        memcpy(ctx->buffer + ctx->len, evt->data, evt->data_len);
        ctx->len += (size_t)evt->data_len;
        ctx->buffer[ctx->len] = '\0';
    }
    return ESP_OK;
}

static esp_err_t firmware_http_event(esp_http_client_event_t *evt)
{
    firmware_http_ctx_t *ctx = (firmware_http_ctx_t *)evt->user_data;
    if (!ctx) return ESP_OK;

    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        if (ctx->error != ESP_OK) return ctx->error;

        ctx->error = esp_ota_write(ctx->handle, evt->data, evt->data_len);
        if (ctx->error != ESP_OK) return ctx->error;

        if (mbedtls_sha256_update(&ctx->sha, (const unsigned char *)evt->data, (size_t)evt->data_len) != 0) {
            ctx->error = ESP_FAIL;
            return ctx->error;
        }
        ctx->received += (size_t)evt->data_len;
    }
    return ESP_OK;
}

static esp_http_client_handle_t create_https_client(const char *url,
                                                     esp_err_t (*event_handler)(esp_http_client_event_t *),
                                                     void *user_data)
{
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = OTA_HTTP_TIMEOUT_MS,
        .disable_auto_redirect = false,
        .max_redirection_count = 10,
        .event_handler = event_handler,
        .buffer_size = 4096,
        .buffer_size_tx = 1024,
        .user_data = user_data,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };
    return esp_http_client_init(&config);
}

static esp_err_t fetch_manifest(ota_manifest_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    manifest_http_ctx_t ctx = {
        .buffer = calloc(1, OTA_MANIFEST_MAX_BYTES + 1),
        .len = 0,
        .cap = OTA_MANIFEST_MAX_BYTES + 1,
        .error = ESP_OK,
    };
    if (!ctx.buffer) return ESP_ERR_NO_MEM;

    esp_http_client_handle_t client = create_https_client(OTA_MANIFEST_URL, manifest_http_event, &ctx);
    if (!client) {
        free(ctx.buffer);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Checking GitHub manifest: %s", OTA_MANIFEST_URL);
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && ctx.error != ESP_OK) err = ctx.error;
    if (err == ESP_OK && status != 200) {
        ESP_LOGW(TAG, "Manifest HTTP status=%d", status);
        err = status == 404 ? ESP_ERR_NOT_FOUND : ESP_FAIL;
    }
    if (err != ESP_OK) {
        free(ctx.buffer);
        return err;
    }

    cJSON *root = cJSON_Parse(ctx.buffer);
    free(ctx.buffer);
    if (!root) return ESP_ERR_INVALID_RESPONSE;

    const cJSON *schema = cJSON_GetObjectItemCaseSensitive(root, "schema");
    const cJSON *board = cJSON_GetObjectItemCaseSensitive(root, "board");
    const cJSON *app = cJSON_GetObjectItemCaseSensitive(root, "app");
    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *channel = cJSON_GetObjectItemCaseSensitive(root, "channel");
    const cJSON *size = cJSON_GetObjectItemCaseSensitive(root, "size");
    const cJSON *sha256 = cJSON_GetObjectItemCaseSensitive(root, "sha256");
    const cJSON *firmware = cJSON_GetObjectItemCaseSensitive(root, "firmware");

    bool valid = cJSON_IsNumber(schema) && schema->valueint == 1 &&
                 cJSON_IsString(board) && strcmp(board->valuestring, OTA_BOARD_ID) == 0 &&
                 cJSON_IsString(app) && strcmp(app->valuestring, OTA_APP_ID) == 0 &&
                 cJSON_IsString(version) &&
                 cJSON_IsString(channel) && strcmp(channel->valuestring, OTA_CHANNEL) == 0 &&
                 cJSON_IsNumber(size) && size->valuedouble > 0 &&
                 cJSON_IsString(sha256) && is_sha256_hex(sha256->valuestring) &&
                 cJSON_IsString(firmware) &&
                 strncmp(firmware->valuestring, OTA_FIRMWARE_PREFIX, strlen(OTA_FIRMWARE_PREFIX)) == 0;

    if (valid) {
        int maj = 0, min = 0, pat = 0;
        valid = parse_version(version->valuestring, &maj, &min, &pat);
    }

    if (valid) {
        strlcpy(out->version, version->valuestring, sizeof(out->version));
        strlcpy(out->firmware_url, firmware->valuestring, sizeof(out->firmware_url));
        strlcpy(out->sha256, sha256->valuestring, sizeof(out->sha256));
        for (size_t i = 0; out->sha256[i]; ++i) {
            out->sha256[i] = (char)tolower((unsigned char)out->sha256[i]);
        }
        out->size = (size_t)size->valuedouble;
    }

    cJSON_Delete(root);
    return valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static void hash_to_hex(const unsigned char hash[32], char out[65])
{
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < 32; ++i) {
        out[i * 2] = hex[(hash[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex[hash[i] & 0x0f];
    }
    out[64] = '\0';
}

static esp_err_t check_manifest(bool for_install, ota_manifest_t *manifest)
{
    esp_err_t err = fetch_manifest(manifest);
    if (err != ESP_OK) return err;

    char current[32];
    status_lock();
    strlcpy(current, s_status.current_version, sizeof(current));
    strlcpy(s_status.available_version, manifest->version, sizeof(s_status.available_version));
    status_unlock();

    if (compare_versions(manifest->version, current) <= 0) {
        status_lock();
        s_status.update_available = false;
        s_status.progress_percent = 0;
        set_message_locked("UP_TO_DATE", "GitHub reports no newer stable firmware");
        status_unlock();
        ESP_LOGI(TAG, "GitHub up to date: installed=%s available=%s", current, manifest->version);
        return ESP_OK;
    }

    status_lock();
    s_status.update_available = true;
    s_status.progress_percent = 0;
    set_message_locked(for_install ? "UPDATE_FOUND" : "UPDATE_AVAILABLE",
                       "Newer GitHub firmware is available");
    status_unlock();
    ESP_LOGI(TAG, "Update available: installed=%s available=%s", current, manifest->version);
    return ESP_OK;
}

static esp_err_t install_manifest(const ota_manifest_t *manifest)
{
    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (!target) return ESP_ERR_NOT_FOUND;
    if (manifest->size > target->size) {
        ESP_LOGE(TAG, "Image does not fit target partition: image=%u partition=%u",
                 (unsigned)manifest->size, (unsigned)target->size);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "OTA target=%s offset=0x%lx size=%lu",
             target->label, (unsigned long)target->address, (unsigned long)target->size);

    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(target, manifest->size, &handle);
    if (err != ESP_OK) return err;

    firmware_http_ctx_t ctx = {
        .handle = handle,
        .received = 0,
        .error = ESP_OK,
    };
    mbedtls_sha256_init(&ctx.sha);
    if (mbedtls_sha256_starts(&ctx.sha, 0) != 0) {
        mbedtls_sha256_free(&ctx.sha);
        esp_ota_abort(handle);
        return ESP_FAIL;
    }

    status_lock();
    set_message_locked("DOWNLOADING", "Downloading GitHub Release asset over verified HTTPS");
    s_status.progress_percent = 1;
    status_unlock();

    esp_http_client_handle_t client = create_https_client(manifest->firmware_url, firmware_http_event, &ctx);
    if (!client) {
        mbedtls_sha256_free(&ctx.sha);
        esp_ota_abort(handle);
        return ESP_ERR_NO_MEM;
    }

    err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && ctx.error != ESP_OK) err = ctx.error;
    if (err == ESP_OK && status != 200) err = ESP_FAIL;
    if (err == ESP_OK && ctx.received != manifest->size) {
        ESP_LOGE(TAG, "Byte count mismatch: received=%u expected=%u",
                 (unsigned)ctx.received, (unsigned)manifest->size);
        err = ESP_ERR_INVALID_SIZE;
    }

    unsigned char digest[32] = {0};
    if (err == ESP_OK && mbedtls_sha256_finish(&ctx.sha, digest) != 0) {
        err = ESP_FAIL;
    }
    mbedtls_sha256_free(&ctx.sha);

    if (err == ESP_OK) {
        char digest_hex[65];
        hash_to_hex(digest, digest_hex);
        if (strcmp(digest_hex, manifest->sha256) != 0) {
            ESP_LOGE(TAG, "SHA-256 mismatch");
            err = ESP_ERR_INVALID_CRC;
        } else {
            ESP_LOGI(TAG, "SHA-256 PASS: %s", digest_hex);
        }
    }

    if (err != ESP_OK) {
        esp_ota_abort(handle);
        return err;
    }

    err = esp_ota_end(handle);
    if (err != ESP_OK) return err;

    esp_app_desc_t desc = {0};
    err = esp_ota_get_partition_description(target, &desc);
    if (err != ESP_OK) return err;

    if (strcmp(desc.project_name, OTA_PROJECT_NAME) != 0 ||
        strcmp(desc.version, manifest->version) != 0) {
        ESP_LOGE(TAG, "Candidate descriptor mismatch: project=%s version=%s",
                 desc.project_name, desc.version);
        return ESP_ERR_INVALID_RESPONSE;
    }

    status_lock();
    set_message_locked("VERIFIED", "Size, SHA-256, project and version checks passed");
    s_status.progress_percent = 100;
    status_unlock();

    err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "OTA verified; next boot partition=%s version=%s", target->label, desc.version);
    set_message("REBOOTING", "Verified candidate selected; rebooting into PENDING_VERIFY");
    vTaskDelay(pdMS_TO_TICKS(1200));
    esp_restart();
    return ESP_OK;
}

static void finish_action(esp_err_t err, const char *operation)
{
    status_lock();
    s_status.busy = false;
    s_action = OTA_ACTION_NONE;
    s_task = NULL;
    if (err != ESP_OK) {
        char message[160];
        snprintf(message, sizeof(message), "%s failed: %s", operation, esp_err_to_name(err));
        set_message_locked("ERROR", message);
    }
    status_unlock();
}

static void ota_task(void *arg)
{
    (void)arg;

    ota_action_t action;
    status_lock();
    action = s_action;
    status_unlock();

    esp_err_t err = ESP_OK;

    if (action == OTA_ACTION_CHECK || action == OTA_ACTION_INSTALL) {
        set_message("CHECKING", "Checking stable GitHub Release manifest");
        set_progress(0);
        ota_manifest_t manifest;
        err = check_manifest(action == OTA_ACTION_INSTALL, &manifest);
        if (err == ESP_OK && action == OTA_ACTION_INSTALL) {
            char current[32];
            status_lock();
            strlcpy(current, s_status.current_version, sizeof(current));
            bool newer = compare_versions(manifest.version, current) > 0;
            status_unlock();
            if (newer) err = install_manifest(&manifest);
        }
        finish_action(err, action == OTA_ACTION_CHECK ? "GitHub check" : "OTA install");
        vTaskDelete(NULL);
        return;
    }

    if (action == OTA_ACTION_ROLLBACK) {
        set_message("ROLLBACK", "Candidate marked invalid; rebooting to previous OTA image");
        vTaskDelay(pdMS_TO_TICKS(800));
        err = esp_ota_mark_app_invalid_rollback_and_reboot();
        finish_action(err, "Rollback");
        vTaskDelete(NULL);
        return;
    }

    if (action == OTA_ACTION_RECOVERY) {
        const esp_partition_t *factory = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY,
            NULL);
        if (!factory) {
            err = ESP_ERR_NOT_FOUND;
        } else {
            err = esp_ota_set_boot_partition(factory);
        }
        if (err == ESP_OK) {
            set_message("RECOVERY", "Factory recovery image selected; rebooting");
            ESP_LOGW(TAG, "Recovery boot selected: %s", factory->label);
            vTaskDelay(pdMS_TO_TICKS(800));
            esp_restart();
        }
        finish_action(err, "Recovery");
        vTaskDelete(NULL);
        return;
    }

    finish_action(ESP_ERR_INVALID_STATE, "OTA action");
    vTaskDelete(NULL);
}

static esp_err_t start_action(ota_action_t action)
{
    status_lock();
    if (s_status.busy) {
        status_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    if (action == OTA_ACTION_ROLLBACK && !s_status.pending_verify) {
        status_unlock();
        return ESP_ERR_INVALID_STATE;
    }

    s_status.busy = true;
    s_action = action;
    status_unlock();

    BaseType_t ok = xTaskCreate(ota_task, "app06_ota", OTA_TASK_STACK_BYTES, NULL, 5, &s_task);
    if (ok != pdPASS) {
        status_lock();
        s_status.busy = false;
        s_action = OTA_ACTION_NONE;
        s_task = NULL;
        status_unlock();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ota_manager_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    memset(&s_status, 0, sizeof(s_status));
    status_lock();
    refresh_running_status_locked();
    set_message_locked("IDLE", "Ready to check GitHub Releases");
    status_unlock();

    ESP_LOGI(TAG, "Manifest URL: %s", OTA_MANIFEST_URL);
    ESP_LOGI(TAG, "Running partition=%s version=%s state=%s",
             s_status.running_partition,
             s_status.current_version,
             s_status.image_state);
    if (s_status.pending_verify) {
        ESP_LOGW(TAG, "OTA candidate is PENDING_VERIFY: confirm it or rollback before reset");
    }
    return ESP_OK;
}

void ota_manager_get_status(ota_status_t *out)
{
    if (!out) return;
    status_lock();
    *out = s_status;
    status_unlock();
}

esp_err_t ota_manager_start_check(void)
{
    return start_action(OTA_ACTION_CHECK);
}

esp_err_t ota_manager_start_install(void)
{
    return start_action(OTA_ACTION_INSTALL);
}

esp_err_t ota_manager_confirm_running(void)
{
    status_lock();
    bool pending = s_status.pending_verify;
    bool busy = s_status.busy;
    status_unlock();
    if (busy || !pending) return ESP_ERR_INVALID_STATE;

    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK) return err;

    status_lock();
    refresh_running_status_locked();
    set_message_locked("CONFIRMED", "Running OTA image marked VALID");
    status_unlock();
    ESP_LOGI(TAG, "Running OTA candidate confirmed VALID");
    return ESP_OK;
}

esp_err_t ota_manager_start_rollback(void)
{
    return start_action(OTA_ACTION_ROLLBACK);
}

esp_err_t ota_manager_start_recovery(void)
{
    return start_action(OTA_ACTION_RECOVERY);
}

const char *ota_manager_manifest_url(void)
{
    return OTA_MANIFEST_URL;
}
