#include "modbus_service.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define TAG "MODBUS_SERVICE"

#define MODBUS_UART              UART_NUM_1
#define MODBUS_TX_GPIO           17
#define MODBUS_RX_GPIO           18
#define MODBUS_BAUD              9600
#define MODBUS_RX_BUFFER_BYTES   256
#define MODBUS_TIMEOUT_MS        250
#define MODBUS_POLL_MS           1000
#define MODBUS_EID041_SLAVE      1

static SemaphoreHandle_t s_bus_lock;
static SemaphoreHandle_t s_status_lock;
static modbus_service_status_t s_status;

static uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; ++pos) {
        crc ^= data[pos];
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA001);
            else crc >>= 1;
        }
    }
    return crc;
}

static void status_frame_tx(void)
{
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_status.tx_frames++;
    xSemaphoreGive(s_status_lock);
}

static void status_frame_rx(void)
{
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_status.rx_frames++;
    xSemaphoreGive(s_status_lock);
}

static void status_error(bool timeout, bool crc)
{
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_status.online = false;
    if (timeout) s_status.timeouts++;
    else if (crc) s_status.crc_errors++;
    else s_status.protocol_errors++;
    xSemaphoreGive(s_status_lock);
}

esp_err_t modbus_service_read_registers(uint8_t slave,
                                        uint8_t function,
                                        uint16_t start_register,
                                        uint16_t register_count,
                                        uint16_t *out_registers,
                                        size_t out_count)
{
    if (!s_bus_lock || !s_status_lock) return ESP_ERR_INVALID_STATE;
    if (!slave || (function != 0x03 && function != 0x04) ||
        !register_count || register_count > 32 ||
        !out_registers || out_count < register_count) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t request[8] = {
        slave,
        function,
        (uint8_t)(start_register >> 8),
        (uint8_t)(start_register & 0xFF),
        (uint8_t)(register_count >> 8),
        (uint8_t)(register_count & 0xFF),
        0,
        0,
    };
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = (uint8_t)(crc & 0xFF);
    request[7] = (uint8_t)(crc >> 8);

    const size_t expected = 5U + (size_t)register_count * 2U;
    uint8_t response[69] = {0};

    xSemaphoreTake(s_bus_lock, portMAX_DELAY);
    uart_flush_input(MODBUS_UART);

    int written = uart_write_bytes(MODBUS_UART, request, sizeof(request));
    if (written != (int)sizeof(request)) {
        xSemaphoreGive(s_bus_lock);
        status_error(false, false);
        return ESP_FAIL;
    }
    status_frame_tx();

    esp_err_t wait_err = uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));
    if (wait_err != ESP_OK) {
        xSemaphoreGive(s_bus_lock);
        status_error(true, false);
        return wait_err;
    }

    size_t got = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(MODBUS_TIMEOUT_MS);
    while (got < expected) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) break;
        int n = uart_read_bytes(MODBUS_UART,
                                response + got,
                                expected - got,
                                deadline - now);
        if (n > 0) got += (size_t)n;
    }
    xSemaphoreGive(s_bus_lock);

    if (got != expected) {
        status_error(true, false);
        return ESP_ERR_TIMEOUT;
    }

    uint16_t response_crc = (uint16_t)response[expected - 2] |
                            ((uint16_t)response[expected - 1] << 8);
    if (modbus_crc16(response, expected - 2) != response_crc) {
        status_error(false, true);
        return ESP_ERR_INVALID_CRC;
    }

    if (response[0] != slave || response[1] != function ||
        response[2] != register_count * 2U) {
        status_error(false, false);
        return ESP_ERR_INVALID_RESPONSE;
    }

    for (uint16_t i = 0; i < register_count; ++i) {
        out_registers[i] = ((uint16_t)response[3 + i * 2] << 8) |
                           response[4 + i * 2];
    }

    status_frame_rx();
    return ESP_OK;
}

