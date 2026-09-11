#include "sd_launcher.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#define TAG "APP09_LAUNCHER"
#define CARD_W 338
#define CARD_H 82
#define CARD_GAP 10
#define ENTRY_H 58

struct sd_launcher_view {
    lv_obj_t *container;
    lv_obj_t *empty_label;
    sd_launcher_run_cb_t run_cb;
    void *user_data;
    char selected_package_id[SD_MANAGER_ENTRY_TEXT_MAX + 1];
    char selected_package_name[SD_MANAGER_ENTRY_TEXT_MAX + 1];
};

static void make_decorative(lv_obj_t *obj)
{
    if (obj) lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

static void style_card(lv_obj_t *button, int32_t w, int32_t h, uint32_t bg)
{
    lv_obj_set_size(button, w, h);
    lv_obj_set_flex_grow(button, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x30363D), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_pad_all(button, 0, 0);
}

static lv_obj_t *make_text(lv_obj_t *parent, int32_t x, int32_t y, int32_t w,
                           const char *text, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    if (!label) return NULL;
    lv_label_set_text(label, text ? text : "");
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, w);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    make_decorative(label);
    return label;
}

static bool package_is_first(size_t index, const sd_manager_entry_t *entry)
{
    if (!entry) return false;
    for (size_t i = 0; i < index; ++i) {
        sd_manager_entry_t previous;
        if (sd_manager_entry_get(i, &previous) != ESP_OK) continue;
        if (strcmp(previous.package_id, entry->package_id) == 0) return false;
    }
    return true;
}

static size_t package_entry_count(const char *package_id)
{
    if (!package_id || !package_id[0]) return 0;
    size_t matches = 0;
    size_t count = sd_manager_entry_count();
    for (size_t i = 0; i < count; ++i) {
        sd_manager_entry_t entry;
        if (sd_manager_entry_get(i, &entry) == ESP_OK && strcmp(entry.package_id, package_id) == 0) matches++;
    }
    return matches;
}

static esp_err_t render_package_list(sd_launcher_view_t *view);
static esp_err_t render_entry_list(sd_launcher_view_t *view);

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

static void package_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    sd_launcher_view_t *view = (sd_launcher_view_t *)lv_event_get_user_data(e);
    lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);
    if (!view || !button) return;

    uintptr_t encoded = (uintptr_t)lv_obj_get_user_data(button);
    if (encoded == 0) return;
    size_t index = (size_t)(encoded - 1U);

    sd_manager_entry_t entry;
    if (sd_manager_entry_get(index, &entry) != ESP_OK) return;

    strlcpy(view->selected_package_id, entry.package_id, sizeof(view->selected_package_id));
    strlcpy(view->selected_package_name, entry.package_name, sizeof(view->selected_package_name));
    render_entry_list(view);
}

static void back_button_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    sd_launcher_view_t *view = (sd_launcher_view_t *)lv_event_get_user_data(e);
    if (!view) return;
    render_package_list(view);
}

