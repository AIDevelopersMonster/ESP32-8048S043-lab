#include "climate_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "modbus_service.h"
#include "nvs.h"

/* Bench default, not a compressor-specific protection interval. */
#define MIN_OFF_US (5000000LL)
#define SENSOR_GRACE_US (60000000LL)
static SemaphoreHandle_t s_lock;
static climate_config_t s_config;
static bool s_enabled, s_claimed, s_known, s_fault;
static bool s_demand[4], s_actual[4];
static int64_t s_off_since[4];
static int64_t s_last_fresh_us = -1;
static const char *s_state = "OFF";
static esp_err_t s_error;
static const char *s_edit_state = "READY";

esp_err_t climate_service_init(void)
{
    if (s_lock) return ESP_OK;
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    s_config = climate_defaults();
    nvs_handle_t h;
    esp_err_t err = nvs_open("climate", NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    /* Explicit versioned integer record, no compiler-dependent struct padding. */
    int16_t record[7] = {0};
    size_t size = sizeof(record);
    err = nvs_get_blob(h, "config", record, &size);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) return err;
    climate_config_t c = {record[1], record[2], record[3], record[4], record[5], record[6]};
    if (size != sizeof(record) || record[0] != 1 || !climate_config_valid(&c))
        return ESP_ERR_INVALID_STATE;
    s_config = c;
    return ESP_OK;
}

bool climate_service_requires_sensor(void)
{
    if (!s_lock) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool required = s_enabled || s_claimed;
    xSemaphoreGive(s_lock);
    return required;
}

void climate_service_get_config(climate_config_t *out)
{
    if (!out || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_config;
    xSemaphoreGive(s_lock);
}

esp_err_t climate_service_set_config(const climate_config_t *c)
{
    if (!climate_config_valid(c)) return ESP_ERR_INVALID_ARG;
    if (!s_lock) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    /* Current cycle and threshold edit share the service mutex. The next poll
     * evaluates the new configuration and applies break-before-make. */
    esp_err_t err;
    nvs_handle_t h;
    err = nvs_open("climate", NVS_READWRITE, &h);
    if (err != ESP_OK) { s_edit_state = "SAVE ERROR"; goto done; }
    int16_t record[7] = {1, c->t_min, c->t_max, c->t_hyst, c->rh_min, c->rh_max, c->rh_hyst};
    err = nvs_set_blob(h, "config", record, sizeof(record));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) s_config = *c;
    s_edit_state = err == ESP_OK ? "SAVED" : "SAVE ERROR";
done:
    xSemaphoreGive(s_lock);
    return err;
}

esp_err_t climate_service_adjust(unsigned index, int delta)
{
    if (!s_lock) return ESP_ERR_INVALID_STATE;
    if (index >= 6 || (delta != 1 && delta != -1 && delta != 5 &&
                        delta != -5 && delta != 10 && delta != -10))
        return ESP_ERR_INVALID_ARG;
    climate_config_t c;
    climate_service_get_config(&c);
    int16_t *fields[] = {&c.t_min, &c.t_max, &c.t_hyst,
                         &c.rh_min, &c.rh_max, &c.rh_hyst};
    int32_t next = (int32_t)*fields[index] + delta;
    if (next < INT16_MIN || next > INT16_MAX) return ESP_ERR_INVALID_ARG;
    *fields[index] = (int16_t)next;
    if (!climate_config_valid(&c)) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_edit_state = "LIMIT REACHED";
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_ARG;
    }
    return climate_service_set_config(&c);
}

