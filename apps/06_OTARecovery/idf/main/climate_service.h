#pragma once
#include <stddef.h>
#include "esp_err.h"
#include "climate_logic.h"

esp_err_t climate_service_init(void);
esp_err_t climate_service_set_config(const climate_config_t *config);
void climate_service_get_config(climate_config_t *out);
/* Index 0..5: T MIN/MAX/HYST, RH MIN/MAX/HYST; delta in tenths. */
esp_err_t climate_service_adjust(unsigned index, int delta);
esp_err_t climate_service_enable(bool enabled);
/* Runs from the existing Modbus polling task, independent of displayed widget. */
void climate_service_poll(void);
void climate_service_format_binding(const char *binding, char *out, size_t size);
esp_err_t climate_service_command(const char *command, char *out, size_t size);
