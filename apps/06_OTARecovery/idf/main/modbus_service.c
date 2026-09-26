#include "modbus_service.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"

#define TAG "MODBUS_SERVICE"

#define MODBUS_UART              UART_NUM_1
#define MODBUS_TX_GPIO           17
#define MODBUS_RX_GPIO           18
#define MODBUS_BAUD              9600
#define MODBUS_RX_BUFFER_BYTES   256
#define MODBUS_TIMEOUT_MS        250
#define MODBUS_POLL_MS           5000
#define MODBUS_EID041_SLAVE      1
#define MODBUS_MA01_COILS         8
#define MODBUS_MA01_MODE_BASE     0x0578
#define MODBUS_MA01_PULSE_BASE    0x05DC
#define MODBUS_NVS_NAMESPACE      "platform"
#define MODBUS_NVS_MA01_ADDR      "ma01_addr"

static bool s_ma01_coils[MODBUS_MA01_COILS];
static uint16_t s_ma01_modes[MODBUS_MA01_COILS];
static uint16_t s_ma01_pulse_ms[MODBUS_MA01_COILS];
static bool s_ma01_online;
static bool s_ma01_config_valid;
static uint8_t s_ma01_slave;

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

static void ma01_load_slave(void)
{
    s_ma01_slave = 0;

    nvs_handle_t handle;
    if (nvs_open(MODBUS_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }

    uint8_t slave = 0;
    if (nvs_get_u8(handle, MODBUS_NVS_MA01_ADDR, &slave) == ESP_OK &&
        slave >= 1 && slave <= 247) {
        s_ma01_slave = slave;
    }
    nvs_close(handle);
}

uint8_t modbus_service_ma01_get_slave(void)
{
    if (!s_status_lock) return 0;
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    uint8_t slave = s_ma01_slave;
    xSemaphoreGive(s_status_lock);
    return slave;
}

esp_err_t modbus_service_ma01_set_slave(uint8_t slave)
{
    if (!s_status_lock) return ESP_ERR_INVALID_STATE;
    if (slave < 1 || slave > 247) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(MODBUS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_u8(handle, MODBUS_NVS_MA01_ADDR, slave);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_ma01_slave = slave;
    s_ma01_online = false;
    s_ma01_config_valid = false;
    memset(s_ma01_coils, 0, sizeof(s_ma01_coils));
    memset(s_ma01_modes, 0, sizeof(s_ma01_modes));
    memset(s_ma01_pulse_ms, 0, sizeof(s_ma01_pulse_ms));
    xSemaphoreGive(s_status_lock);

    ESP_LOGI(TAG, "MA01 slave address set to %u and saved", (unsigned)slave);
    return ESP_OK;
}

esp_err_t modbus_service_ma01_scan(uint8_t *out_slave, uint16_t *out_model, uint16_t *out_fw)
{
    if (!s_bus_lock || !s_status_lock) return ESP_ERR_INVALID_STATE;

    for (unsigned slave = 1; slave <= 247; ++slave) {
        uint16_t model = 0;
        esp_err_t err = modbus_service_read_registers((uint8_t)slave, 0x03, 0x07D0, 1, &model, 1);
        if (err != ESP_OK) continue;

        uint16_t fw = 0;
        err = modbus_service_read_registers((uint8_t)slave, 0x03, 0x07DC, 1, &fw, 1);
        if (err != ESP_OK) continue;

        err = modbus_service_ma01_set_slave((uint8_t)slave);
        if (err != ESP_OK) return err;

        if (out_slave) *out_slave = (uint8_t)slave;
        if (out_model) *out_model = model;
        if (out_fw) *out_fw = fw;
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
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

static esp_err_t modbus_service_write_single_register(uint8_t slave, uint16_t reg, uint16_t value)
{
    if (!s_bus_lock || !s_status_lock || !slave) return ESP_ERR_INVALID_STATE;

    uint8_t request[8] = {
        slave, 0x06,
        (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
        (uint8_t)(value >> 8), (uint8_t)(value & 0xFF), 0, 0
    };
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = (uint8_t)(crc & 0xFF);
    request[7] = (uint8_t)(crc >> 8);

    uint8_t response[8] = {0};

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

    int got = uart_read_bytes(MODBUS_UART, response, sizeof(response), pdMS_TO_TICKS(MODBUS_TIMEOUT_MS));
    xSemaphoreGive(s_bus_lock);

    if (got != (int)sizeof(response)) {
        status_error(true, false);
        return ESP_ERR_TIMEOUT;
    }

    uint16_t response_crc = (uint16_t)response[6] | ((uint16_t)response[7] << 8);
    if (modbus_crc16(response, 6) != response_crc) {
        status_error(false, true);
        return ESP_ERR_INVALID_CRC;
    }
    if (memcmp(request, response, 6) != 0) {
        status_error(false, false);
        return ESP_ERR_INVALID_RESPONSE;
    }

    status_frame_rx();
    return ESP_OK;
}

esp_err_t modbus_service_ma01_refresh_config(void)
{
    uint8_t slave = modbus_service_ma01_get_slave();
    if (!slave) return ESP_ERR_INVALID_STATE;

    uint16_t modes[MODBUS_MA01_COILS] = {0};
    uint16_t pulse_ms[MODBUS_MA01_COILS] = {0};

    esp_err_t err = modbus_service_read_registers(slave, 0x03,
                                                  MODBUS_MA01_MODE_BASE,
                                                  MODBUS_MA01_COILS,
                                                  modes,
                                                  MODBUS_MA01_COILS);
    if (err != ESP_OK) return err;

    err = modbus_service_read_registers(slave, 0x03,
                                        MODBUS_MA01_PULSE_BASE,
                                        MODBUS_MA01_COILS,
                                        pulse_ms,
                                        MODBUS_MA01_COILS);
    if (err != ESP_OK) return err;

    for (unsigned i = 0; i < MODBUS_MA01_COILS; ++i) {
        if (modes[i] > MODBUS_MA01_MODE_FOLLOW) return ESP_ERR_INVALID_RESPONSE;
    }

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    memcpy(s_ma01_modes, modes, sizeof(s_ma01_modes));
    memcpy(s_ma01_pulse_ms, pulse_ms, sizeof(s_ma01_pulse_ms));
    s_ma01_config_valid = true;
    xSemaphoreGive(s_status_lock);

    return ESP_OK;
}

static esp_err_t ma01_ensure_config(void)
{
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    bool valid = s_ma01_config_valid;
    xSemaphoreGive(s_status_lock);
    return valid ? ESP_OK : modbus_service_ma01_refresh_config();
}

static void ma01_delayed_refresh_task(void *arg)
{
    uint32_t delay_ms = (uint32_t)(uintptr_t)arg;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    esp_err_t err = modbus_service_ma01_refresh();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MA01 delayed pulse refresh failed: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}

static void ma01_schedule_refresh(uint32_t delay_ms)
{
    if (delay_ms < 100) delay_ms = 100;
    if (delay_ms > 66000) delay_ms = 66000;
    BaseType_t ok = xTaskCreate(ma01_delayed_refresh_task,
                                "ma01_pulse_refresh",
                                3072,
                                (void *)(uintptr_t)delay_ms,
                                4,
                                NULL);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "MA01 delayed refresh task allocation failed");
    }
}

esp_err_t modbus_service_read_coils(uint8_t slave,
                                    uint16_t start_coil,
                                    uint16_t coil_count,
                                    bool *out_coils,
                                    size_t out_count)
{
    if (!s_bus_lock || !s_status_lock) return ESP_ERR_INVALID_STATE;
    if (!slave || !coil_count || coil_count > 32 || !out_coils || out_count < coil_count) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t request[8] = {
        slave, 0x01,
        (uint8_t)(start_coil >> 8), (uint8_t)(start_coil & 0xFF),
        (uint8_t)(coil_count >> 8), (uint8_t)(coil_count & 0xFF), 0, 0
    };
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = (uint8_t)(crc & 0xFF);
    request[7] = (uint8_t)(crc >> 8);

    size_t byte_count = (coil_count + 7U) / 8U;
    size_t expected = 5U + byte_count;
    uint8_t response[9] = {0};

    ESP_LOGI(TAG, "FC01 TX slave=%u start=0x%04X count=%u",
             (unsigned)slave, (unsigned)start_coil, (unsigned)coil_count);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, request, sizeof(request), ESP_LOG_INFO);

    xSemaphoreTake(s_bus_lock, portMAX_DELAY);
    uart_flush_input(MODBUS_UART);
    int written = uart_write_bytes(MODBUS_UART, request, sizeof(request));
    if (written != (int)sizeof(request)) {
        xSemaphoreGive(s_bus_lock); status_error(false, false); return ESP_FAIL;
    }
    status_frame_tx();
    esp_err_t wait_err = uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));
    if (wait_err != ESP_OK) {
        xSemaphoreGive(s_bus_lock); status_error(true, false); return wait_err;
    }

    size_t got = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(MODBUS_TIMEOUT_MS);
    while (got < expected) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) break;
        int n = uart_read_bytes(MODBUS_UART, response + got, expected - got, deadline - now);
        if (n > 0) got += (size_t)n;
    }
    xSemaphoreGive(s_bus_lock);

    if (got > 0) {
        ESP_LOGI(TAG, "FC01 RX got=%u expected=%u", (unsigned)got, (unsigned)expected);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, response, got, ESP_LOG_INFO);
    } else {
        ESP_LOGW(TAG, "FC01 RX got=0 expected=%u", (unsigned)expected);
    }

    if (got != expected) { status_error(true, false); return ESP_ERR_TIMEOUT; }
    uint16_t response_crc = (uint16_t)response[expected - 2] | ((uint16_t)response[expected - 1] << 8);
    if (modbus_crc16(response, expected - 2) != response_crc) {
        status_error(false, true); return ESP_ERR_INVALID_CRC;
    }
    if (response[0] != slave || response[1] != 0x01 || response[2] != byte_count) {
        status_error(false, false); return ESP_ERR_INVALID_RESPONSE;
    }
    for (uint16_t i = 0; i < coil_count; ++i) out_coils[i] = (response[3 + i / 8] & (1U << (i % 8))) != 0;
    status_frame_rx();
    return ESP_OK;
}

