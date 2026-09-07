#pragma once

#include <stdint.h>

#include "lvgl.h"

typedef struct sevenseg_clock_view sevenseg_clock_view_t;

sevenseg_clock_view_t *sevenseg_clock_create(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color);
void sevenseg_clock_update(sevenseg_clock_view_t *view);
void sevenseg_clock_destroy(sevenseg_clock_view_t *view);
