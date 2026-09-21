#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t serial_service_init(void);
esp_err_t serial_service_send_test(void);
esp_err_t serial_service_clear(void);
void serial_service_format_binding(const char *binding, char *out, size_t out_len);