esp_err_t modbus_service_write_single_coil(uint8_t slave, uint16_t coil, bool on)
{
    if (!s_bus_lock || !s_status_lock || !slave) return ESP_ERR_INVALID_STATE;
    uint16_t value = on ? 0xFF00 : 0x0000;
    uint8_t request[8] = {
        slave, 0x05,
        (uint8_t)(coil >> 8), (uint8_t)(coil & 0xFF),
        (uint8_t)(value >> 8), (uint8_t)(value & 0xFF), 0, 0
    };
    uint16_t crc = modbus_crc16(request, 6);
    request[6] = (uint8_t)(crc & 0xFF); request[7] = (uint8_t)(crc >> 8);
    uint8_t response[8] = {0};

    ESP_LOGI(TAG, "FC05 TX slave=%u coil=0x%04X value=%s",
             (unsigned)slave, (unsigned)coil, on ? "ON" : "OFF");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, request, sizeof(request), ESP_LOG_INFO);

    xSemaphoreTake(s_bus_lock, portMAX_DELAY);
    uart_flush_input(MODBUS_UART);
    int written = uart_write_bytes(MODBUS_UART, request, sizeof(request));
    if (written != (int)sizeof(request)) {
        xSemaphoreGive(s_bus_lock); status_error(false, false); return ESP_FAIL;
    }
    status_frame_tx();
    esp_err_t wait_err = uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));
    if (wait_err != ESP_OK) {
        xSemaphoreGive(s_bus_lock); status_error(true, false); return wait_err;
    }
    int got = uart_read_bytes(MODBUS_UART, response, sizeof(response), pdMS_TO_TICKS(MODBUS_TIMEOUT_MS));
    xSemaphoreGive(s_bus_lock);

    if (got > 0) {
        ESP_LOGI(TAG, "FC05 RX got=%d expected=%u", got, (unsigned)sizeof(response));
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, response, got, ESP_LOG_INFO);
    } else {
        ESP_LOGW(TAG, "FC05 RX got=0 expected=%u", (unsigned)sizeof(response));
    }

    if (got != (int)sizeof(response)) { status_error(true, false); return ESP_ERR_TIMEOUT; }
    uint16_t response_crc = (uint16_t)response[6] | ((uint16_t)response[7] << 8);
    if (modbus_crc16(response, 6) != response_crc) { status_error(false, true); return ESP_ERR_INVALID_CRC; }
    if (memcmp(request, response, 6) != 0) { status_error(false, false); return ESP_ERR_INVALID_RESPONSE; }
    status_frame_rx();
    return ESP_OK;
}

