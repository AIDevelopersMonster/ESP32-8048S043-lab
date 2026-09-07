#include "widget_runtime.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "storage_fs.h"

#define TAG "APP07_WIDGET"
#define WIDGET_PATH STORAGE_FS_BASE "/widget.json"
#define WIDGET_TEMP_PATH STORAGE_FS_BASE "/widget.tmp"
#define WIDGET_BACKUP_PATH STORAGE_FS_BASE "/widget.bak"

static SemaphoreHandle_t s_lock;
static widget_model_t *s_model;
static widget_info_t s_info;

static void *psram_alloc(size_t size)
{
    void *p = heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : calloc(1, size);
}

static void set_reason(char *reason, size_t len, const char *text)
{
    if (reason && len) strlcpy(reason, text ? text : "", len);
}

static bool valid_short_string(const cJSON *item, size_t max_len)
{
    return cJSON_IsString(item) && item->valuestring && item->valuestring[0] &&
           strlen(item->valuestring) <= max_len;
}

static bool parse_hex_color(const cJSON *item, uint32_t fallback, uint32_t *out)
{
    if (!out) return false;
    if (!item) {
        *out = fallback;
        return true;
    }
    if (!cJSON_IsString(item) || !item->valuestring) return false;
    const char *s = item->valuestring;
    if (strlen(s) != 7 || s[0] != '#') return false;
    for (size_t i = 1; i < 7; ++i) {
        if (!isxdigit((unsigned char)s[i])) return false;
    }
    *out = (uint32_t)strtoul(s + 1, NULL, 16);
    return true;
}

static int json_int(const cJSON *object, const char *key, int fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsNumber(item) ? item->valueint : fallback;
}

static const char *json_string(const cJSON *object, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    return (cJSON_IsString(item) && item->valuestring) ? item->valuestring : "";
}

static bool binding_allowed(const char *binding)
{
    if (!binding || !binding[0]) return true;
    static const char *allowed[] = {
        "system.uptime", "system.heap", "system.psram", "wifi.ip",
        "wifi.rssi", "firmware.version", "ota.state",
    };
    for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i) {
        if (strcmp(binding, allowed[i]) == 0) return true;
    }
    return false;
}

static bool button_action_allowed(const char *action)
{
    return action && (strcmp(action, "show_status") == 0 || strcmp(action, "show_ota") == 0);
}

