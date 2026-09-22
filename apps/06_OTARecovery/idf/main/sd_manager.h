#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define SD_MANAGER_BASE_PATH "/sd"
#define SD_MANAGER_WIDGET_ROOT SD_MANAGER_BASE_PATH "/widgets"
#define SD_MANAGER_UPDATE_ROOT SD_MANAGER_BASE_PATH "/UPDATE"

#define SD_MANAGER_MAX_ENTRIES 24
#define SD_MANAGER_ENTRY_TEXT_MAX 63
#define SD_MANAGER_ENTRY_PATH_MAX 191

typedef struct {
    bool mounted;
    char state[32];
    char message[128];
    char card_name[24];
    uint64_t capacity_bytes;
    uint32_t frequency_khz;
} sd_manager_status_t;

typedef struct {
    char package_id[SD_MANAGER_ENTRY_TEXT_MAX + 1];
    char package_name[SD_MANAGER_ENTRY_TEXT_MAX + 1];
    char entry_id[SD_MANAGER_ENTRY_TEXT_MAX + 1];
    char entry_name[SD_MANAGER_ENTRY_TEXT_MAX + 1];
    char relative_path[SD_MANAGER_ENTRY_PATH_MAX + 1];
} sd_manager_entry_t;

esp_err_t sd_manager_init(void);
esp_err_t sd_manager_mount(void);
esp_err_t sd_manager_unmount(void);
bool sd_manager_is_mounted(void);
void sd_manager_get_status(sd_manager_status_t *out);

/*
 * Rebuild the application catalog by reading package.json from every direct
 * child directory under /sd/widgets. The platform UI must use this catalog
 * rather than hard-coded application launchers. Adding a compatible
 * application therefore requires only copying its package directory to SD.
 */
esp_err_t sd_manager_rescan(void);
size_t sd_manager_entry_count(void);
esp_err_t sd_manager_entry_get(size_t index, sd_manager_entry_t *out);

/* Compatibility/debug JSON view of the currently scanned catalog. */
esp_err_t sd_manager_packages_json(char **out_json);

esp_err_t sd_manager_run_widget(const char *relative_path, char *reason, size_t reason_len);
