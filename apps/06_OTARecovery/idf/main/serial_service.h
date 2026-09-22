#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    SERIAL_ENDING_NONE = 0,
    SERIAL_ENDING_LF,
    SERIAL_ENDING_CR,
    SERIAL_ENDING_CRLF,
} serial_ending_t;

typedef enum {
    SERIAL_TX_MODE_ASCII = 0,
    SERIAL_TX_MODE_HEX,
} serial_tx_mode_t;

esp_err_t serial_service_init(void);
esp_err_t serial_service_send_test(void);
esp_err_t serial_service_send_text(const char *text);
esp_err_t serial_service_repeat_last(void);
esp_err_t serial_service_clear(void);

void serial_service_set_ending(serial_ending_t ending);
void serial_service_set_tx_mode(serial_tx_mode_t mode);

esp_err_t serial_service_history_prev(char *out, size_t out_len);
esp_err_t serial_service_history_next(char *out, size_t out_len);

void serial_service_format_binding(const char *binding, char *out, size_t out_len);
