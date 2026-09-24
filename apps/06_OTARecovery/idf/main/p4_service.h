#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

typedef enum {
    P4_MODE_IDLE = 0,
    P4_MODE_UART1,
    P4_MODE_GPIO_OUT,
} p4_mode_t;

typedef enum {
    P4_ENDING_NONE = 0,
    P4_ENDING_LF,
    P4_ENDING_CR,
    P4_ENDING_CRLF,
} p4_ending_t;

typedef enum {
    P4_TX_ASCII = 0,
    P4_TX_HEX,
} p4_tx_mode_t;

/*
 * Fixed board contract for ESP32-8048S043 Sample A:
 *   P4 GND / 3.3V / GPIO17 / GPIO18
 *
 * GPIO17/18 are intentionally treated as one small flex port, not as a
 * universal pin allocator. Supported modes are UART1 (TX=17, RX=18) and
 * two digital outputs. GPIO19/20 remain the system I2C bus used by GT911.
 */
esp_err_t p4_service_init(void);
esp_err_t p4_service_idle(void);

esp_err_t p4_service_uart_enable(int baud);
esp_err_t p4_service_uart_send_text(const char *text);
void p4_service_set_ending(p4_ending_t ending);
void p4_service_set_tx_mode(p4_tx_mode_t mode);
esp_err_t p4_service_clear(void);

esp_err_t p4_service_gpio_enable(void);
esp_err_t p4_service_gpio_set(int gpio_num, bool level);

void p4_service_format_binding(const char *binding, char *out, size_t out_len);
