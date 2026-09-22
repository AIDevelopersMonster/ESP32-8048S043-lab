#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"
#include "sd_manager.h"

typedef struct sd_launcher_view sd_launcher_view_t;
typedef void (*sd_launcher_run_cb_t)(const sd_manager_entry_t *entry, void *user_data);

/*
 * Generic App09 launcher. It knows only the SD package catalog contract;
 * application names and entrypoints come exclusively from package.json.
 */
sd_launcher_view_t *sd_launcher_create(lv_obj_t *parent,
                                       int32_t x, int32_t y,
                                       int32_t w, int32_t h,
                                       sd_launcher_run_cb_t run_cb,
                                       void *user_data);

/* Rebuild visible launch buttons from the current sd_manager catalog. */
esp_err_t sd_launcher_refresh(sd_launcher_view_t *view);

void sd_launcher_destroy(sd_launcher_view_t *view);
