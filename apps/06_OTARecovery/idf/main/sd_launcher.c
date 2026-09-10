#include "sd_launcher.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#define TAG "APP09_LAUNCHER"

struct sd_launcher_view {
    lv_obj_t *container;
    lv_obj_t *empty_label;
    sd_launcher_run_cb_t run_cb;
    void *user_data;
};

static void entry_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    sd_launcher_view_t *view = (sd_launcher_view_t *)lv_event_get_user_data(e);
    lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);
    if (!view || !button || !view->run_cb) return;

    uintptr_t encoded = (uintptr_t)lv_obj_get_user_data(button);
    if (encoded == 0) return;
    size_t index = (size_t)(encoded - 1U);

    sd_manager_entry_t entry;
    esp_err_t err = sd_manager_entry_get(index, &entry);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "catalog entry %u unavailable: %s", (unsigned)index, esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "launch %s / %s -> %s", entry.package_name, entry.entry_name, entry.relative_path);
    view->run_cb(&entry, view->user_data);
}

sd_launcher_view_t *sd_launcher_create(lv_obj_t *parent,
                                       int32_t x, int32_t y,
                                       int32_t w, int32_t h,
                                       sd_launcher_run_cb_t run_cb,
                                       void *user_data)
{
    if (!parent || !run_cb) return NULL;

    sd_launcher_view_t *view = calloc(1, sizeof(*view));
    if (!view) return NULL;

    view->run_cb = run_cb;
    view->user_data = user_data;
    view->container = lv_obj_create(parent);
    if (!view->container) {
        free(view);
        return NULL;
    }

    lv_obj_set_pos(view->container, x, y);
    lv_obj_set_size(view->container, w, h);
    lv_obj_set_style_pad_all(view->container, 8, 0);
    lv_obj_set_style_pad_row(view->container, 8, 0);
    lv_obj_set_style_border_width(view->container, 1, 0);
    lv_obj_set_flex_flow(view->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(view->container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(view->container, LV_SCROLLBAR_MODE_AUTO);

    return view;
}

esp_err_t sd_launcher_refresh(sd_launcher_view_t *view)
{
    if (!view || !view->container) return ESP_ERR_INVALID_ARG;

    lv_obj_clean(view->container);
    view->empty_label = NULL;

    size_t count = sd_manager_entry_count();
    if (count == 0) {
        view->empty_label = lv_label_create(view->container);
        lv_label_set_text(view->empty_label, "No applications found in /sd/widgets");
        lv_obj_set_width(view->empty_label, lv_pct(100));
        return ESP_OK;
    }

    for (size_t i = 0; i < count; ++i) {
        sd_manager_entry_t entry;
        esp_err_t err = sd_manager_entry_get(i, &entry);
        if (err != ESP_OK) continue;

        lv_obj_t *button = lv_button_create(view->container);
        if (!button) return ESP_ERR_NO_MEM;
        lv_obj_set_width(button, lv_pct(100));
        lv_obj_set_height(button, 54);
        lv_obj_set_flex_grow(button, 0);
        lv_obj_set_user_data(button, (void *)(uintptr_t)(i + 1U));
        lv_obj_add_event_cb(button, entry_button_cb, LV_EVENT_CLICKED, view);

        lv_obj_t *label = lv_label_create(button);
        if (!label) return ESP_ERR_NO_MEM;
        if (strcmp(entry.package_name, entry.entry_name) == 0) {
            lv_label_set_text(label, entry.entry_name);
        } else {
            lv_label_set_text_fmt(label, "%s  |  %s", entry.package_name, entry.entry_name);
        }
        lv_obj_set_width(label, lv_pct(92));
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_obj_center(label);
    }

    ESP_LOGI(TAG, "launcher rebuilt from SD catalog entries=%u", (unsigned)count);
    return ESP_OK;
}

void sd_launcher_destroy(sd_launcher_view_t *view)
{
    if (!view) return;
    if (view->container) lv_obj_delete(view->container);
    free(view);
}