esp_err_t modbus_service_ma01_refresh(void)
{
    uint8_t slave = modbus_service_ma01_get_slave();
    if (!slave) return ESP_ERR_INVALID_STATE;

    bool coils[MODBUS_MA01_COILS] = {0};
    esp_err_t err = modbus_service_read_coils(slave, 0, MODBUS_MA01_COILS,
                                              coils, MODBUS_MA01_COILS);
    if (err == ESP_OK) {
        xSemaphoreTake(s_status_lock, portMAX_DELAY);
        memcpy(s_ma01_coils, coils, sizeof(s_ma01_coils));
        s_ma01_online = true;
        xSemaphoreGive(s_status_lock);
    } else {
        xSemaphoreTake(s_status_lock, portMAX_DELAY);
        s_ma01_online = false;
        xSemaphoreGive(s_status_lock);
    }
    return err;
}

esp_err_t modbus_service_ma01_set(uint8_t channel, bool on)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;
    uint8_t slave = modbus_service_ma01_get_slave();
    if (!slave) return ESP_ERR_INVALID_STATE;

    esp_err_t err = ma01_ensure_config();
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    uint16_t mode = s_ma01_modes[channel - 1];
    uint16_t pulse_ms = s_ma01_pulse_ms[channel - 1];
    xSemaphoreGive(s_status_lock);

    if (mode == MODBUS_MA01_MODE_FOLLOW) return ESP_ERR_NOT_SUPPORTED;

    err = modbus_service_write_single_coil(slave,
                                           (uint16_t)(channel - 1),
                                           on);
    if (err != ESP_OK) return err;

    if (mode == MODBUS_MA01_MODE_PULSE && on) {
        xSemaphoreTake(s_status_lock, portMAX_DELAY);
        s_ma01_coils[channel - 1] = true;
        s_ma01_online = true;
        xSemaphoreGive(s_status_lock);
        ma01_schedule_refresh((uint32_t)pulse_ms + 150U);
        return ESP_OK;
    }

    return modbus_service_ma01_refresh();
}

