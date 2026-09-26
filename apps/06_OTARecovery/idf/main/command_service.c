#include "command_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modbus_service.h"

static void trim(char *s)
{
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static void upper(char *s)
{
    for (; s && *s; ++s) *s = (char)toupper((unsigned char)*s);
}

esp_err_t command_service_execute(const char *command, char *response, size_t response_len)
{
    if (!command || !response || response_len == 0) return ESP_ERR_INVALID_ARG;

    char line[128];
    strlcpy(line, command, sizeof(line));
    trim(line);
    upper(line);

    if (!line[0]) {
        strlcpy(response, "ERR empty command", response_len);
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(line, "HELP") == 0) {
        strlcpy(response,
                "OK commands: HELP | MA01 ADDR [1..247] | MA01 SCAN | MA01 INFO | MA01 READ | MA01 CONFIG | MA01 DO<n> ACTION|ON|OFF|TOGGLE|MODE <LEVEL|PULSE|FOLLOW>|PULSEMS <0..65535>",
                response_len);
        return ESP_OK;
    }

    if (strcmp(line, "MA01 ADDR") == 0) {
        uint8_t slave = modbus_service_ma01_get_slave();
        if (slave) snprintf(response, response_len, "OK MA01 ADDR=%u", (unsigned)slave);
        else strlcpy(response, "OK MA01 ADDR=NOT_SET", response_len);
        return ESP_OK;
    }

    if (strncmp(line, "MA01 ADDR ", 10) == 0) {
        char *end = NULL;
        long slave = strtol(line + 10, &end, 10);
        if (!end || *end != '\0' || slave < 1 || slave > 247) {
            strlcpy(response, "ERR syntax: MA01 ADDR <1..247>", response_len);
            return ESP_ERR_INVALID_ARG;
        }
        esp_err_t err = modbus_service_ma01_set_slave((uint8_t)slave);
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 ADDR %s", esp_err_to_name(err));
            return err;
        }
        snprintf(response, response_len, "OK MA01 ADDR=%ld SAVED", slave);
        return ESP_OK;
    }

    if (strcmp(line, "MA01 SCAN") == 0) {
        uint8_t slave = 0;
        uint16_t model = 0, fw = 0;
        esp_err_t err = modbus_service_ma01_scan(&slave, &model, &fw);
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 SCAN %s", esp_err_to_name(err));
            return err;
        }
        snprintf(response, response_len,
                 "OK MA01 FOUND ADDR=%u MODEL=0x%04X FW=0x%04X SAVED",
                 (unsigned)slave, (unsigned)model, (unsigned)fw);
        return ESP_OK;
    }

    if (strcmp(line, "MA01 INFO") == 0) {
        uint8_t slave = modbus_service_ma01_get_slave();
        if (!slave) {
            strlcpy(response, "ERR MA01 address not configured; use MA01 SCAN or MA01 ADDR <1..247>", response_len);
            return ESP_ERR_INVALID_STATE;
        }

        uint16_t model = 0, fw = 0;
        esp_err_t err = modbus_service_read_registers(slave, 0x03, 0x07D0, 1, &model, 1);
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 INFO model %s", esp_err_to_name(err));
            return err;
        }
        err = modbus_service_read_registers(slave, 0x03, 0x07DC, 1, &fw, 1);
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 INFO firmware %s", esp_err_to_name(err));
            return err;
        }
        snprintf(response, response_len, "OK MA01 ADDR=%u MODEL=0x%04X FW=0x%04X",
                 (unsigned)slave, (unsigned)model, (unsigned)fw);
        return ESP_OK;
    }

    if (strcmp(line, "MA01 READ") == 0 || strcmp(line, "MA01 STATUS") == 0) {
        esp_err_t err = modbus_service_ma01_refresh();
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 READ %s", esp_err_to_name(err));
            return err;
        }

        char state[24];
        modbus_service_format_binding("modbus.ma01.state", state, sizeof(state));
        size_t used = (size_t)snprintf(response, response_len, "OK MA01 %s", state);
        for (unsigned i = 1; i <= 8 && used < response_len; ++i) {
            char binding[24], value[8];
            snprintf(binding, sizeof(binding), "modbus.ma01.do%u", i);
            modbus_service_format_binding(binding, value, sizeof(value));
            used += (size_t)snprintf(response + used, response_len - used, " DO%u=%s", i, value);
        }
        return ESP_OK;
    }

    if (strcmp(line, "MA01 CONFIG") == 0) {
        esp_err_t err = modbus_service_ma01_refresh_config();
        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 CONFIG %s", esp_err_to_name(err));
            return err;
        }
        (void)modbus_service_ma01_refresh();

        size_t used = (size_t)snprintf(response, response_len, "OK MA01 CONFIG");
        for (unsigned i = 1; i <= 8 && used < response_len; ++i) {
            char binding[32], value[24];
            snprintf(binding, sizeof(binding), "modbus.ma01.summary%u", i);
            modbus_service_format_binding(binding, value, sizeof(value));
            used += (size_t)snprintf(response + used, response_len - used, " DO%u=%s", i, value);
        }
        return ESP_OK;
    }

    if (strncmp(line, "MA01 DO", 7) == 0) {
        const char *p = line + 7;
        char *end = NULL;
        long channel = strtol(p, &end, 10);
        if (channel < 1 || channel > 8 || !end || *end != ' ') {
            strlcpy(response, "ERR syntax: MA01 DO<n> ACTION|ON|OFF|TOGGLE|MODE <LEVEL|PULSE|FOLLOW>|PULSEMS <0..65535>", response_len);
            return ESP_ERR_INVALID_ARG;
        }
        const char *op = end + 1;
        esp_err_t err;

        if (strcmp(op, "ACTION") == 0) {
            err = modbus_service_ma01_action((uint8_t)channel);
            if (err == ESP_OK) {
                snprintf(response, response_len, "OK MA01 DO%ld ACTION", channel);
                return ESP_OK;
            }
        } else if (strcmp(op, "ON") == 0) {
            err = modbus_service_ma01_set((uint8_t)channel, true);
        } else if (strcmp(op, "OFF") == 0) {
            err = modbus_service_ma01_set((uint8_t)channel, false);
        } else if (strcmp(op, "TOGGLE") == 0) {
            err = modbus_service_ma01_toggle((uint8_t)channel);
        } else if (strncmp(op, "MODE ", 5) == 0) {
            const char *mode_text = op + 5;
            modbus_ma01_mode_t mode;
            if (strcmp(mode_text, "LEVEL") == 0) mode = MODBUS_MA01_MODE_LEVEL;
            else if (strcmp(mode_text, "PULSE") == 0) mode = MODBUS_MA01_MODE_PULSE;
            else if (strcmp(mode_text, "FOLLOW") == 0) mode = MODBUS_MA01_MODE_FOLLOW;
            else {
                strlcpy(response, "ERR syntax: MA01 DO<n> MODE LEVEL|PULSE|FOLLOW", response_len);
                return ESP_ERR_INVALID_ARG;
            }
            err = modbus_service_ma01_set_mode((uint8_t)channel, mode);
            if (err == ESP_OK) {
                snprintf(response, response_len, "OK MA01 DO%ld MODE=%s", channel, mode_text);
                return ESP_OK;
            }
        } else if (strncmp(op, "PULSEMS ", 8) == 0) {
            char *pulse_end = NULL;
            long pulse_ms = strtol(op + 8, &pulse_end, 10);
            if (!pulse_end || *pulse_end != '\0' || pulse_ms < 0 || pulse_ms > 65535) {
                strlcpy(response, "ERR syntax: MA01 DO<n> PULSEMS <0..65535>", response_len);
                return ESP_ERR_INVALID_ARG;
            }
            err = modbus_service_ma01_set_pulse_ms((uint8_t)channel, (uint16_t)pulse_ms);
            if (err == ESP_OK) {
                snprintf(response, response_len, "OK MA01 DO%ld PULSEMS=%ld", channel, pulse_ms);
                return ESP_OK;
            }
        } else {
            strlcpy(response, "ERR syntax: MA01 DO<n> ACTION|ON|OFF|TOGGLE|MODE <LEVEL|PULSE|FOLLOW>|PULSEMS <0..65535>", response_len);
            return ESP_ERR_INVALID_ARG;
        }

        if (err != ESP_OK) {
            snprintf(response, response_len, "ERR MA01 DO%ld %s", channel, esp_err_to_name(err));
            return err;
        }

        char binding[24], value[8];
        snprintf(binding, sizeof(binding), "modbus.ma01.do%ld", channel);
        modbus_service_format_binding(binding, value, sizeof(value));
        snprintf(response, response_len, "OK MA01 DO%ld=%s", channel, value);
        return ESP_OK;
    }

    strlcpy(response, "ERR unknown command; type HELP", response_len);
    return ESP_ERR_NOT_SUPPORTED;
}
