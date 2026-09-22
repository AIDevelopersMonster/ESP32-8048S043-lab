#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "esp_err.h"

#define YOUTUBE_HISTORY_MAX 366
#define YOUTUBE_CHART_MAX_POINTS 366

typedef enum {
    YOUTUBE_PERIOD_7D = 7,
    YOUTUBE_PERIOD_30D = 30,
    YOUTUBE_PERIOD_90D = 90,
    YOUTUBE_PERIOD_ALL = 0,
} youtube_period_t;

typedef struct {
    bool configured;
    bool busy;
    bool has_data;
    char channel_id[64];
    char channel_title[96];
    char state[32];
    char message[128];
    uint64_t subscribers;
    uint64_t views;
    uint64_t videos;
    int64_t period_subscribers_delta;
    uint64_t period_views_delta;
    youtube_period_t period;
    size_t history_count;
    time_t last_update;
} youtube_status_t;

esp_err_t youtube_service_init(void);
esp_err_t youtube_service_start(void);
esp_err_t youtube_service_save_config(const char *channel_id, const char *api_key);
esp_err_t youtube_service_clear_config(void);
esp_err_t youtube_service_request_refresh(void);
void youtube_service_set_period(youtube_period_t period);
void youtube_service_get_status(youtube_status_t *out);
void youtube_service_format_binding(const char *binding, char *out, size_t out_len);
size_t youtube_service_get_chart(const char *binding, int32_t *values, size_t max_values,
                                 int32_t *min_out, int32_t *max_out);