esp_err_t climate_service_enable(bool enabled)
{
    if (!s_lock) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (enabled && s_fault) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    if (!enabled) s_fault = false;
    s_enabled = enabled;
    s_state = enabled ? "STARTING" : (s_claimed ? "STOPPING" : "OFF");
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

/* Try ALL channels even after an error. A timeout means UNKNOWN, not OFF. */
static esp_err_t all_off(void)
{
    esp_err_t result = ESP_OK;
    uint8_t slave = modbus_service_ma01_get_slave();
    for (unsigned i = 0; i < 4; ++i) {
        esp_err_t err = modbus_service_write_single_coil(slave, i, false);
        if (err != ESP_OK) result = err;
    }
    bool coils[4] = {0};
    esp_err_t err = modbus_service_climate_snapshot(coils, false);
    if (err != ESP_OK) result = err;
    for (unsigned i = 0; i < 4; ++i) if (coils[i]) result = ESP_ERR_INVALID_RESPONSE;
    s_known = result == ESP_OK;
    if (s_known) {
        memset(s_actual, 0, sizeof(s_actual));
        for (unsigned i = 0; i < 4; ++i) s_off_since[i] = esp_timer_get_time();
    }
    memset(s_demand, 0, sizeof(s_demand));
    return result;
}

void climate_service_poll(void)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_enabled && !s_claimed) goto done;
    if (!s_claimed) {
        s_error = modbus_service_climate_claim(true);
        if (s_error != ESP_OK) { s_state = "FAULT"; goto done; }
        s_claimed = true;
        s_known = false;
    }
    modbus_service_status_t sensor = {0};
    modbus_service_get_status(&sensor);
    bool valid = sensor.sensor_online && sensor.temperature_tenths_c >= -400 &&
        sensor.temperature_tenths_c <= 1250 && sensor.humidity_tenths_rh <= 1000;
    if (valid) s_last_fresh_us = esp_timer_get_time();
    /* Hold confirmed outputs during a brief sensor outage, without evaluating
     * stale values or issuing another relay command. A missing first reading
     * never receives this grace period. */
    if (s_enabled && !valid && s_known && !s_fault && s_last_fresh_us >= 0 &&
        esp_timer_get_time() - s_last_fresh_us < SENSOR_GRACE_US) {
        s_state = "SENSOR WAIT";
        goto done;
    }
    if (!s_enabled || !valid || !s_known || s_fault) {
        esp_err_t off_error = all_off();
        if (!s_fault || off_error != ESP_OK) s_error = off_error;
        s_state = (s_error != ESP_OK || s_fault) ? "FAULT" : !s_enabled ? "OFF" : !valid ? "FAILSAFE" : "STARTING";
        /* After confirmed OFF, keep the fault latched but stop retrying writes. */
        if (!s_enabled && off_error == ESP_OK) {
            (void)modbus_service_climate_claim(false);
            s_claimed = false;
        }
        goto done;
    }
    bool observed[4];
    s_error = modbus_service_climate_snapshot(observed, true);
    if (s_error != ESP_OK) goto fault;
    /* External changes are not silently adopted as legitimate hysteresis state. */
    if (memcmp(observed, s_actual, sizeof(observed)) != 0) {
        s_error = ESP_ERR_INVALID_STATE;
        goto fault;
    }
    climate_evaluate(&s_config, sensor.temperature_tenths_c, sensor.humidity_tenths_rh, s_demand);
    /* Break before make: every OFF is confirmed before any ON. */
    for (unsigned i = 0; i < 4; ++i) {
        if (s_actual[i] && !s_demand[i]) {
            s_error = modbus_service_ma01_set(i + 1, false);
            if (s_error != ESP_OK) goto fault;
            s_error = modbus_service_climate_snapshot(observed, true);
            if (s_error != ESP_OK || observed[i]) { s_error = ESP_ERR_INVALID_RESPONSE; goto fault; }
            s_actual[i] = false;
            s_off_since[i] = esp_timer_get_time();
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (!s_actual[i] && s_demand[i] &&
            esp_timer_get_time() - s_off_since[i] >= MIN_OFF_US &&
            esp_timer_get_time() - s_off_since[i ^ 1U] >= MIN_OFF_US) {
            if (s_actual[i ^ 1U]) { s_error = ESP_ERR_INVALID_STATE; goto fault; }
            s_error = modbus_service_ma01_set(i + 1, true);
            if (s_error != ESP_OK) goto fault;
            s_error = modbus_service_climate_snapshot(observed, true);
            if (s_error != ESP_OK || !observed[i] || observed[i ^ 1U]) {
                s_error = ESP_ERR_INVALID_RESPONSE; goto fault;
            }
            s_actual[i] = true;
        }
    }
    s_state = "AUTO";
    goto done;
fault:
    /* Latch faults until an explicit OFF/ON cycle; retain reservation on failed OFF. */
    s_enabled = false;
    s_fault = true;
    s_known = false;
    (void)all_off();
    s_state = "FAULT";
done:
    xSemaphoreGive(s_lock);
}

