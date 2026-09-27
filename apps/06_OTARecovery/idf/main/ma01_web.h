#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/*
 * Browser transport for the common command/service layer.
 * It must not contain MA01 register-map logic; all operations go through
 * command_service and the existing MA01 provider.
 */
esp_err_t ma01_web_register(httpd_handle_t server);
