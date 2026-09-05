/*
 * Project: KONTAKTS / ESP32-8048S043 Lab
 * Application: App 02 - Mixed Widgets
 * Programmer: Sol
 * Engineer: Alex Malachevsky
 *
 * Hardware baseline is intentionally inherited from physically validated App 01.
 * This experiment changes only the LVGL UI layer.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#define TAG "APP02"

#define LCD_H_RES 800
#define LCD_V_RES 480
#define LCD_PCLK_HZ (16 * 1000 * 1000)
#define LCD_BOUNCE_LINES 10
#define LVGL_BUF_LINES 60

#define APP_UI_TASK_STACK_SIZE 16384
#define APP_UI_TASK_PRIORITY 9
#define APP_UI_TASK_CORE 1

#define LCD_PIN_BL    GPIO_NUM_2
#define LCD_PIN_HSYNC GPIO_NUM_39
#define LCD_PIN_VSYNC GPIO_NUM_41
#define LCD_PIN_DE    GPIO_NUM_40
#define LCD_PIN_PCLK  GPIO_NUM_42

#define TOUCH_PIN_SDA GPIO_NUM_19
#define TOUCH_PIN_SCL GPIO_NUM_20
#define TOUCH_PIN_RST GPIO_NUM_38
#define TOUCH_I2C_PORT I2C_NUM_1
#define TOUCH_I2C_HZ 400000

#define TOUCH_RAW_X_MAX 479
#define TOUCH_RAW_Y_MAX 271

static esp_lcd_panel_handle_t s_panel = NULL;
static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_touch_io = NULL;
static esp_lcd_touch_handle_t s_touch = NULL;

static lv_obj_t *s_controls_screen = NULL;
static lv_obj_t *s_status_screen = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_switch = NULL;
static lv_obj_t *s_switch_value_label = NULL;
static lv_obj_t *s_slider = NULL;
static lv_obj_t *s_slider_value_label = NULL;
static lv_obj_t *s_arc = NULL;
static lv_obj_t *s_arc_value_label = NULL;
static lv_obj_t *s_progress_bar = NULL;
static lv_obj_t *s_progress_value_label = NULL;
static lv_obj_t *s_summary_action = NULL;
static lv_obj_t *s_summary_switch = NULL;
static lv_obj_t *s_summary_slider = NULL;
static lv_obj_t *s_summary_arc = NULL;

static uint32_t s_action_count = 0;
static bool s_switch_on = true;
static int32_t s_slider_value = 35;
static int32_t s_arc_value = 65;

static uint16_t scale_touch(uint16_t value, uint16_t in_max, uint16_t out_max)
{
    if (value > in_max) value = in_max;
    return (uint16_t)(((uint32_t)value * out_max) / in_max);
}

static void touch_process_coordinates(esp_lcd_touch_handle_t tp,
                                      uint16_t *x,
                                      uint16_t *y,
                                      uint16_t *strength,
                                      uint8_t *point_num,
                                      uint8_t max_point_num)
{
    (void)tp;
    (void)strength;
    uint8_t count = *point_num;
    if (count > max_point_num) count = max_point_num;

    for (uint8_t i = 0; i < count; i++) {
        x[i] = scale_touch(x[i], TOUCH_RAW_X_MAX, LCD_H_RES - 1);
        y[i] = scale_touch(y[i], TOUCH_RAW_Y_MAX, LCD_V_RES - 1);
    }
}

static void display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(display);
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel,
                                               area->x1,
                                               area->y1,
                                               area->x2 + 1,
                                               area->y2 + 1,
                                               px_map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "display flush failed: %s", esp_err_to_name(err));
    }
    lv_display_flush_ready(display);
}

static uint32_t lv_tick_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t x[1] = {0};
    uint16_t y[1] = {0};
    uint8_t count = 0;

    esp_lcd_touch_read_data(s_touch);
    bool pressed = esp_lcd_touch_get_coordinates(s_touch, x, y, NULL, &count, 1);

    if (pressed && count > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x[0];
        data->point.y = y[0];
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void make_decorative(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

static void style_card_base(lv_obj_t *obj, uint32_t color)
{
    lv_obj_set_size(obj, 236, 160);
    lv_obj_set_style_radius(obj, 22, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x455A64), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

static lv_obj_t *create_panel(lv_obj_t *parent,
                              int32_t x,
                              int32_t y,
                              uint32_t color,
                              const char *title)
{
    lv_obj_t *panel = lv_obj_create(parent);
    style_card_base(panel, color);
    lv_obj_set_pos(panel, x, y);

    /* A panel is presentation only. Interactive child widgets own the touch. */
    make_decorative(panel);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, title);
    make_decorative(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xECEFF1), 0);
    lv_obj_set_pos(label, 16, 14);

    return panel;
}

