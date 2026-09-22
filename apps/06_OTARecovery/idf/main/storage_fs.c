#include "storage_fs.h"

#include "esp_log.h"
#include "esp_spiffs.h"

#define TAG "APP07_FS"

esp_err_t storage_fs_init(void)
{
    const esp_vfs_spiffs_conf_t conf = {
        .base_path = STORAGE_FS_BASE,
        .partition_label = "storage",
        .max_files = 10,
        .format_if_mount_failed = true,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return err;
    }

    size_t total = 0;
    size_t used = 0;
    err = esp_spiffs_info("storage", &total, &used);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "FILESYSTEM mounted at %s total=%u used=%u",
             STORAGE_FS_BASE, (unsigned)total, (unsigned)used);
    return ESP_OK;
}

esp_err_t storage_fs_info(size_t *total_bytes, size_t *used_bytes)
{
    if (!total_bytes || !used_bytes) return ESP_ERR_INVALID_ARG;
    return esp_spiffs_info("storage", total_bytes, used_bytes);
}