static bool parse_widget(const char *json, size_t len, widget_model_t *out,
                         char *reason, size_t reason_len)
{
    if (!json || !out || len == 0 || len > WIDGET_MAX_JSON_BYTES) {
        set_reason(reason, reason_len, "Widget must be 1..32768 bytes");
        return false;
    }

    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) {
        set_reason(reason, reason_len, "JSON parse failed");
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->background = 0x101820;
    bool ok = true;

    const cJSON *schema = cJSON_GetObjectItemCaseSensitive(root, "schema");
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *background = cJSON_GetObjectItemCaseSensitive(root, "background");
    const cJSON *objects = cJSON_GetObjectItemCaseSensitive(root, "objects");

    if (!cJSON_IsNumber(schema) || schema->valueint != 1) {
        set_reason(reason, reason_len, "schema must equal 1"); ok = false;
    } else if (!valid_short_string(id, 48)) {
        set_reason(reason, reason_len, "id missing/too long"); ok = false;
    } else if (!valid_short_string(name, 64)) {
        set_reason(reason, reason_len, "name missing/too long"); ok = false;
    } else if (!valid_short_string(version, 24)) {
        set_reason(reason, reason_len, "version missing/too long"); ok = false;
    } else if (!parse_hex_color(background, 0x101820, &out->background)) {
        set_reason(reason, reason_len, "background must be #RRGGBB"); ok = false;
    } else if (!cJSON_IsArray(objects) || cJSON_GetArraySize(objects) < 1 ||
               cJSON_GetArraySize(objects) > WIDGET_MAX_OBJECTS) {
        set_reason(reason, reason_len, "objects must contain 1..24 items"); ok = false;
    }

    if (ok) {
        strlcpy(out->id, id->valuestring, sizeof(out->id));
        strlcpy(out->name, name->valuestring, sizeof(out->name));
        strlcpy(out->version, version->valuestring, sizeof(out->version));
        out->object_count = (size_t)cJSON_GetArraySize(objects);

        for (size_t i = 0; i < out->object_count && ok; ++i) {
            const cJSON *object = cJSON_GetArrayItem(objects, (int)i);
            const cJSON *type = cJSON_GetObjectItemCaseSensitive(object, "type");
            if (!cJSON_IsObject(object) || !valid_short_string(type, 16)) {
                set_reason(reason, reason_len, "object type missing"); ok = false; break;
            }

            widget_object_t *dst = &out->objects[i];
            dst->x = json_int(object, "x", 16);
            dst->y = json_int(object, "y", 16);
            dst->w = json_int(object, "w", 300);
            dst->h = json_int(object, "h", 36);
            dst->value = json_int(object, "value", 0);

            if (dst->x < 0 || dst->x > 730 || dst->y < 0 || dst->y > 350 ||
                dst->w < 20 || dst->w > 740 || dst->h < 18 || dst->h > 350 ||
                dst->x + dst->w > 744 || dst->y + dst->h > 365) {
                set_reason(reason, reason_len, "object geometry out of range"); ok = false; break;
            }

            if (!parse_hex_color(cJSON_GetObjectItemCaseSensitive(object, "color"), 0xFFFFFF, &dst->color)) {
                set_reason(reason, reason_len, "object color must be #RRGGBB"); ok = false; break;
            }

            if (strcmp(type->valuestring, "label") == 0) {
                dst->type = WIDGET_OBJECT_LABEL;
                const char *text = json_string(object, "text");
                const char *binding = json_string(object, "bind");
                const char *prefix = json_string(object, "prefix");
                const char *suffix = json_string(object, "suffix");
                if ((!text[0] && !binding[0]) || strlen(text) > 160 || strlen(binding) > 32 ||
                    strlen(prefix) > 64 || strlen(suffix) > 64 || !binding_allowed(binding)) {
                    set_reason(reason, reason_len, "label text/binding invalid"); ok = false; break;
                }
                strlcpy(dst->text, text, sizeof(dst->text));
                strlcpy(dst->binding, binding, sizeof(dst->binding));
                strlcpy(dst->prefix, prefix, sizeof(dst->prefix));
                strlcpy(dst->suffix, suffix, sizeof(dst->suffix));
            } else if (strcmp(type->valuestring, "bar") == 0) {
                dst->type = WIDGET_OBJECT_BAR;
                if (dst->value < 0 || dst->value > 100) {
                    set_reason(reason, reason_len, "bar value must be 0..100"); ok = false; break;
                }
            } else if (strcmp(type->valuestring, "button") == 0) {
                dst->type = WIDGET_OBJECT_BUTTON;
                const char *text = json_string(object, "text");
                const char *action = json_string(object, "action");
                if (!text[0] || strlen(text) > 64 || !button_action_allowed(action)) {
                    set_reason(reason, reason_len, "button supports show_status/show_ota only"); ok = false; break;
                }
                strlcpy(dst->text, text, sizeof(dst->text));
                strlcpy(dst->action, action, sizeof(dst->action));
            } else {
                set_reason(reason, reason_len, "unsupported object type"); ok = false; break;
            }
        }
    }

    cJSON_Delete(root);
    out->valid = ok;
    if (ok) set_reason(reason, reason_len, "OK");
    return ok;
}

static esp_err_t read_file(char **json_out, size_t *len_out)
{
    if (!json_out || !len_out) return ESP_ERR_INVALID_ARG;
    *json_out = NULL; *len_out = 0;
    struct stat st;
    if (stat(WIDGET_PATH, &st) != 0) return ESP_ERR_NOT_FOUND;
    if (st.st_size <= 0 || st.st_size > WIDGET_MAX_JSON_BYTES) return ESP_ERR_INVALID_SIZE;

    FILE *f = fopen(WIDGET_PATH, "rb");
    if (!f) return ESP_FAIL;
    char *json = psram_alloc((size_t)st.st_size + 1);
    if (!json) { fclose(f); return ESP_ERR_NO_MEM; }
    size_t got = fread(json, 1, (size_t)st.st_size, f);
    fclose(f);
    if (got != (size_t)st.st_size) { free(json); return ESP_FAIL; }
    json[got] = '\0';
    *json_out = json; *len_out = got;
    return ESP_OK;
}

