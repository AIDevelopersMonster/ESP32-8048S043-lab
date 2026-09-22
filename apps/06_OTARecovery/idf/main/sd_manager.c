#include "sd_manager.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "widget_runtime.h"

#define TAG "APP09_SD"
#define SD_PIN_CS 10
#define SD_PIN_MOSI 11
#define SD_PIN_CLK 12
#define SD_PIN_MISO 13
#define SD_FREQ_KHZ 10000
#define SD_PACKAGE_NAME_MAX 63
#define SD_MANIFEST_MAX 8192

static sdmmc_card_t *s_card;
static sdmmc_host_t s_host;
static bool s_bus_initialized;
static sd_manager_status_t s_status;
static sd_manager_entry_t s_entries[SD_MANAGER_MAX_ENTRIES];
static size_t s_entry_count;

static void set_state(const char *state, const char *message)
{
    strlcpy(s_status.state, state ? state : "", sizeof(s_status.state));
    strlcpy(s_status.message, message ? message : "", sizeof(s_status.message));
}

static bool safe_relative_path(const char *path)
{
    if (!path || !path[0] || path[0] == '/' || strstr(path, "..")) return false;
    for (const char *p = path; *p; ++p) {
        if (*p == '\\') return false;
    }
    return true;
}

static bool safe_package_name(const char *name)
{
    if (!name || !name[0]) return false;
    size_t len = strnlen(name, SD_PACKAGE_NAME_MAX + 1);
    if (len == 0 || len > SD_PACKAGE_NAME_MAX) return false;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (!(isalnum(*p) || *p == '-' || *p == '_' || *p == '.')) return false;
    }
    return true;
}

static bool safe_manifest_file(const char *name)
{
    if (!name || !name[0] || strlen(name) > SD_PACKAGE_NAME_MAX) return false;
    if (strchr(name, '/') || strchr(name, '\\') || strstr(name, "..")) return false;
    return true;
}

static char *read_small_file(const char *path, size_t max_bytes)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size <= 0 || (size_t)size > max_bytes) { fclose(f); return NULL; }
    rewind(f);
    char *buf = heap_caps_malloc((size_t)size + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) { free(buf); return NULL; }
    buf[got] = '\0';
    return buf;
}

esp_err_t sd_manager_init(void)
{
    memset(&s_status, 0, sizeof(s_status));
    memset(s_entries, 0, sizeof(s_entries));
    s_entry_count = 0;
    s_status.frequency_khz = SD_FREQ_KHZ;
    set_state("NOT_MOUNTED", "SD is optional; use MOUNT / RESCAN after platform UI starts");
    ESP_LOGI(TAG, "SD manager ready; boot-time mount deferred to preserve UI internal RAM");
    return ESP_OK;
}

esp_err_t sd_manager_mount(void)
{
    if (s_status.mounted) return sd_manager_rescan();

    s_host = (sdmmc_host_t)SDSPI_HOST_DEFAULT();
    s_host.max_freq_khz = SD_FREQ_KHZ;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t err = spi_bus_initialize(s_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        set_state("BUS_ERROR", esp_err_to_name(err));
        return err;
    }
    s_bus_initialized = true;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_PIN_CS;
    slot_cfg.host_id = s_host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };

    err = esp_vfs_fat_sdspi_mount(SD_MANAGER_BASE_PATH, &s_host, &slot_cfg, &mount_cfg, &s_card);
    if (err != ESP_OK) {
        if (s_bus_initialized) {
            spi_bus_free(s_host.slot);
            s_bus_initialized = false;
        }
        s_card = NULL;
        s_status.mounted = false;
        set_state("NO_CARD", "No usable SD card; no formatting attempted");
        ESP_LOGW(TAG, "SD mount skipped/failed: %s", esp_err_to_name(err));
        return err;
    }

    s_status.mounted = true;
    strlcpy(s_status.card_name, s_card->cid.name, sizeof(s_status.card_name));
    s_status.capacity_bytes = (uint64_t)s_card->csd.capacity * s_card->csd.sector_size;
    set_state("MOUNTED", "SD mounted read/write; automatic formatting disabled");
    ESP_LOGI(TAG, "SD mounted name=%s capacity=%llu bytes pins CS=%d MOSI=%d CLK=%d MISO=%d freq=%u kHz",
             s_status.card_name,
             (unsigned long long)s_status.capacity_bytes,
             SD_PIN_CS, SD_PIN_MOSI, SD_PIN_CLK, SD_PIN_MISO,
             (unsigned)s_status.frequency_khz);
    return sd_manager_rescan();
}