void climate_service_format_binding(const char *binding, char *out, size_t size)
{
    if (!out || !size) return;
    snprintf(out, size, "--");
    if (!binding || !s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!strcmp(binding, "climate.state")) snprintf(out, size, "%s", s_state);
    else if (!strcmp(binding, "climate.edit_state")) snprintf(out, size, "%s", s_edit_state);
    else if (!strcmp(binding, "climate.error")) snprintf(out, size, "%s", esp_err_to_name(s_error));
    else if (!strcmp(binding, "climate.sensor_state") ||
             !strcmp(binding, "climate.temperature") ||
             !strcmp(binding, "climate.humidity")) {
        modbus_service_status_t sensor = {0};
        modbus_service_get_status(&sensor);
        bool valid = sensor.sensor_seen && sensor.temperature_tenths_c >= -400 &&
            sensor.temperature_tenths_c <= 1250 && sensor.humidity_tenths_rh <= 1000;
        if (!strcmp(binding, "climate.sensor_state"))
            snprintf(out, size, "%s", !valid ? "NO SENSOR" :
                     sensor.sensor_online ? "ONLINE" : "STALE");
        else if (!valid) snprintf(out, size, "--");
        else if (!strcmp(binding, "climate.temperature")) {
            int v = sensor.temperature_tenths_c;
            snprintf(out, size, "%s%d.%d", v < 0 ? "-" : "", abs(v) / 10, abs(v) % 10);
        } else snprintf(out, size, "%u.%u", (unsigned)(sensor.humidity_tenths_rh / 10),
                      (unsigned)(sensor.humidity_tenths_rh % 10));
    }
    else {
        const char *keys[] = {"climate.t_min", "climate.t_max", "climate.t_hyst", "climate.rh_min", "climate.rh_max", "climate.rh_hyst"};
        int values[] = {s_config.t_min, s_config.t_max, s_config.t_hyst, s_config.rh_min, s_config.rh_max, s_config.rh_hyst};
        for (unsigned i = 0; i < 6; ++i) if (!strcmp(binding, keys[i])) {
            int v = values[i];
            snprintf(out, size, "%s%d.%d", v < 0 ? "-" : "", abs(v) / 10, abs(v) % 10);
        }
        const char *relays[] = {"climate.heater", "climate.cooler", "climate.humidifier", "climate.dehumidifier"};
        for (unsigned i = 0; i < 4; ++i) if (!strcmp(binding, relays[i]))
            snprintf(out, size, "%s", s_claimed && s_known ? (s_actual[i] ? "ON" : "OFF") : "--");
    }
    xSemaphoreGive(s_lock);
}

esp_err_t climate_service_command(const char *command, char *out, size_t size)
{
    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (!strcmp(command, "CLIMATE STATUS")) {
        char state[24];
        climate_service_format_binding("climate.state", state, sizeof(state));
        snprintf(out, size, "OK CLIMATE %s", state);
        return ESP_OK;
    }
    if (!strcmp(command, "CLIMATE ON")) err = climate_service_enable(true);
    else if (!strcmp(command, "CLIMATE OFF")) err = climate_service_enable(false);
    else if (!strcmp(command, "CLIMATE DEFAULTS")) {
        climate_config_t c = climate_defaults();
        err = climate_service_set_config(&c);
    } else if (!strncmp(command, "CLIMATE SET ", 12)) {
        /* Atomic six-value edit in integer tenths; strict trailing-token rejection. */
        long v[6];
        const char *p = command + 12;
        bool valid = true;
        for (unsigned i = 0; i < 6; ++i) {
            char *end;
            v[i] = strtol(p, &end, 10);
            if (end == p || v[i] < -32768 || v[i] > 32767 || (*end && *end != ' ')) { valid = false; break; }
            p = end;
            while (*p == ' ') ++p;
        }
        if (valid && !*p) {
            climate_config_t c = {v[0], v[1], v[2], v[3], v[4], v[5]};
            err = climate_service_set_config(&c);
        }
    }
    snprintf(out, size, "%s CLIMATE %s", err == ESP_OK ? "OK" : "ERR", esp_err_to_name(err));
    return err;
}
