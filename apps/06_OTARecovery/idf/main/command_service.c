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
                "OK commands: HELP | MA01 READ | MA01 STATUS | MA01 DO<n> ON|OFF|TOGGLE",
                response_len);
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

    if (strncmp(line, "MA01 DO", 7) == 0) {
        const char *p = line + 7;
        char *end = NULL;
        long channel = strtol(p, &end, 10);
        if (channel < 1 || channel > 8 || !end || *end != ' ') {
            strlcpy(response, "ERR syntax: MA01 DO<n> ON|OFF|TOGGLE", response_len);
            return ESP_ERR_INVALID_ARG;
        }
        const char *op = end + 1;
        esp_err_t err;
        if (strcmp(op, "ON") == 0) {
            err = modbus_service_ma01_set((uint8_t)channel, true);
        } else if (strcmp(op, "OFF") == 0) {
            err = modbus_service_ma01_set((uint8_t)channel, false);
        } else if (strcmp(op, "TOGGLE") == 0) {
            err = modbus_service_ma01_toggle((uint8_t)channel);
        } else {
            strlcpy(response, "ERR syntax: MA01 DO<n> ON|OFF|TOGGLE", response_len);
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
