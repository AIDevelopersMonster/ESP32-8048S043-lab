#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

esp_err_t storage_credentials_init(void);

bool storage_credentials_load(
    char *ssid,
    size_t ssid_len,
    char *password,
    size_t password_len
);

esp_err_t storage_credentials_save(const char *ssid, const char *password);
esp_err_t storage_credentials_clear(void);
