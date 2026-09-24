#include "p4_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define TAG "P4_SERVICE"

#define P4_UART UART_NUM_1
#define P4_TX_PIN GPIO_NUM_17
#define P4_RX_PIN GPIO_NUM_18

#define P4_RX_BUFFER_BYTES 1024
#define P4_RX_TEXT_BYTES 512
#define P4_TX_TEXT_MAX 256
#define P4_RX_TASK_STACK 4096
#define P4_RX_TASK_PRIORITY 5

static SemaphoreHandle_t s_lock;
static SemaphoreHandle_t s_io_lock;
static p4_mode_t s_mode = P4_MODE_IDLE;
static bool s_uart_installed;
static int s_baud = 9600;
static p4_ending_t s_ending = P4_ENDING_NONE;
static p4_tx_mode_t s_tx_mode = P4_TX_ASCII;
static char s_rx_text[P4_RX_TEXT_BYTES];
static size_t s_rx_len;
static uint64_t s_rx_bytes;
static uint64_t s_tx_bytes;
static bool s_gpio17;
static bool s_gpio18;

static const char *ending_name(p4_ending_t ending)
{
    switch (ending) {
        case P4_ENDING_LF: return "LF";
        case P4_ENDING_CR: return "CR";
        case P4_ENDING_CRLF: return "CRLF";
        default: return "NONE";
    }
}

static const char *tx_mode_name(p4_tx_mode_t mode)
{
    return mode == P4_TX_HEX ? "HEX" : "ASCII";
}

static void append_rx(const uint8_t *data, size_t len)
{
    if (!data || !len || !s_lock) return;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_rx_bytes += len;

    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = data[i];
        char out = (ch >= 32 && ch <= 126) ? (char)ch : '.';

        if (s_rx_len + 1 >= sizeof(s_rx_text)) {
            size_t drop = sizeof(s_rx_text) / 4;
            memmove(s_rx_text, s_rx_text + drop, s_rx_len - drop);
            s_rx_len -= drop;
        }
        s_rx_text[s_rx_len++] = out;
    }
    s_rx_text[s_rx_len] = '\0';
    xSemaphoreGive(s_lock);
}

static bool uart_active(void)
{
    bool active = false;
    if (!s_lock) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    active = s_mode == P4_MODE_UART1 && s_uart_installed;
    xSemaphoreGive(s_lock);
    return active;
}

static void rx_task(void *arg)
{
    (void)arg;
    uint8_t buffer[96];

    for (;;) {
        if (!uart_active()) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        int count = 0;
        xSemaphoreTake(s_io_lock, portMAX_DELAY);
        if (uart_active()) {
            count = uart_read_bytes(P4_UART, buffer, sizeof(buffer), pdMS_TO_TICKS(20));
        }
        xSemaphoreGive(s_io_lock);

        if (count > 0) append_rx(buffer, (size_t)count);
    }
}

static void reset_pins_to_input(void)
{
    gpio_reset_pin(P4_TX_PIN);
    gpio_reset_pin(P4_RX_PIN);
    gpio_set_direction(P4_TX_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(P4_RX_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(P4_TX_PIN, GPIO_FLOATING);
    gpio_set_pull_mode(P4_RX_PIN, GPIO_FLOATING);
}

static esp_err_t stop_uart_locked(void)
{
    if (!s_uart_installed) return ESP_OK;

    esp_err_t err = uart_wait_tx_done(P4_UART, pdMS_TO_TICKS(100));
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "uart_wait_tx_done: %s", esp_err_to_name(err));
    }

    err = uart_driver_delete(P4_UART);
    if (err == ESP_OK) s_uart_installed = false;
    return err;
}

static esp_err_t parse_hex_text(const char *text, uint8_t *out, size_t out_size, size_t *out_len)
{
    if (!text || !out || !out_len) return ESP_ERR_INVALID_ARG;
    *out_len = 0;

    const char *p = text;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1]))
            return ESP_ERR_INVALID_ARG;
        if (*out_len >= out_size) return ESP_ERR_INVALID_SIZE;

        char byte_text[3] = {p[0], p[1], '\0'};
        out[(*out_len)++] = (uint8_t)strtoul(byte_text, NULL, 16);
        p += 2;

        if (*p && !isspace((unsigned char)*p)) return ESP_ERR_INVALID_ARG;
    }

    return *out_len ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

