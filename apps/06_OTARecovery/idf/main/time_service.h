#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "esp_err.h"

typedef struct {
    bool synchronized;
    bool syncing;
    bool ntp_initialized;
    time_t last_sync_epoch;
    char state[24];
    char timezone[24];
} time_service_status_t;

esp_err_t time_service_init(void);
esp_err_t time_service_request_sync(void);
bool time_service_get_local_time(struct tm *out);
void time_service_get_status(time_service_status_t *out);
void time_service_format_binding(const char *binding, char *out, size_t out_len);
