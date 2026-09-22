#pragma once

#include <stdint.h>

#include "esp_err.h"

typedef enum {
    NETWORK_STATE_BOOT = 0,
    NETWORK_STATE_STA_CONNECTING,
    NETWORK_STATE_STA_ONLINE,
    NETWORK_STATE_AP_SETUP,
} network_state_t;

esp_err_t network_manager_init(void);
esp_err_t network_manager_begin(void);
esp_err_t network_manager_connect(const char *ssid, const char *password);
esp_err_t network_manager_enter_ap_setup(void);
esp_err_t network_manager_disconnect_sta(void);
esp_err_t network_manager_scan_json(char **json_out);

network_state_t network_manager_state(void);
const char *network_manager_state_name(void);
const char *network_manager_sta_ip(void);
const char *network_manager_ap_ssid(void);
