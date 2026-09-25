#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool online;
    int16_t temperature_tenths_c;
    uint16_t humidity_tenths_rh;
    uint32_t tx_frames;
    uint32_t rx_frames;
    uint32_t crc_errors;
    uint32_t timeouts;
    uint32_t protocol_errors;
} modbus_service_status_t;

/*
 * KONTAKTS field bus allocation for Sample A:
 *   UART1 TX = GPIO17
 *   UART1 RX = GPIO18
 *   9600 8N1 by default
 *
 * The external TTL<->RS485 adapter used by the lab has automatic direction
 * control, so no DE/RE GPIO is consumed.
 */
esp_err_t modbus_service_init(void);

/* Generic Modbus RTU register read primitive used by device providers. */
esp_err_t modbus_service_read_registers(uint8_t slave,
                                        uint8_t function,
                                        uint16_t start_register,
                                        uint16_t register_count,
                                        uint16_t *out_registers,
                                        size_t out_count);

void modbus_service_get_status(modbus_service_status_t *out);
void modbus_service_format_binding(const char *binding, char *out, size_t out_len);
