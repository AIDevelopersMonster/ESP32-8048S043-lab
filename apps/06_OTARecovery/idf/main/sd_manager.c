#include "sd_manager.h"

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

static sdmmc_card_t *s_card;
static sdmmc_host_t s_host;
static bool s_bus_initialized;
static sd_manager_status_t s_status;

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

esp_err_t sd_manager_init(void)
{
    memset(&s_status, 0, sizeof(s_status));
    s_status.frequency_khz = SD_FREQ_KHZ;
    set_state("NOT_MOUNTED", "SD is optional; platform remains usable without card");
    return sd_manager_mount();
}

esp_err_t sd_manager_mount(void)
{
    if (s_status.mounted) return ESP_OK;

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
    return ESP_OK;
}

esp_err_t sd_manager_unmount(void)
{
    if (!s_status.mounted) return ESP_OK;
    esp_err_t err = esp_vfs_fat_sdcard_unmount(SD_MANAGER_BASE_PATH, s_card);
    s_card = NULL;
    s_status.mounted = false;
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

esp_err_t sd_manager_packages_json(char **out_json)
{
    if (!out_json) return ESP_ERR_INVALID_ARG;
    *out_json = NULL;
    if (!s_status.mounted) return ESP_ERR_INVALID_STATE;

    cJSON *root = cJSON_CreateObject();
    cJSON *packages = cJSON_AddArrayToObject(root, "packages");
    if (!root || !packages) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    DIR *dir = opendir(SD_MANAGER_WIDGET_ROOT);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            char package_path[256];
            snprintf(package_path, sizeof(package_path), "%s/%s/package.json", SD_MANAGER_WIDGET_ROOT, entry->d_name);
            struct stat st;
            if (stat(package_path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
            cJSON *item = cJSON_CreateObject();
            if (!item) continue;
            cJSON_AddStringToObject(item, "id", entry->d_name);
            cJSON_AddStringToObject(item, "manifest", package_path + strlen(SD_MANAGER_BASE_PATH) + 1);
            cJSON_AddItemToArray(packages, item);
        }
        closedir(dir);
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
    snprintf(path, sizeof(path), "%s/%s", SD_MANAGER_BASE_PATH, relative_path);
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