esp_err_t p4_service_init(void)
{
    if (s_lock) return ESP_OK;

    s_lock = xSemaphoreCreateMutex();
    s_io_lock = xSemaphoreCreateMutex();
    if (!s_lock || !s_io_lock) return ESP_ERR_NO_MEM;

    s_rx_text[0] = '\0';
    reset_pins_to_input();

    BaseType_t ok = xTaskCreate(rx_task,
                                "p4_rx",
                                P4_RX_TASK_STACK,
                                NULL,
                                P4_RX_TASK_PRIORITY,
                                NULL);
    if (ok != pdPASS) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG, "P4 ready: GPIO17/18 flex port; GPIO19/20 remain system I2C");
    return ESP_OK;
}

esp_err_t p4_service_idle(void)
{
    if (!s_lock || !s_io_lock) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_io_lock, portMAX_DELAY);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_mode = P4_MODE_IDLE;
    xSemaphoreGive(s_lock);

    esp_err_t err = stop_uart_locked();
    reset_pins_to_input();
    xSemaphoreGive(s_io_lock);

    return err;
}

esp_err_t p4_service_uart_enable(int baud)
{
    if (!s_lock || !s_io_lock) return ESP_ERR_INVALID_STATE;
    if (baud < 1200 || baud > 1000000) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_io_lock, portMAX_DELAY);

    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool already_uart = s_mode == P4_MODE_UART1 && s_uart_installed;
    xSemaphoreGive(s_lock);

    if (already_uart) {
        esp_err_t err = uart_set_baudrate(P4_UART, baud);
        if (err == ESP_OK) {
            xSemaphoreTake(s_lock, portMAX_DELAY);
            s_baud = baud;
            xSemaphoreGive(s_lock);
        }
        xSemaphoreGive(s_io_lock);
        return err;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_mode = P4_MODE_IDLE;
    xSemaphoreGive(s_lock);

    esp_err_t err = stop_uart_locked();
    if (err != ESP_OK) {
        xSemaphoreGive(s_io_lock);
        return err;
    }

    reset_pins_to_input();

    const uart_config_t config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };

    err = uart_param_config(P4_UART, &config);
    if (err == ESP_OK)
        err = uart_set_pin(P4_UART, P4_TX_PIN, P4_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err == ESP_OK)
        err = uart_driver_install(P4_UART, P4_RX_BUFFER_BYTES, 0, 0, NULL, 0);

    if (err == ESP_OK) {
        uart_flush_input(P4_UART);
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_uart_installed = true;
        s_mode = P4_MODE_UART1;
        s_baud = baud;
        xSemaphoreGive(s_lock);
        ESP_LOGI(TAG, "UART1 enabled TX=GPIO17 RX=GPIO18 baud=%d", baud);
    } else {
        reset_pins_to_input();
    }

    xSemaphoreGive(s_io_lock);
    return err;
}

esp_err_t p4_service_uart_send_text(const char *text)
{
    if (!text || !s_lock || !s_io_lock) return ESP_ERR_INVALID_ARG;

    size_t len = strnlen(text, P4_TX_TEXT_MAX + 1);
    if (len == 0 || len > P4_TX_TEXT_MAX) return ESP_ERR_INVALID_SIZE;

    p4_tx_mode_t mode;
    p4_ending_t ending;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool active = s_mode == P4_MODE_UART1 && s_uart_installed;
    mode = s_tx_mode;
    ending = s_ending;
    xSemaphoreGive(s_lock);
    if (!active) return ESP_ERR_INVALID_STATE;

    uint8_t binary[P4_TX_TEXT_MAX / 2 + 1];
    const uint8_t *payload = (const uint8_t *)text;
    size_t payload_len = len;

    if (mode == P4_TX_HEX) {
        esp_err_t parse_err = parse_hex_text(text, binary, sizeof(binary), &payload_len);
        if (parse_err != ESP_OK) return parse_err;
        payload = binary;
    }

    const char *tail = "";
    size_t tail_len = 0;
    if (ending == P4_ENDING_LF) { tail = "\n"; tail_len = 1; }
    else if (ending == P4_ENDING_CR) { tail = "\r"; tail_len = 1; }
    else if (ending == P4_ENDING_CRLF) { tail = "\r\n"; tail_len = 2; }

    xSemaphoreTake(s_io_lock, portMAX_DELAY);
    int written = uart_write_bytes(P4_UART, payload, payload_len);
    int tail_written = tail_len ? uart_write_bytes(P4_UART, tail, tail_len) : 0;
    xSemaphoreGive(s_io_lock);

    if (written < 0 || (size_t)written != payload_len ||
        (tail_len && (tail_written < 0 || (size_t)tail_written != tail_len))) {
        return ESP_FAIL;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_tx_bytes += payload_len + tail_len;
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

void p4_service_set_ending(p4_ending_t ending)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ending = ending;
    xSemaphoreGive(s_lock);
}

void p4_service_set_tx_mode(p4_tx_mode_t mode)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_tx_mode = mode;
    xSemaphoreGive(s_lock);
}