static void refresh_summary(void)
{
    if (!s_summary_action) return;

    lv_label_set_text_fmt(s_summary_action, "COMMAND taps: %lu", (unsigned long)s_action_count);
    lv_label_set_text_fmt(s_summary_switch, "SWITCH: %s", s_switch_on ? "ON" : "OFF");
    lv_label_set_text_fmt(s_summary_slider, "SLIDER: %ld%%", (long)s_slider_value);
    lv_label_set_text_fmt(s_summary_arc, "ARC: %ld", (long)s_arc_value);
}

static void command_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    s_action_count++;
    printf("WIDGET:COMMAND:RUN:%lu\n", (unsigned long)s_action_count);
    fflush(stdout);

    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label, "COMMAND fired - count %lu", (unsigned long)s_action_count);
    }
    refresh_summary();
}

static void switch_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;

    lv_obj_t *sw = lv_event_get_target_obj(event);
    s_switch_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    lv_label_set_text(s_switch_value_label, s_switch_on ? "ON" : "OFF");

    printf("WIDGET:SWITCH:%s\n", s_switch_on ? "ON" : "OFF");
    fflush(stdout);

    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label, "SWITCH -> %s", s_switch_on ? "ON" : "OFF");
    }
    refresh_summary();
}

static void slider_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;

    lv_obj_t *slider = lv_event_get_target_obj(event);
    s_slider_value = lv_slider_get_value(slider);

    lv_label_set_text_fmt(s_slider_value_label, "%ld%%", (long)s_slider_value);
    lv_bar_set_value(s_progress_bar, s_slider_value, LV_ANIM_OFF);
    lv_label_set_text_fmt(s_progress_value_label, "%ld%%", (long)s_slider_value);

    if (code == LV_EVENT_RELEASED) {
        printf("WIDGET:SLIDER:%ld\n", (long)s_slider_value);
        fflush(stdout);
        if (s_status_label) {
            lv_label_set_text_fmt(s_status_label, "SLIDER released at %ld%%", (long)s_slider_value);
        }
        refresh_summary();
    }
}

static void arc_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_VALUE_CHANGED && code != LV_EVENT_RELEASED) return;

    lv_obj_t *arc = lv_event_get_target_obj(event);
    s_arc_value = lv_arc_get_value(arc);
    lv_label_set_text_fmt(s_arc_value_label, "%ld", (long)s_arc_value);

    if (code == LV_EVENT_RELEASED) {
        printf("WIDGET:ARC:%ld\n", (long)s_arc_value);
        fflush(stdout);
        if (s_status_label) {
            lv_label_set_text_fmt(s_status_label, "ARC released at %ld", (long)s_arc_value);
        }
        refresh_summary();
    }
}

static void nav_status_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    refresh_summary();
    printf("WIDGET:NAV:STATUS\n");
    fflush(stdout);
    lv_screen_load(s_status_screen);
}

static void nav_back_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    printf("WIDGET:NAV:CONTROLS\n");
    fflush(stdout);
    lv_screen_load(s_controls_screen);
}