esp_err_t sd_manager_unmount(void)
{
    if (!s_status.mounted) return ESP_OK;
    esp_err_t err = esp_vfs_fat_sdcard_unmount(SD_MANAGER_BASE_PATH, s_card);
    s_card = NULL;
    s_status.mounted = false;
    s_entry_count = 0;
    memset(s_entries, 0, sizeof(s_entries));
    if (s_bus_initialized) {
        esp_err_t bus_err = spi_bus_free(s_host.slot);
        s_bus_initialized = false;
        if (err == ESP_OK) err = bus_err;
    }
    set_state("NOT_MOUNTED", "SD unmounted");
    return err;
}

bool sd_manager_is_mounted(void)
{
    return s_status.mounted;
}

void sd_manager_get_status(sd_manager_status_t *out)
{
    if (out) *out = s_status;
}

esp_err_t sd_manager_rescan(void)
{
    if (!s_status.mounted) return ESP_ERR_INVALID_STATE;
    s_entry_count = 0;
    memset(s_entries, 0, sizeof(s_entries));

    DIR *dir = opendir(SD_MANAGER_WIDGET_ROOT);
    if (!dir) {
        set_state("MOUNTED", "SD mounted; /widgets directory not found");
        ESP_LOGW(TAG, "SD catalog root missing: %s", SD_MANAGER_WIDGET_ROOT);
        return ESP_OK;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && s_entry_count < SD_MANAGER_MAX_ENTRIES) {
        if (entry->d_name[0] == '.' || !safe_package_name(entry->d_name)) continue;

        char manifest_path[256];
        int written = snprintf(manifest_path, sizeof(manifest_path), "%s/%.63s/package.json",
                               SD_MANAGER_WIDGET_ROOT, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(manifest_path)) continue;

        struct stat st;
        if (stat(manifest_path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        char *text = read_small_file(manifest_path, SD_MANIFEST_MAX);
        if (!text) continue;
        cJSON *root = cJSON_Parse(text);
        free(text);
        if (!root) {
            ESP_LOGW(TAG, "Ignoring invalid package manifest: %s", manifest_path);
            continue;
        }

        const cJSON *jid = cJSON_GetObjectItemCaseSensitive(root, "id");
        const cJSON *jname = cJSON_GetObjectItemCaseSensitive(root, "name");
        const cJSON *jentries = cJSON_GetObjectItemCaseSensitive(root, "entrypoints");
        const char *package_id = cJSON_IsString(jid) && safe_package_name(jid->valuestring) ? jid->valuestring : entry->d_name;
        const char *package_name = cJSON_IsString(jname) && jname->valuestring[0] ? jname->valuestring : package_id;

        if (cJSON_IsArray(jentries)) {
            const cJSON *je;
            cJSON_ArrayForEach(je, jentries) {
                if (s_entry_count >= SD_MANAGER_MAX_ENTRIES) break;
                const cJSON *jeid = cJSON_GetObjectItemCaseSensitive(je, "id");
                const cJSON *jename = cJSON_GetObjectItemCaseSensitive(je, "name");
                const cJSON *jfile = cJSON_GetObjectItemCaseSensitive(je, "file");
                if (!cJSON_IsString(jfile) || !safe_manifest_file(jfile->valuestring)) continue;

                sd_manager_entry_t *dst = &s_entries[s_entry_count];
                strlcpy(dst->package_id, package_id, sizeof(dst->package_id));
                strlcpy(dst->package_name, package_name, sizeof(dst->package_name));
                strlcpy(dst->entry_id,
                        cJSON_IsString(jeid) && jeid->valuestring[0] ? jeid->valuestring : jfile->valuestring,
                        sizeof(dst->entry_id));
                strlcpy(dst->entry_name,
                        cJSON_IsString(jename) && jename->valuestring[0] ? jename->valuestring : dst->entry_id,
                        sizeof(dst->entry_name));
                int n = snprintf(dst->relative_path, sizeof(dst->relative_path),
                                 "widgets/%s/%s", entry->d_name, jfile->valuestring);
                if (n < 0 || (size_t)n >= sizeof(dst->relative_path)) {
                    memset(dst, 0, sizeof(*dst));
                    continue;
                }
                s_entry_count++;
            }
        }
        cJSON_Delete(root);
    }
    closedir(dir);

    char msg[128];
    snprintf(msg, sizeof(msg), "SD application catalog: %u entrypoint(s)", (unsigned)s_entry_count);
    set_state("MOUNTED", msg);
    ESP_LOGI(TAG, "SD catalog scan complete entries=%u", (unsigned)s_entry_count);
    for (size_t i = 0; i < s_entry_count; ++i) {
        ESP_LOGI(TAG, "CATALOG[%u] %s / %s -> %s", (unsigned)i,
                 s_entries[i].package_name, s_entries[i].entry_name, s_entries[i].relative_path);
    }
    return ESP_OK;
}

size_t sd_manager_entry_count(void)
{
    return s_entry_count;
}

esp_err_t sd_manager_entry_get(size_t index, sd_manager_entry_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    if (!s_status.mounted) return ESP_ERR_INVALID_STATE;
    if (index >= s_entry_count) return ESP_ERR_NOT_FOUND;
    *out = s_entries[index];
    return ESP_OK;
}

esp_err_t sd_manager_packages_json(char **out_json)
{
    if (!out_json) return ESP_ERR_INVALID_ARG;
    *out_json = NULL;
    if (!s_status.mounted) return ESP_ERR_INVALID_STATE;

    cJSON *root = cJSON_CreateObject();
    cJSON *entries = root ? cJSON_AddArrayToObject(root, "entries") : NULL;
    if (!root || !entries) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    for (size_t i = 0; i < s_entry_count; ++i) {
        cJSON *item = cJSON_CreateObject();
        if (!item) continue;
        cJSON_AddStringToObject(item, "package", s_entries[i].package_id);
        cJSON_AddStringToObject(item, "package_name", s_entries[i].package_name);
        cJSON_AddStringToObject(item, "entry", s_entries[i].entry_id);
        cJSON_AddStringToObject(item, "name", s_entries[i].entry_name);
        cJSON_AddStringToObject(item, "file", s_entries[i].relative_path);
        cJSON_AddItemToArray(entries, item);
    }
    char *printed = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!printed) return ESP_ERR_NO_MEM;
    *out_json = printed;
    return ESP_OK;
}

esp_err_t sd_manager_run_widget(const char *relative_path, char *reason, size_t reason_len)
{
    if (!s_status.mounted) {
        if (reason && reason_len) strlcpy(reason, "SD not mounted", reason_len);
        return ESP_ERR_INVALID_STATE;
    }
    if (!safe_relative_path(relative_path)) {
        if (reason && reason_len) strlcpy(reason, "Unsafe SD path", reason_len);
        return ESP_ERR_INVALID_ARG;
    }

    char path[320];
    int written = snprintf(path, sizeof(path), "%s/%s", SD_MANAGER_BASE_PATH, relative_path);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        if (reason && reason_len) strlcpy(reason, "SD path too long", reason_len);
        return ESP_ERR_INVALID_SIZE;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (reason && reason_len) strlcpy(reason, "SD widget file not found", reason_len);
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return ESP_FAIL; }
    long size = ftell(f);
    if (size <= 0 || size > WIDGET_MAX_JSON_BYTES) {
        fclose(f);
        if (reason && reason_len) strlcpy(reason, "SD widget must be 1..32768 bytes", reason_len);
        return ESP_ERR_INVALID_SIZE;
    }
    rewind(f);

    char *json = heap_caps_malloc((size_t)size + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!json) json = malloc((size_t)size + 1);
    if (!json) { fclose(f); return ESP_ERR_NO_MEM; }

    size_t got = fread(json, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) { free(json); return ESP_FAIL; }
    json[got] = '\0';

    esp_err_t err = widget_runtime_install_json(json, got, reason, reason_len);
    free(json);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SD widget installed from %s and persisted through Widget Runtime", relative_path);
    }
    return err;
}