esp_err_t p4_service_clear(void)
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

esp_err_t p4_service_gpio_enable(void)
{
    if (!s_lock || !s_io_lock) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_io_lock, portMAX_DELAY);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_mode = P4_MODE_IDLE;
    xSemaphoreGive(s_lock);

    esp_err_t err = stop_uart_locked();
    if (err != ESP_OK) {
        xSemaphoreGive(s_io_lock);
        return err;
    }

    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << P4_TX_PIN) | (1ULL << P4_RX_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&io);
    if (err == ESP_OK) err = gpio_set_level(P4_TX_PIN, 0);
    if (err == ESP_OK) err = gpio_set_level(P4_RX_PIN, 0);

    if (err == ESP_OK) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_gpio17 = false;
        s_gpio18 = false;
        s_mode = P4_MODE_GPIO_OUT;
        xSemaphoreGive(s_lock);
        ESP_LOGI(TAG, "GPIO output mode enabled on GPIO17/18, both LOW");
    }

    xSemaphoreGive(s_io_lock);
    return err;
}

esp_err_t p4_service_gpio_set(int gpio_num, bool level)
{
    if (!s_lock || !s_io_lock) return ESP_ERR_INVALID_STATE;
    if (gpio_num != 17 && gpio_num != 18) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool active = s_mode == P4_MODE_GPIO_OUT;
    xSemaphoreGive(s_lock);
    if (!active) return ESP_ERR_INVALID_STATE;

    gpio_num_t pin = gpio_num == 17 ? P4_TX_PIN : P4_RX_PIN;

    xSemaphoreTake(s_io_lock, portMAX_DELAY);
    esp_err_t err = gpio_set_level(pin, level ? 1 : 0);
    xSemaphoreGive(s_io_lock);

    if (err == ESP_OK) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (gpio_num == 17) s_gpio17 = level;
        else s_gpio18 = level;
        xSemaphoreGive(s_lock);
    }
    return err;
}

void p4_service_format_binding(const char *binding, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    out[0] = '\0';

    if (!binding || !s_lock) {
        strlcpy(out, "offline", out_len);
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);

    if (strcmp(binding, "p4.state") == 0) {
        if (s_mode == P4_MODE_UART1)
            snprintf(out, out_len, "UART1 GPIO17/18 %d 8N1", s_baud);
        else if (s_mode == P4_MODE_GPIO_OUT)
            strlcpy(out, "GPIO OUT 17/18", out_len);
        else
            strlcpy(out, "IDLE", out_len);
    } else if (strcmp(binding, "p4.mode") == 0) {
        strlcpy(out, s_mode == P4_MODE_UART1 ? "UART1" :
                     s_mode == P4_MODE_GPIO_OUT ? "GPIO" : "IDLE", out_len);
    } else if (strcmp(binding, "p4.rx_text") == 0) {
        strlcpy(out, s_rx_text[0] ? s_rx_text : "(waiting)", out_len);
    } else if (strcmp(binding, "p4.rx_bytes") == 0) {
        snprintf(out, out_len, "%llu", (unsigned long long)s_rx_bytes);
    } else if (strcmp(binding, "p4.tx_bytes") == 0) {
        snprintf(out, out_len, "%llu", (unsigned long long)s_tx_bytes);
    } else if (strcmp(binding, "p4.baud") == 0) {
        snprintf(out, out_len, "%d", s_baud);
    } else if (strcmp(binding, "p4.ending") == 0) {
        strlcpy(out, ending_name(s_ending), out_len);
    } else if (strcmp(binding, "p4.tx_mode") == 0) {
        strlcpy(out, tx_mode_name(s_tx_mode), out_len);
    } else if (strcmp(binding, "p4.gpio17") == 0) {
        strlcpy(out, s_gpio17 ? "ON" : "OFF", out_len);
    } else if (strcmp(binding, "p4.gpio18") == 0) {
        strlcpy(out, s_gpio18 ? "ON" : "OFF", out_len);
    } else {
        strlcpy(out, "unsupported", out_len);
    }

    xSemaphoreGive(s_lock);
}
