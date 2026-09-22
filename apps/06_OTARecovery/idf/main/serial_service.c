#include "serial_service.h"

#include <stdio.h>
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

static SemaphoreHandle_t s_lock;
static char s_rx_text[SERIAL_RX_TEXT_BYTES];
static size_t s_rx_len;
static uint64_t s_rx_bytes;
static uint64_t s_tx_bytes;

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

esp_err_t serial_service_init(void)
{
    if (s_lock) return ESP_OK;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    s_rx_text[0] = '\0';
    s_rx_len = 0;
    s_rx_bytes = 0;
    s_tx_bytes = 0;

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
    static const char test[] = "KONTAKTS USB SERIAL TEST\r\n";
    size_t written = fwrite(test, 1, sizeof(test) - 1, stdout);
    fflush(stdout);

    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_tx_bytes += written;
        xSemaphoreGive(s_lock);
    }

    return written == sizeof(test) - 1 ? ESP_OK : ESP_FAIL;
}

esp_err_t serial_service_send_text(const char *text)
{
    if (!text) return ESP_ERR_INVALID_ARG;

    size_t len = strnlen(text, SERIAL_TX_TEXT_MAX + 1);
    if (len == 0 || len > SERIAL_TX_TEXT_MAX) return ESP_ERR_INVALID_SIZE;

    static const char ending[] = "\r\n";
    size_t text_written = fwrite(text, 1, len, stdout);
    size_t ending_written = fwrite(ending, 1, sizeof(ending) - 1, stdout);
    fflush(stdout);

    size_t total = text_written + ending_written;
    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_tx_bytes += total;
        xSemaphoreGive(s_lock);
    }

    return (text_written == len && ending_written == sizeof(ending) - 1) ? ESP_OK : ESP_FAIL;
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
    } else {
        strlcpy(out, "unsupported", out_len);
    }
    xSemaphoreGive(s_lock);
}
