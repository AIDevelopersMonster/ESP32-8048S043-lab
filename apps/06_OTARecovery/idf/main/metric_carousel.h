#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"
#include "widget_runtime.h"

typedef struct metric_carousel_view metric_carousel_view_t;

typedef void (*metric_carousel_value_cb_t)(const char *binding, char *out, size_t out_len, void *user_data);

metric_carousel_view_t *metric_carousel_create(lv_obj_t *parent,
                                                int x, int y, int w, int h,
                                                const char *title,
                                                int interval_ms,
                                                const widget_carousel_item_t *items,
                                                size_t item_count,
                                                metric_carousel_value_cb_t value_cb,
                                                void *user_data);
void metric_carousel_update(metric_carousel_view_t *view);
void metric_carousel_destroy(metric_carousel_view_t *view);
