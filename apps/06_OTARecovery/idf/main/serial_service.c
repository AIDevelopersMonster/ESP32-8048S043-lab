#include "serial_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "SERIAL_SERVICE"
#define SERIAL_RX_TEXT_BYTES 512
#define SERIAL_RX_TASK_STACK 4096
#define SERIAL_RX_TASK_PRIORITY 5
#define SERIAL_TX_TEXT_MAX 256
#define SERIAL_HISTORY_DEPTH 8
#define SERIAL_HISTORY_TEXT_MAX 128

static SemaphoreHandle_t s_lock;
static char s_rx_text[SERIAL_RX_TEXT_BYTES];
static size_t s_rx_len;
static uint64_t s_rx_bytes;
static uint64_t s_tx_bytes;
static serial_ending_t s_ending = SERIAL_ENDING_CRLF;
static serial_tx_mode_t s_tx_mode = SERIAL_TX_MODE_ASCII;
static char s_history[SERIAL_HISTORY_DEPTH][SERIAL_HISTORY_TEXT_MAX + 1];
static size_t s_history_count;
static size_t s_history_cursor;
static char s_last_tx[SERIAL_HISTORY_TEXT_MAX + 1];

static const char *ending_name(serial_ending_t ending)
{
    switch (ending) {
        case SERIAL_ENDING_NONE: return "NONE";
        case SERIAL_ENDING_LF: return "LF";
        case SERIAL_ENDING_CR: return "CR";
        default: return "CRLF";
    }
}

static const char *mode_name(serial_tx_mode_t mode)
{
    return mode == SERIAL_TX_MODE_HEX ? "HEX" : "ASCII";
}

static void append_rx_char(char ch)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_rx_bytes++;

    if (ch == '\r') {
        xSemaphoreGive(s_lock);
        return;
    }

    if (s_rx_len + 1 >= sizeof(s_rx_text)) {
        size_t drop = sizeof(s_rx_text) / 4;
        memmove(s_rx_text, s_rx_text + drop, s_rx_len - drop);
        s_rx_len -= drop;
    }

    s_rx_text[s_rx_len++] = ch;
    s_rx_text[s_rx_len] = '\0';
    xSemaphoreGive(s_lock);
}

static void serial_rx_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "stdin RX task started");

    for (;;) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        append_rx_char((char)ch);
    }
}

static void remember_command(const char *text)
{
    if (!text || !text[0] || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);

    strlcpy(s_last_tx, text, sizeof(s_last_tx));
    if (s_history_count > 0 && strcmp(s_history[s_history_count - 1], text) == 0) {
        s_history_cursor = s_history_count;
        xSemaphoreGive(s_lock);
        return;
    }

    if (s_history_count < SERIAL_HISTORY_DEPTH) {
        strlcpy(s_history[s_history_count++], text, sizeof(s_history[0]));
    } else {
        memmove(s_history, s_history + 1, sizeof(s_history[0]) * (SERIAL_HISTORY_DEPTH - 1));
        strlcpy(s_history[SERIAL_HISTORY_DEPTH - 1], text, sizeof(s_history[0]));
    }
    s_history_cursor = s_history_count;
    xSemaphoreGive(s_lock);
}

static esp_err_t parse_hex_text(const char *text, uint8_t *out, size_t out_size, size_t *out_len)
{
    if (!text || !out || !out_len) return ESP_ERR_INVALID_ARG;
    *out_len = 0;

    const char *p = text;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])) return ESP_ERR_INVALID_ARG;
        if (*out_len >= out_size) return ESP_ERR_INVALID_SIZE;

        char byte_text[3] = {p[0], p[1], '\0'};
        out[(*out_len)++] = (uint8_t)strtoul(byte_text, NULL, 16);
        p += 2;

        if (*p && !isspace((unsigned char)*p)) return ESP_ERR_INVALID_ARG;
    }

    return *out_len ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

static esp_err_t write_payload(const uint8_t *data, size_t len, serial_ending_t ending)
{
    size_t written = fwrite(data, 1, len, stdout);
    size_t ending_written = 0;
    if (ending == SERIAL_ENDING_LF) ending_written = fwrite("\n", 1, 1, stdout);
    else if (ending == SERIAL_ENDING_CR) ending_written = fwrite("\r", 1, 1, stdout);
    else if (ending == SERIAL_ENDING_CRLF) ending_written = fwrite("\r\n", 1, 2, stdout);
    fflush(stdout);

    size_t expected_ending = ending == SERIAL_ENDING_NONE ? 0 : (ending == SERIAL_ENDING_CRLF ? 2 : 1);
    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_tx_bytes += written + ending_written;
        xSemaphoreGive(s_lock);
    }

    return (written == len && ending_written == expected_ending) ? ESP_OK : ESP_FAIL;
}

