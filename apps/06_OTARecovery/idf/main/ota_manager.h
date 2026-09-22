#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    char state[32];
    char current_version[32];
    char available_version[32];
    char running_partition[17];
    char image_state[32];
    char message[160];
    int progress_percent;
    bool busy;
    bool update_available;
    bool pending_verify;
} ota_status_t;

esp_err_t ota_manager_init(void);
void ota_manager_get_status(ota_status_t *out);

esp_err_t ota_manager_start_check(void);
esp_err_t ota_manager_start_install(void);
esp_err_t ota_manager_confirm_running(void);
esp_err_t ota_manager_start_rollback(void);
esp_err_t ota_manager_start_recovery(void);

const char *ota_manager_manifest_url(void);