esp_err_t modbus_service_ma01_toggle(uint8_t channel)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ma01_ensure_config();
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    uint16_t mode = s_ma01_modes[channel - 1];
    xSemaphoreGive(s_status_lock);
    if (mode != MODBUS_MA01_MODE_LEVEL) return ESP_ERR_NOT_SUPPORTED;

    err = modbus_service_ma01_refresh();
    if (err != ESP_OK) return err;

    bool next;
    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    next = !s_ma01_coils[channel - 1];
    xSemaphoreGive(s_status_lock);

    return modbus_service_ma01_set(channel, next);
}

esp_err_t modbus_service_ma01_action(uint8_t channel)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ma01_ensure_config();
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    uint16_t mode = s_ma01_modes[channel - 1];
    xSemaphoreGive(s_status_lock);

    if (mode == MODBUS_MA01_MODE_LEVEL) return modbus_service_ma01_toggle(channel);
    if (mode == MODBUS_MA01_MODE_PULSE) return modbus_service_ma01_set(channel, true);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t modbus_service_ma01_set_mode(uint8_t channel, modbus_ma01_mode_t mode)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;
    if (mode < MODBUS_MA01_MODE_LEVEL || mode > MODBUS_MA01_MODE_FOLLOW) return ESP_ERR_INVALID_ARG;

    uint8_t slave = modbus_service_ma01_get_slave();
    if (!slave) return ESP_ERR_INVALID_STATE;

    esp_err_t err = modbus_service_write_single_register(
        slave,
        (uint16_t)(MODBUS_MA01_MODE_BASE + channel - 1),
        (uint16_t)mode);
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_ma01_modes[channel - 1] = (uint16_t)mode;
    s_ma01_config_valid = true;
    xSemaphoreGive(s_status_lock);
    return ESP_OK;
}

