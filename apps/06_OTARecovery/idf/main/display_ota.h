#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t display_ota_start(void);

bool display_ota_is_ready(void);