static lv_obj_t *create_full_card_button(lv_obj_t *parent,
                                         int32_t x,
                                         int32_t y,
                                         uint32_t color,
                                         const char *symbol,
                                         const char *title,
                                         const char *subtitle,
                                         lv_event_cb_t cb)
{
    lv_obj_t *button = lv_button_create(parent);
    style_card_base(button, color);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_opa(button, LV_OPA_70, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_label_create(button);
    lv_label_set_text(icon, symbol);
    make_decorative(icon);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *title_label = lv_label_create(button);
    lv_label_set_text(title_label, title);
    make_decorative(title_label);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_BOTTOM_MID, 0, -34);

    lv_obj_t *sub_label = lv_label_create(button);
    lv_label_set_text(sub_label, subtitle);
    make_decorative(sub_label);
    lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(sub_label, lv_color_hex(0xCFD8DC), 0);
    lv_obj_align(sub_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    return button;
}

static void create_controls_screen(void)
{
    s_controls_screen = lv_obj_create(NULL);
    make_decorative(s_controls_screen);
    lv_obj_set_style_bg_color(s_controls_screen, lv_color_hex(0x101418), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_controls_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_controls_screen, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(s_controls_screen);
    lv_label_set_text(title, "App 02 - Mixed Widgets");
    make_decorative(title);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8EEF2), 0);
    lv_obj_set_pos(title, 24, 20);

    lv_obj_t *badge = lv_label_create(s_controls_screen);
    lv_label_set_text(badge, "CONTROL LAB");
    make_decorative(badge);
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0x90CAF9), 0);
    lv_obj_set_pos(badge, 634, 20);

    const int32_t x0 = 24;
    const int32_t x1 = 282;
    const int32_t x2 = 540;
    const int32_t y0 = 68;
    const int32_t y1 = 246;

    create_full_card_button(s_controls_screen,
                            x0,
                            y0,
                            0x1565C0,
                            LV_SYMBOL_PLAY,
                            "COMMAND",
                            "tap anywhere",
                            command_event_cb);

    lv_obj_t *switch_panel = create_panel(s_controls_screen, x1, y0, 0x263238, "SWITCH");
    s_switch = lv_switch_create(switch_panel);
    lv_obj_set_size(s_switch, 108, 54);
    lv_obj_align(s_switch, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_ext_click_area(s_switch, 12);
    lv_obj_add_state(s_switch, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(s_switch, lv_color_hex(0x37474F), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_switch, lv_color_hex(0x26A69A), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(s_switch, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(s_switch, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_switch_value_label = lv_label_create(switch_panel);
    lv_label_set_text(s_switch_value_label, "ON");
    make_decorative(s_switch_value_label);
    lv_obj_set_style_text_font(s_switch_value_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_switch_value_label, lv_color_hex(0x80CBC4), 0);
    lv_obj_align(s_switch_value_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_obj_t *slider_panel = create_panel(s_controls_screen, x2, y0, 0x263238, "SLIDER");
    s_slider = lv_slider_create(slider_panel);
    lv_obj_set_size(s_slider, 184, 28);
    lv_obj_align(s_slider, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_ext_click_area(s_slider, 14);
    lv_slider_set_range(s_slider, 0, 100);
    lv_slider_set_value(s_slider, s_slider_value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(0x455A64), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(0x42A5F5), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider, 9, LV_PART_KNOB);
    lv_obj_add_event_cb(s_slider, slider_event_cb, LV_EVENT_ALL, NULL);

    s_slider_value_label = lv_label_create(slider_panel);
    lv_label_set_text_fmt(s_slider_value_label, "%ld%%", (long)s_slider_value);
    make_decorative(s_slider_value_label);
    lv_obj_set_style_text_font(s_slider_value_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_slider_value_label, lv_color_hex(0x90CAF9), 0);
    lv_obj_align(s_slider_value_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_obj_t *arc_panel = create_panel(s_controls_screen, x0, y1, 0x263238, "ARC");
    s_arc = lv_arc_create(arc_panel);
    lv_obj_set_size(s_arc, 112, 112);
    lv_obj_align(s_arc, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_ext_click_area(s_arc, 10);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_rotation(s_arc, 135);
    lv_arc_set_bg_angles(s_arc, 0, 270);
    lv_arc_set_value(s_arc, s_arc_value);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x455A64), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(0xAB47BC), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_arc, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_arc, 5, LV_PART_KNOB);
    lv_obj_add_event_cb(s_arc, arc_event_cb, LV_EVENT_ALL, NULL);

    s_arc_value_label = lv_label_create(arc_panel);
    lv_label_set_text_fmt(s_arc_value_label, "%ld", (long)s_arc_value);
    make_decorative(s_arc_value_label);
    lv_obj_set_style_text_font(s_arc_value_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_arc_value_label, lv_color_white(), 0);
    lv_obj_align(s_arc_value_label, LV_ALIGN_CENTER, 0, 12);

    lv_obj_t *progress_panel = create_panel(s_controls_screen, x1, y1, 0x263238, "PROGRESS / STATUS");
    s_progress_bar = lv_bar_create(progress_panel);
    lv_obj_set_size(s_progress_bar, 188, 24);
    lv_obj_align(s_progress_bar, LV_ALIGN_CENTER, 0, 7);
    make_decorative(s_progress_bar);
    lv_bar_set_range(s_progress_bar, 0, 100);
    lv_bar_set_value(s_progress_bar, s_slider_value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_progress_bar, lv_color_hex(0x455A64), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_progress_bar, lv_color_hex(0x66BB6A), LV_PART_INDICATOR);

    s_progress_value_label = lv_label_create(progress_panel);
    lv_label_set_text_fmt(s_progress_value_label, "%ld%%", (long)s_slider_value);
    make_decorative(s_progress_value_label);
    lv_obj_set_style_text_font(s_progress_value_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_progress_value_label, lv_color_hex(0xA5D6A7), 0);
    lv_obj_align(s_progress_value_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    create_full_card_button(s_controls_screen,
                            x2,
                            y1,
                            0x37474F,
                            LV_SYMBOL_RIGHT,
                            "NAVIGATION",
                            "status page",
                            nav_status_event_cb);

    s_status_label = lv_label_create(s_controls_screen);
    lv_label_set_text(s_status_label, "Ready - each widget has one intentional touch owner");
    make_decorative(s_status_label);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xB0BEC5), 0);
    lv_obj_set_pos(s_status_label, 24, 438);
}

static void create_status_screen(void)
{
    s_status_screen = lv_obj_create(NULL);
    make_decorative(s_status_screen);
    lv_obj_set_style_bg_color(s_status_screen, lv_color_hex(0x101418), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_status_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_status_screen, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(s_status_screen);
    lv_label_set_text(title, "App 02 - Live Status");
    make_decorative(title);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8EEF2), 0);
    lv_obj_set_pos(title, 24, 22);

    lv_obj_t *back = lv_button_create(s_status_screen);
    lv_obj_set_size(back, 236, 48);
    lv_obj_set_pos(back, 540, 8);
    lv_obj_set_style_radius(back, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x37474F), LV_PART_MAIN);
    lv_obj_set_style_opa(back, LV_OPA_70, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(back, nav_back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT "  BACK TO CONTROLS");
    make_decorative(back_label);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    lv_obj_t *panel = lv_obj_create(s_status_screen);
    lv_obj_set_size(panel, 752, 330);
    lv_obj_set_pos(panel, 24, 86);
    lv_obj_set_style_radius(panel, 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x182027), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x37474F), LV_PART_MAIN);
    make_decorative(panel);

    s_summary_action = lv_label_create(panel);
    s_summary_switch = lv_label_create(panel);
    s_summary_slider = lv_label_create(panel);
    s_summary_arc = lv_label_create(panel);

    lv_obj_t *labels[] = {s_summary_action, s_summary_switch, s_summary_slider, s_summary_arc};
    for (int i = 0; i < 4; i++) {
        make_decorative(labels[i]);
        lv_obj_set_style_text_font(labels[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(0xECEFF1), 0);
        lv_obj_set_pos(labels[i], 40, 36 + i * 58);
    }

    lv_obj_t *note = lv_label_create(panel);
    lv_label_set_text(note, "PROGRESS follows SLIDER live. Return and keep interacting.");
    make_decorative(note);
    lv_obj_set_style_text_font(note, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(note, lv_color_hex(0x90CAF9), 0);
    lv_obj_set_pos(note, 40, 272);

    refresh_summary();
}

static void create_ui(void)
{
    create_controls_screen();
    create_status_screen();
    lv_screen_load(s_controls_screen);
}

static void init_display(void)
{
    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PCLK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags = {
                .hsync_idle_low = true,
                .vsync_idle_low = true,
                .de_idle_high = false,
                .pclk_active_neg = true,
            },
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = LCD_H_RES * LCD_BOUNCE_LINES,
        .sram_trans_align = 8,
        .psram_trans_align = 64,
        .hsync_gpio_num = LCD_PIN_HSYNC,
        .vsync_gpio_num = LCD_PIN_VSYNC,
        .de_gpio_num = LCD_PIN_DE,
        .pclk_gpio_num = LCD_PIN_PCLK,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums = {
            GPIO_NUM_8, GPIO_NUM_3, GPIO_NUM_46, GPIO_NUM_9, GPIO_NUM_1,
            GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_4,
            GPIO_NUM_45, GPIO_NUM_48, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_14,
        },
        .flags = {
            .fb_in_psram = true,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    const gpio_config_t bl_config = {
        .pin_bit_mask = 1ULL << LCD_PIN_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_config));
    gpio_set_level(LCD_PIN_BL, 0);
}

static void init_touch(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = TOUCH_I2C_PORT,
        .sda_io_num = TOUCH_PIN_SDA,
        .scl_io_num = TOUCH_PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &s_i2c_bus));

    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_config.scl_speed_hz = TOUCH_I2C_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_config, &s_touch_io));

    const esp_lcd_touch_config_t touch_config = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = TOUCH_PIN_RST,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = touch_process_coordinates,
    };

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(s_touch_io, &touch_config, &s_touch));
}

