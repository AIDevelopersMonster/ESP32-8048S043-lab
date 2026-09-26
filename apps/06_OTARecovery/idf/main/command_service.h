#pragma once

#include <stddef.h>
#include "esp_err.h"

esp_err_t command_service_execute(const char *command, char *response, size_t response_len);