static esp_err_t render_package_list(sd_launcher_view_t *view)
{
    if (!view || !view->container) return ESP_ERR_INVALID_ARG;

    lv_obj_clean(view->container);
    view->empty_label = NULL;
    view->selected_package_id[0] = '\0';
    view->selected_package_name[0] = '\0';
    lv_obj_set_flex_flow(view->container, LV_FLEX_FLOW_ROW_WRAP);

    size_t count = sd_manager_entry_count();
    if (count == 0) {
        view->empty_label = lv_label_create(view->container);
        if (!view->empty_label) return ESP_ERR_NO_MEM;
        lv_label_set_text(view->empty_label, "No applications found");
        lv_obj_set_style_text_font(view->empty_label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(view->empty_label, lv_color_hex(0x8B949E), 0);
        lv_obj_set_width(view->empty_label, lv_pct(100));
        return ESP_OK;
    }

    size_t packages = 0;
    for (size_t i = 0; i < count; ++i) {
        sd_manager_entry_t entry;
        if (sd_manager_entry_get(i, &entry) != ESP_OK || !package_is_first(i, &entry)) continue;

        lv_obj_t *button = lv_button_create(view->container);
        if (!button) return ESP_ERR_NO_MEM;
        style_card(button, CARD_W, CARD_H, 0x1C2128);
        lv_obj_set_user_data(button, (void *)(uintptr_t)(i + 1U));
        lv_obj_add_event_cb(button, package_button_cb, LV_EVENT_CLICKED, view);

        lv_obj_t *accent = lv_obj_create(button);
        if (!accent) return ESP_ERR_NO_MEM;
        lv_obj_set_pos(accent, 0, 0);
        lv_obj_set_size(accent, 6, CARD_H);
        lv_obj_set_style_radius(accent, 14, 0);
        lv_obj_set_style_bg_color(accent, lv_color_hex(0x1F6FEB), 0);
        lv_obj_set_style_border_width(accent, 0, 0);
        make_decorative(accent);

        if (!make_text(button, 20, 12, 270, entry.package_name, &lv_font_montserrat_24, 0xF0F6FC)) return ESP_ERR_NO_MEM;

        size_t screens = package_entry_count(entry.package_id);
        char meta[48];
        snprintf(meta, sizeof(meta), "%u %s", (unsigned)screens, screens == 1 ? "screen" : "screens");
        if (!make_text(button, 20, 49, 220, meta, &lv_font_montserrat_18, 0x8B949E)) return ESP_ERR_NO_MEM;
        if (!make_text(button, 302, 28, 24, ">", &lv_font_montserrat_24, 0x58A6FF)) return ESP_ERR_NO_MEM;
        packages++;
    }

    ESP_LOGI(TAG, "launcher package view rebuilt packages=%u entries=%u",
             (unsigned)packages, (unsigned)count);
    return ESP_OK;
}

static esp_err_t render_entry_list(sd_launcher_view_t *view)
{
    if (!view || !view->container || !view->selected_package_id[0]) return ESP_ERR_INVALID_ARG;

    lv_obj_clean(view->container);
    view->empty_label = NULL;
    lv_obj_set_flex_flow(view->container, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t *header = lv_obj_create(view->container);
    if (!header) return ESP_ERR_NO_MEM;
    lv_obj_set_size(header, 686, 52);
    lv_obj_set_flex_grow(header, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back = lv_button_create(header);
    if (!back) return ESP_ERR_NO_MEM;
    lv_obj_set_pos(back, 0, 3);
    lv_obj_set_size(back, 104, 44);
    lv_obj_set_style_radius(back, 12, 0);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(back, 0, 0);
    lv_obj_add_event_cb(back, back_button_cb, LV_EVENT_CLICKED, view);
    if (!make_text(back, 13, 11, 82, "< APPS", &lv_font_montserrat_18, 0xF0F6FC)) return ESP_ERR_NO_MEM;

    if (!make_text(header, 126, 10, 540, view->selected_package_name, &lv_font_montserrat_24, 0xF0F6FC)) return ESP_ERR_NO_MEM;

    size_t count = sd_manager_entry_count();
    size_t entries = 0;
    for (size_t i = 0; i < count; ++i) {
        sd_manager_entry_t entry;
        if (sd_manager_entry_get(i, &entry) != ESP_OK) continue;
        if (strcmp(entry.package_id, view->selected_package_id) != 0) continue;

        lv_obj_t *button = lv_button_create(view->container);
        if (!button) return ESP_ERR_NO_MEM;
        style_card(button, CARD_W, ENTRY_H, 0x1F6FEB);
        lv_obj_set_user_data(button, (void *)(uintptr_t)(i + 1U));
        lv_obj_add_event_cb(button, entry_button_cb, LV_EVENT_CLICKED, view);
        if (!make_text(button, 18, 17, 285, entry.entry_name, &lv_font_montserrat_18, 0xFFFFFF)) return ESP_ERR_NO_MEM;
        if (!make_text(button, 304, 14, 22, ">", &lv_font_montserrat_24, 0xFFFFFF)) return ESP_ERR_NO_MEM;
        entries++;
    }

    if (entries == 0) {
        view->empty_label = lv_label_create(view->container);
        if (!view->empty_label) return ESP_ERR_NO_MEM;
        lv_label_set_text(view->empty_label, "No screens in this application");
        lv_obj_set_style_text_font(view->empty_label, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(view->empty_label, lv_color_hex(0x8B949E), 0);
    }

    ESP_LOGI(TAG, "launcher entry view package=%s entries=%u",
             view->selected_package_id, (unsigned)entries);
    return ESP_OK;
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
    lv_obj_set_style_pad_all(view->container, 0, 0);
    lv_obj_set_style_pad_row(view->container, CARD_GAP, 0);
    lv_obj_set_style_pad_column(view->container, CARD_GAP, 0);
    lv_obj_set_style_border_width(view->container, 0, 0);
    lv_obj_set_style_bg_opa(view->container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(view->container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scroll_dir(view->container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(view->container, LV_SCROLLBAR_MODE_AUTO);

    return view;
}

esp_err_t sd_launcher_refresh(sd_launcher_view_t *view)
{
    return render_package_list(view);
}

void sd_launcher_destroy(sd_launcher_view_t *view)
{
    if (!view) return;
    if (view->container) lv_obj_delete(view->container);
    free(view);
}
