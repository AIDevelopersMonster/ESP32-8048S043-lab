#pragma once

#include <stdbool.h>
#include "esp_err.h"

/*
 * BLE transport for the common command/service layer.
 * Phone clients send the same ASCII commands used by UART0.
 *
 * Device name: KONTAKTS-8048
 * Service UUID: 0xFFF0
 * Command characteristic: 0xFFF1 (WRITE)
 * Response characteristic: 0xFFF2 (READ)
 */
esp_err_t ble_service_init(void);
bool ble_service_is_ready(void);