esp_err_t serial_service_init(void)
{
    if (s_lock) return ESP_OK;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    s_rx_text[0] = '\0';
    s_rx_len = 0;
    s_rx_bytes = 0;
    s_tx_bytes = 0;
    s_ending = SERIAL_ENDING_CRLF;
    s_tx_mode = SERIAL_TX_MODE_ASCII;
    s_history_count = 0;
    s_history_cursor = 0;
    s_last_tx[0] = '\0';

    BaseType_t ok = xTaskCreate(serial_rx_task,
                                "serial_rx",
                                SERIAL_RX_TASK_STACK,
                                NULL,
                                SERIAL_RX_TASK_PRIORITY,
                                NULL);
    if (ok != pdPASS) {
        vSemaphoreDelete(s_lock);
        s_lock = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "UART0 console bridge ready through stdin/stdout");
    return ESP_OK;
}

esp_err_t serial_service_send_test(void)
{
    static const char test[] = "KONTAKTS USB SERIAL TEST";
    /* Diagnostic monitor action must remain deterministic even if the
     * Advanced Terminal previously selected HEX or another line ending. */
    return write_payload((const uint8_t *)test, sizeof(test) - 1, SERIAL_ENDING_CRLF);
}

esp_err_t serial_service_send_text(const char *text)
{
    if (!text) return ESP_ERR_INVALID_ARG;
    size_t len = strnlen(text, SERIAL_TX_TEXT_MAX + 1);
    if (len == 0 || len > SERIAL_TX_TEXT_MAX) return ESP_ERR_INVALID_SIZE;

    serial_ending_t ending;
    serial_tx_mode_t mode;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    ending = s_ending;
    mode = s_tx_mode;
    xSemaphoreGive(s_lock);

    esp_err_t err;
    if (mode == SERIAL_TX_MODE_HEX) {
        uint8_t bytes[SERIAL_TX_TEXT_MAX / 2 + 1];
        size_t byte_count = 0;
        err = parse_hex_text(text, bytes, sizeof(bytes), &byte_count);
        if (err == ESP_OK) err = write_payload(bytes, byte_count, ending);
    } else {
        err = write_payload((const uint8_t *)text, len, ending);
    }

    if (err == ESP_OK) remember_command(text);
    return err;
}

esp_err_t serial_service_repeat_last(void)
{
    char text[SERIAL_HISTORY_TEXT_MAX + 1];
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strlcpy(text, s_last_tx, sizeof(text));
    xSemaphoreGive(s_lock);
    return text[0] ? serial_service_send_text(text) : ESP_ERR_NOT_FOUND;
}

void serial_service_set_ending(serial_ending_t ending)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ending = ending;
    xSemaphoreGive(s_lock);
}

void serial_service_set_tx_mode(serial_tx_mode_t mode)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_tx_mode = mode;
    xSemaphoreGive(s_lock);
}

esp_err_t serial_service_history_prev(char *out, size_t out_len)
{
    if (!out || out_len == 0 || !s_lock) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_history_count == 0) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_NOT_FOUND;
    }
    if (s_history_cursor > 0) s_history_cursor--;
    strlcpy(out, s_history[s_history_cursor], out_len);
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t serial_service_history_next(char *out, size_t out_len)
{
    if (!out || out_len == 0 || !s_lock) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_history_count == 0) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_NOT_FOUND;
    }
    if (s_history_cursor + 1 < s_history_count) {
        s_history_cursor++;
        strlcpy(out, s_history[s_history_cursor], out_len);
    } else {
        s_history_cursor = s_history_count;
        out[0] = '\0';
    }
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t serial_service_clear(void)
{
    if (!s_lock) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_rx_text[0] = '\0';
    s_rx_len = 0;
    s_rx_bytes = 0;
    s_tx_bytes = 0;
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

void serial_service_format_binding(const char *binding, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    out[0] = '\0';

    if (!binding || !s_lock) {
        strlcpy(out, "offline", out_len);
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (strcmp(binding, "serial.rx_text") == 0) {
        strlcpy(out, s_rx_text[0] ? s_rx_text : "(waiting for USB serial input)", out_len);
    } else if (strcmp(binding, "serial.rx_bytes") == 0) {
        snprintf(out, out_len, "%llu", (unsigned long long)s_rx_bytes);
    } else if (strcmp(binding, "serial.tx_bytes") == 0) {
        snprintf(out, out_len, "%llu", (unsigned long long)s_tx_bytes);
    } else if (strcmp(binding, "serial.state") == 0) {
        strlcpy(out, "UART0 / CH340C / 115200 8N1", out_len);
    } else if (strcmp(binding, "serial.ending") == 0) {
        strlcpy(out, ending_name(s_ending), out_len);
    } else if (strcmp(binding, "serial.tx_mode") == 0) {
        strlcpy(out, mode_name(s_tx_mode), out_len);
    } else if (strcmp(binding, "serial.last_tx") == 0) {
        strlcpy(out, s_last_tx[0] ? s_last_tx : "-", out_len);
    } else if (strcmp(binding, "serial.history_count") == 0) {
        snprintf(out, out_len, "%u", (unsigned)s_history_count);
    } else {
        strlcpy(out, "unsupported", out_len);
    }
    xSemaphoreGive(s_lock);
}
