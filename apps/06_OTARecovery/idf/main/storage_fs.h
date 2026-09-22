#pragma once

#include <stddef.h>
#include "esp_err.h"

#define STORAGE_FS_BASE "/storage"

esp_err_t storage_fs_init(void);
esp_err_t storage_fs_info(size_t *total_bytes, size_t *used_bytes);