static void eid041_poll_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "EID041 provider poll started: slave=%u, FC04, regs 0x0000..0x0001",
             MODBUS_EID041_SLAVE);

    for (;;) {
        uint16_t regs[2] = {0};
        esp_err_t err = modbus_service_read_registers(MODBUS_EID041_SLAVE,
                                                      0x04,
                                                      0x0000,
                                                      2,
                                                      regs,
                                                      2);
        if (err == ESP_OK) {
            xSemaphoreTake(s_status_lock, portMAX_DELAY);
            s_status.temperature_tenths_c = (int16_t)regs[0];
            s_status.humidity_tenths_rh = regs[1];
            s_status.online = true;
            xSemaphoreGive(s_status_lock);
        } else {
            ESP_LOGW(TAG, "EID041 poll failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(MODBUS_POLL_MS));
    }
}

esp_err_t modbus_service_init(void)
{
    if (s_bus_lock) return ESP_OK;

    s_bus_lock = xSemaphoreCreateMutex();
    s_status_lock = xSemaphoreCreateMutex();
    if (!s_bus_lock || !s_status_lock) return ESP_ERR_NO_MEM;

    uart_config_t config = {
        .baud_rate = MODBUS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(MODBUS_UART,
                                        MODBUS_RX_BUFFER_BYTES,
                                        0,
                                        0,
                                        NULL,
                                        0);
    if (err != ESP_OK) return err;

    err = uart_param_config(MODBUS_UART, &config);
    if (err != ESP_OK) return err;

    err = uart_set_pin(MODBUS_UART,
                       MODBUS_TX_GPIO,
                       MODBUS_RX_GPIO,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    memset(&s_status, 0, sizeof(s_status));

    BaseType_t task_ok = xTaskCreate(eid041_poll_task,
                                     "modbus_eid041",
                                     4096,
                                     NULL,
                                     5,
                                     NULL);
    if (task_ok != pdPASS) return ESP_ERR_NO_MEM;

    ESP_LOGI(TAG,
             "Modbus RTU master ready: UART1 TX=GPIO%d RX=GPIO%d %d 8N1",
             MODBUS_TX_GPIO, MODBUS_RX_GPIO, MODBUS_BAUD);
    return ESP_OK;
}

void modbus_service_get_status(modbus_service_status_t *out)
{
    if (!out || !s_status_lock) return;
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    *out = s_status;
    xSemaphoreGive(s_status_lock);
}

void modbus_service_format_binding(const char *binding, char *out, size_t out_len)
{
    if (!out || out_len == 0) return;
    out[0] = '\0';

    if (!binding || !s_status_lock) {
        strlcpy(out, "offline", out_len);
        return;
    }

    modbus_service_status_t status;
    modbus_service_get_status(&status);

    if (strcmp(binding, "modbus.state") == 0) {
        strlcpy(out, status.online ? "ONLINE" : "NO RESPONSE", out_len);
    } else if (strcmp(binding, "modbus.bus") == 0) {
        strlcpy(out, "UART1 GPIO17/18 | 9600 8N1 | RS485", out_len);
    } else if (strcmp(binding, "modbus.tx_count") == 0) {
        snprintf(out, out_len, "%lu", (unsigned long)status.tx_frames);
    } else if (strcmp(binding, "modbus.rx_count") == 0) {
        snprintf(out, out_len, "%lu", (unsigned long)status.rx_frames);
    } else if (strcmp(binding, "modbus.crc_errors") == 0) {
        snprintf(out, out_len, "%lu", (unsigned long)status.crc_errors);
    } else if (strcmp(binding, "modbus.timeout_count") == 0) {
        snprintf(out, out_len, "%lu", (unsigned long)status.timeouts);
    } else if (strcmp(binding, "modbus.protocol_errors") == 0) {
        snprintf(out, out_len, "%lu", (unsigned long)status.protocol_errors);
    } else if (strcmp(binding, "modbus.sensor1.temperature") == 0 ||
               strcmp(binding, "temperature.value") == 0) {
        int16_t v = status.temperature_tenths_c;
        int16_t abs_v = v < 0 ? (int16_t)-v : v;
        snprintf(out, out_len, "%s%d.%d",
                 v < 0 ? "-" : "",
                 abs_v / 10,
                 abs_v % 10);
    } else if (strcmp(binding, "modbus.sensor1.humidity") == 0 ||
               strcmp(binding, "humidity.value") == 0) {
        snprintf(out, out_len, "%u.%u",
                 (unsigned)(status.humidity_tenths_rh / 10),
                 (unsigned)(status.humidity_tenths_rh % 10));
    } else {
        strlcpy(out, "unsupported", out_len);
    }
}