esp_err_t modbus_service_ma01_cycle_mode(uint8_t channel)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ma01_ensure_config();
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    uint16_t current = s_ma01_modes[channel - 1];
    xSemaphoreGive(s_status_lock);

    modbus_ma01_mode_t next = (modbus_ma01_mode_t)((current + 1U) % 3U);
    return modbus_service_ma01_set_mode(channel, next);
}

esp_err_t modbus_service_ma01_set_pulse_ms(uint8_t channel, uint16_t pulse_ms)
{
    if (channel < 1 || channel > MODBUS_MA01_COILS) return ESP_ERR_INVALID_ARG;

    uint8_t slave = modbus_service_ma01_get_slave();
    if (!slave) return ESP_ERR_INVALID_STATE;

    esp_err_t err = modbus_service_write_single_register(
        slave,
        (uint16_t)(MODBUS_MA01_PULSE_BASE + channel - 1),
        pulse_ms);
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_status_lock, portMAX_DELAY);
    s_ma01_pulse_ms[channel - 1] = pulse_ms;
    s_ma01_config_valid = true;
    xSemaphoreGive(s_status_lock);
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
    ma01_load_slave();

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
    if (s_ma01_slave) {
        ESP_LOGI(TAG, "MA01 configured slave=%u (NVS)", (unsigned)s_ma01_slave);
    } else {
        ESP_LOGW(TAG, "MA01 slave not configured; use MA01 SCAN or MA01 ADDR <1..247>");
    }
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
    } else if (strcmp(binding, "modbus.ma01.state") == 0) {
        if (!s_ma01_slave) strlcpy(out, "NO ADDR", out_len);
        else strlcpy(out, s_ma01_online ? "ONLINE" : "NOT READ", out_len);
    } else if (strcmp(binding, "modbus.ma01.address") == 0) {
        if (s_ma01_slave) snprintf(out, out_len, "%u", (unsigned)s_ma01_slave);
        else strlcpy(out, "--", out_len);
    } else if (strncmp(binding, "modbus.ma01.summary", 20) == 0 &&
               binding[20] >= '1' && binding[20] <= '8' && binding[21] == '\0') {
        unsigned channel = (unsigned)(binding[20] - '1');
        const char *state = s_ma01_online ? (s_ma01_coils[channel] ? "ON" : "OFF") : "--";
        const char *mode = "?";
        if (s_ma01_config_valid) {
            if (s_ma01_modes[channel] == MODBUS_MA01_MODE_LEVEL) mode = "LEVEL";
            else if (s_ma01_modes[channel] == MODBUS_MA01_MODE_PULSE) mode = "PULSE";
            else if (s_ma01_modes[channel] == MODBUS_MA01_MODE_FOLLOW) mode = "FOLLOW";
        }
        snprintf(out, out_len, "%s | %s", state, mode);
    } else if (strncmp(binding, "modbus.ma01.do", 14) == 0 &&
               binding[14] >= '1' && binding[14] <= '8' && binding[15] == '\0') {
        unsigned channel = (unsigned)(binding[14] - '1');
        strlcpy(out, s_ma01_online ? (s_ma01_coils[channel] ? "ON" : "OFF") : "--", out_len);
    } else {
        strlcpy(out, "unsupported", out_len);
    }
}