static void init_lvgl(void)
{
    lv_init();
    lv_tick_set_cb(lv_tick_ms);

    lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_user_data(display, s_panel);
    lv_display_set_flush_cb(display, display_flush_cb);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    const size_t draw_buffer_bytes = LCD_H_RES * LVGL_BUF_LINES * sizeof(uint16_t);
    void *draw_buffer = heap_caps_malloc(draw_buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!draw_buffer) {
        ESP_LOGE(TAG, "Failed to allocate %u-byte INTERNAL LVGL buffer", (unsigned)draw_buffer_bytes);
        abort();
    }

    lv_display_set_buffers(display,
                           draw_buffer,
                           NULL,
                           draw_buffer_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}

static void app_ui_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "UI task started on core %d with %u-byte stack",
             xPortGetCoreID(),
             (unsigned)APP_UI_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "Free INTERNAL heap before display: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Free PSRAM before display: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    init_display();
    init_touch();
    init_lvgl();
    create_ui();

    gpio_set_level(LCD_PIN_BL, 1);

    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "Ready. UI task stack high-water after init: %u bytes", (unsigned)high_water);
    ESP_LOGI(TAG, "Test COMMAND, SWITCH, SLIDER, ARC, PROGRESS and NAVIGATION.");

    TickType_t last_stack_log = xTaskGetTickCount();
    while (true) {
        lv_timer_handler();

        TickType_t now = xTaskGetTickCount();
        if ((now - last_stack_log) >= pdMS_TO_TICKS(10000)) {
            high_water = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "UI task stack high-water: %u bytes", (unsigned)high_water);
            last_stack_log = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "App 02 - Mixed Widgets");

    BaseType_t created = xTaskCreatePinnedToCore(app_ui_task,
                                                 "app02_ui",
                                                 APP_UI_TASK_STACK_SIZE,
                                                 NULL,
                                                 APP_UI_TASK_PRIORITY,
                                                 NULL,
                                                 APP_UI_TASK_CORE);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create app02_ui task");
        abort();
    }

    ESP_LOGI(TAG, "Dedicated UI task created; app_main returning");
}