static esp_err_t commit_file(const char *json, size_t len)
{
    FILE *f = fopen(WIDGET_TEMP_PATH, "wb");
    if (!f) return ESP_FAIL;
    size_t written = fwrite(json, 1, len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (written != len) { remove(WIDGET_TEMP_PATH); return ESP_FAIL; }

    remove(WIDGET_BACKUP_PATH);
    if (rename(WIDGET_PATH, WIDGET_BACKUP_PATH) != 0 && errno != ENOENT) {
        remove(WIDGET_TEMP_PATH); return ESP_FAIL;
    }
    if (rename(WIDGET_TEMP_PATH, WIDGET_PATH) != 0) {
        rename(WIDGET_BACKUP_PATH, WIDGET_PATH); return ESP_FAIL;
    }
    remove(WIDGET_BACKUP_PATH);
    return ESP_OK;
}

static void publish_model(const widget_model_t *model, size_t file_size, const char *status)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *s_model = *model;
    s_info.installed = model->valid;
    s_info.generation++;
    s_info.file_size = file_size;
    strlcpy(s_info.id, model->id, sizeof(s_info.id));
    strlcpy(s_info.name, model->name, sizeof(s_info.name));
    strlcpy(s_info.version, model->version, sizeof(s_info.version));
    strlcpy(s_info.status, status ? status : "OK", sizeof(s_info.status));
    xSemaphoreGive(s_lock);
}

esp_err_t widget_runtime_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    s_model = psram_alloc(sizeof(*s_model));
    if (!s_model) return ESP_ERR_NO_MEM;
    memset(&s_info, 0, sizeof(s_info));
    strlcpy(s_info.status, "No external widget installed", sizeof(s_info.status));

    char *json = NULL; size_t len = 0;
    esp_err_t err = read_file(&json, &len);
    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "No persisted widget; firmware shell remains active");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Persisted widget read failed: %s", esp_err_to_name(err));
        return ESP_OK;
    }

    widget_model_t *model = psram_alloc(sizeof(*model));
    if (!model) { free(json); return ESP_ERR_NO_MEM; }
    char reason[96];
    bool ok = parse_widget(json, len, model, reason, sizeof(reason));
    free(json);
    if (!ok) {
        ESP_LOGW(TAG, "Persisted widget rejected: %s; system shell remains available", reason);
        free(model); return ESP_OK;
    }
    publish_model(model, len, "AUTOLOAD PASS");
    ESP_LOGI(TAG, "WIDGET AUTOLOAD PASS id=%s name=%s version=%s bytes=%u",
             model->id, model->name, model->version, (unsigned)len);
    free(model);
    return ESP_OK;
}

esp_err_t widget_runtime_install_json(const char *json, size_t len, char *reason, size_t reason_len)
{
    widget_model_t *model = psram_alloc(sizeof(*model));
    if (!model) { set_reason(reason, reason_len, "No memory for widget model"); return ESP_ERR_NO_MEM; }
    if (!parse_widget(json, len, model, reason, reason_len)) { free(model); return ESP_ERR_INVALID_ARG; }

    esp_err_t err = commit_file(json, len);
    if (err != ESP_OK) {
        set_reason(reason, reason_len, "Filesystem commit failed"); free(model); return err;
    }
    publish_model(model, len, "INSTALL PASS");
    ESP_LOGI(TAG, "WIDGET INSTALL PASS id=%s name=%s version=%s bytes=%u generation=%u",
             model->id, model->name, model->version, (unsigned)len, (unsigned)widget_runtime_generation());
    free(model);
    set_reason(reason, reason_len, "INSTALL PASS");
    return ESP_OK;
}

esp_err_t widget_runtime_delete(void)
{
    remove(WIDGET_TEMP_PATH); remove(WIDGET_BACKUP_PATH);
    if (remove(WIDGET_PATH) != 0 && errno != ENOENT) return ESP_FAIL;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memset(s_model, 0, sizeof(*s_model));
    s_info.installed = false; s_info.generation++; s_info.file_size = 0;
    s_info.id[0] = s_info.name[0] = s_info.version[0] = '\0';
    strlcpy(s_info.status, "No external widget installed", sizeof(s_info.status));
    xSemaphoreGive(s_lock);
    ESP_LOGI(TAG, "Widget deleted; firmware shell remains available");
    return ESP_OK;
}

void widget_runtime_get_info(widget_info_t *out)
{
    if (!out || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY); *out = s_info; xSemaphoreGive(s_lock);
}

void widget_runtime_get_model(widget_model_t *out)
{
    if (!out || !s_lock || !s_model) return;
    xSemaphoreTake(s_lock, portMAX_DELAY); *out = *s_model; xSemaphoreGive(s_lock);
}

uint32_t widget_runtime_generation(void)
{
    if (!s_lock) return 0;
    xSemaphoreTake(s_lock, portMAX_DELAY); uint32_t g = s_info.generation; xSemaphoreGive(s_lock); return g;
}
