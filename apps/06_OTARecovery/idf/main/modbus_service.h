#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    MODBUS_MA01_MODE_LEVEL = 0,
    MODBUS_MA01_MODE_PULSE = 1,
    MODBUS_MA01_MODE_FOLLOW = 2,
} modbus_ma01_mode_t;

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

/* Generic coil primitives for relay-style Modbus devices. */
esp_err_t modbus_service_read_coils(uint8_t slave,
                                    uint16_t start_coil,
                                    uint16_t coil_count,
                                    bool *out_coils,
                                    size_t out_count);
esp_err_t modbus_service_write_single_coil(uint8_t slave,
                                           uint16_t coil,
                                           bool on);

/*
 * MA01-XXCX0080 provider.
 * Slave address is runtime configuration persisted in NVS, never fixed in firmware.
 */
uint8_t modbus_service_ma01_get_slave(void);
esp_err_t modbus_service_ma01_set_slave(uint8_t slave);
esp_err_t modbus_service_ma01_scan(uint8_t *out_slave, uint16_t *out_model, uint16_t *out_fw);
esp_err_t modbus_service_ma01_refresh(void);
esp_err_t modbus_service_ma01_refresh_config(void);
esp_err_t modbus_service_ma01_set(uint8_t channel, bool on);
esp_err_t modbus_service_ma01_toggle(uint8_t channel);
esp_err_t modbus_service_ma01_action(uint8_t channel);
esp_err_t modbus_service_ma01_set_mode(uint8_t channel, modbus_ma01_mode_t mode);
esp_err_t modbus_service_ma01_cycle_mode(uint8_t channel);
esp_err_t modbus_service_ma01_set_pulse_ms(uint8_t channel, uint16_t pulse_ms);

void modbus_service_get_status(modbus_service_status_t *out);
void modbus_service_format_binding(const char *binding, char *out, size_t out_len);
