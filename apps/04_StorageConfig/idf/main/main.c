/*
 * Project: KONTAKTS / ESP32-8048S043 Lab
 * Application: App 04 - Persistent Storage / Partial Resource Loading
 * Programmer: Sol
 * Engineer: Alex Malachevsky
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"

#define TAG "APP04"
#define NVS_NAMESPACE "platform"
#define STORAGE_BASE "/storage"
#define STREAM_CHUNK_SIZE 128

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires erase/re-init: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t load_and_update_settings(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    uint32_t boot_count = 0;
    err = nvs_get_u32(nvs, "boot_count", &boot_count);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        boot_count = 0;
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    boot_count++;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u32(nvs, "boot_count", boot_count));

    uint8_t brightness = 80;
    err = nvs_get_u8(nvs, "brightness", &brightness);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        brightness = 80;
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs, "brightness", brightness));
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    char device_name[32] = {0};
    size_t name_len = sizeof(device_name);
    err = nvs_get_str(nvs, "device_name", device_name, &name_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        strlcpy(device_name, "KONTAKTS-8048S043", sizeof(device_name));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(nvs, "device_name", device_name));
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "SETTINGS boot_count=%" PRIu32, boot_count);
    ESP_LOGI(TAG, "SETTINGS device_name=%s", device_name);
    ESP_LOGI(TAG, "SETTINGS brightness=%u", (unsigned)brightness);
    return ESP_OK;
}

static esp_err_t mount_storage(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = STORAGE_BASE,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        return err;
    }

    size_t total = 0;
    size_t used = 0;
    err = esp_spiffs_info("storage", &total, &used);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "FILESYSTEM mounted at %s", STORAGE_BASE);
    ESP_LOGI(TAG, "FILESYSTEM total=%u used=%u", (unsigned)total, (unsigned)used);
    return ESP_OK;
}

static uint32_t fnv1a_update(uint32_t hash, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static esp_err_t stream_resource(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "RESOURCE open failed: %s", path);
        return ESP_FAIL;
    }

    uint8_t chunk[STREAM_CHUNK_SIZE];
    size_t total_bytes = 0;
    size_t chunk_count = 0;
    uint32_t hash = 2166136261u;

    while (true) {
        size_t n = fread(chunk, 1, sizeof(chunk), f);
        if (n > 0) {
            hash = fnv1a_update(hash, chunk, n);
            total_bytes += n;
            chunk_count++;
        }

        if (n < sizeof(chunk)) {
            if (ferror(f)) {
                ESP_LOGE(TAG, "RESOURCE read error: %s", path);
                fclose(f);
                return ESP_FAIL;
            }
            break;
        }
    }

    fclose(f);

    ESP_LOGI(TAG,
             "RESOURCE path=%s bytes=%u chunks=%u fnv1a=0x%08" PRIx32,
             path,
             (unsigned)total_bytes,
             (unsigned)chunk_count,
             hash);
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "KONTAKTS App04 storage experiment start");

    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(load_and_update_settings());
    ESP_ERROR_CHECK(mount_storage());

    ESP_ERROR_CHECK(stream_resource(STORAGE_BASE "/platform.cfg"));
    ESP_ERROR_CHECK(stream_resource(STORAGE_BASE "/ui-settings-screen.cfg"));

    ESP_LOGI(TAG, "APP04:STORAGE:PASS-CANDIDATE");
    ESP_LOGI(TAG, "Reboot the board to verify persistent boot_count increment");
}
