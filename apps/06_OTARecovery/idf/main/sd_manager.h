#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#define SD_MANAGER_BASE_PATH "/sd"
#define SD_MANAGER_WIDGET_ROOT SD_MANAGER_BASE_PATH "/widgets"
#define SD_MANAGER_UPDATE_ROOT SD_MANAGER_BASE_PATH "/UPDATE"

typedef struct {
    bool mounted;
    char state[32];
    char message[128];
    char card_name[24];
    uint64_t capacity_bytes;
    uint32_t frequency_khz;
} sd_manager_status_t;

esp_err_t sd_manager_init(void);
esp_err_t sd_manager_mount(void);
esp_err_t sd_manager_unmount(void);
bool sd_manager_is_mounted(void);
void sd_manager_get_status(sd_manager_status_t *out);

esp_err_t sd_manager_packages_json(char **out_json);
esp_err_t sd_manager_run_widget(const char *relative_path, char *reason, size_t reason_len);
