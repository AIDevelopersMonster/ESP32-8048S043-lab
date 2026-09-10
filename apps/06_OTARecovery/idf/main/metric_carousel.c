#include "metric_carousel.h"

#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"

struct metric_carousel_view {
    lv_obj_t *root;
    lv_obj_t *title;
    lv_obj_t *value;
    lv_obj_t *label;
    int interval_ms;
    int64_t last_switch_us;
    size_t index;
    size_t item_count;
    widget_carousel_item_t items[WIDGET_MAX_CAROUSEL_ITEMS];
    metric_carousel_value_cb_t value_cb;
    void *user_data;
};

static void render(metric_carousel_view_t *view)
{
    if (!view || !view->item_count) return;
    widget_carousel_item_t *item = &view->items[view->index];
    char value[96] = {0};
    view->value_cb(item->binding, value, sizeof(value), view->user_data);
    lv_label_set_text(view->value, value[0] ? value : "--");
    lv_label_set_text(view->label, item->label);
    lv_obj_set_style_text_color(view->value, lv_color_hex(item->color), 0);
    lv_obj_set_style_text_color(view->label, lv_color_hex(item->color), 0);
}

metric_carousel_view_t *metric_carousel_create(lv_obj_t *parent,
                                                int x, int y, int w, int h,
                                                const char *title,
                                                int interval_ms,
                                                const widget_carousel_item_t *items,
                                                size_t item_count,
                                                metric_carousel_value_cb_t value_cb,
                                                void *user_data)
{
    if (!parent || !items || item_count < 2 || item_count > WIDGET_MAX_CAROUSEL_ITEMS || !value_cb) return NULL;

    metric_carousel_view_t *view = calloc(1, sizeof(*view));
    if (!view) return NULL;
    view->interval_ms = interval_ms;
    view->item_count = item_count;
    view->value_cb = value_cb;
    view->user_data = user_data;
    memcpy(view->items, items, item_count * sizeof(items[0]));

    view->root = lv_obj_create(parent);
    if (!view->root) { free(view); return NULL; }
    lv_obj_set_pos(view->root, x, y);
    lv_obj_set_size(view->root, w, h);
    lv_obj_set_style_pad_all(view->root, 0, 0);
    lv_obj_set_style_border_width(view->root, 0, 0);
    lv_obj_set_style_radius(view->root, 18, 0);
    lv_obj_set_style_bg_color(view->root, lv_color_hex(0x05070B), 0);
    lv_obj_set_style_bg_opa(view->root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(view->root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    view->title = lv_label_create(view->root);
    lv_label_set_text(view->title, title && title[0] ? title : "LIVE");
    lv_obj_set_style_text_font(view->title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(view->title, lv_color_hex(0x8B949E), 0);
    lv_obj_align(view->title, LV_ALIGN_TOP_MID, 0, 18);

    view->value = lv_label_create(view->root);
    lv_obj_set_width(view->value, w - 40);
    lv_obj_set_style_text_font(view->value, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(view->value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(view->value, LV_ALIGN_CENTER, 0, -6);

    view->label = lv_label_create(view->root);
    lv_obj_set_width(view->label, w - 40);
    lv_obj_set_style_text_font(view->label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(view->label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(view->label, LV_ALIGN_BOTTOM_MID, 0, -26);

    view->last_switch_us = esp_timer_get_time();
    render(view);
    return view;
}

void metric_carousel_update(metric_carousel_view_t *view)
{
    if (!view) return;
    int64_t now = esp_timer_get_time();
    int64_t interval_us = (int64_t)view->interval_ms * 1000LL;
    if (now - view->last_switch_us >= interval_us) {
        view->index = (view->index + 1U) % view->item_count;
        view->last_switch_us = now;
    }
    render(view);
}

void metric_carousel_destroy(metric_carousel_view_t *view)
{
    if (!view) return;
    if (view->root) lv_obj_delete(view->root);
    free(view);
}
