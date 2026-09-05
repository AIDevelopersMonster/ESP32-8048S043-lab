/*
 * Project: KONTAKTS / ESP32-8048S043 Lab
 * Application: App 03 - Live Dashboard
 * Programmer: Sol
 * Engineer: Alex Malachevsky
 *
 * Hardware/runtime baseline is inherited from physically validated App 01/02.
 * New variable: real system telemetry + dashboard UI only.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/temperature_sensor.h"
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

#define TAG "APP03"

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
static temperature_sensor_handle_t s_temp = NULL;

static lv_obj_t *s_dashboard_screen = NULL;
static lv_obj_t *s_details_screen = NULL;
static lv_obj_t *s_temp_value = NULL;
static lv_obj_t *s_temp_arc = NULL;
static lv_obj_t *s_heap_value = NULL;
static lv_obj_t *s_heap_bar = NULL;
static lv_obj_t *s_psram_value = NULL;
static lv_obj_t *s_psram_bar = NULL;
static lv_obj_t *s_uptime_value = NULL;
static lv_obj_t *s_chart = NULL;
static lv_chart_series_t *s_temp_series = NULL;
static lv_obj_t *s_details_temp = NULL;
static lv_obj_t *s_details_heap = NULL;
static lv_obj_t *s_details_psram = NULL;
static lv_obj_t *s_details_uptime = NULL;

static float s_last_temp_c = 0.0f;
static size_t s_last_heap_free = 0;
static size_t s_last_psram_free = 0;
static uint64_t s_last_uptime_s = 0;

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
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
    if (err != ESP_OK) ESP_LOGE(TAG, "display flush failed: %s", esp_err_to_name(err));
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
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void make_decorative(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

static void style_screen(lv_obj_t *screen)
{
    make_decorative(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0D1117), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
}

static lv_obj_t *create_card(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, const char *title)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_radius(card, 22, LV_PART_MAIN);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x161B22), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);
    make_decorative(card);

    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label, title);
    lv_obj_set_pos(label, 16, 12);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x8B949E), 0);
    make_decorative(label);
    return card;
}

static void nav_details_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    printf("DASHBOARD:NAV:DETAILS\n");
    fflush(stdout);
    lv_screen_load(s_details_screen);
}

static void nav_back_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    printf("DASHBOARD:NAV:MAIN\n");
    fflush(stdout);
    lv_screen_load(s_dashboard_screen);
}

static lv_obj_t *create_nav_button(lv_obj_t *parent, int32_t x, int32_t y, const char *text, lv_event_cb_t cb)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, 170, 44);
    lv_obj_set_style_radius(button, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x1F6FEB), LV_PART_MAIN);
    lv_obj_set_style_opa(button, LV_OPA_70, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    make_decorative(label);
    return button;
}

static void create_dashboard_screen(void)
{
    s_dashboard_screen = lv_obj_create(NULL);
    style_screen(s_dashboard_screen);

    lv_obj_t *title = lv_label_create(s_dashboard_screen);
    lv_label_set_text(title, "App 03 - Live System Dashboard");
    lv_obj_set_pos(title, 24, 18);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF0F6FC), 0);
    make_decorative(title);

    lv_obj_t *live = lv_label_create(s_dashboard_screen);
    lv_label_set_text(live, "LIVE");
    lv_obj_set_pos(live, 466, 21);
    lv_obj_set_style_text_font(live, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(live, lv_color_hex(0x3FB950), 0);
    make_decorative(live);

    create_nav_button(s_dashboard_screen, 606, 8, "DETAILS  " LV_SYMBOL_RIGHT, nav_details_cb);

    lv_obj_t *temp_card = create_card(s_dashboard_screen, 24, 72, 280, 222, "ESP32-S3 TEMPERATURE");
    s_temp_arc = lv_arc_create(temp_card);
    lv_obj_set_size(s_temp_arc, 154, 154);
    lv_obj_set_pos(s_temp_arc, 18, 46);
    lv_arc_set_range(s_temp_arc, 10, 80);
    lv_arc_set_rotation(s_temp_arc, 135);
    lv_arc_set_bg_angles(s_temp_arc, 0, 270);
    lv_obj_set_style_arc_width(s_temp_arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_temp_arc, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_temp_arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_temp_arc, lv_color_hex(0x58A6FF), LV_PART_INDICATOR);
    lv_obj_remove_style(s_temp_arc, NULL, LV_PART_KNOB);
    make_decorative(s_temp_arc);

    s_temp_value = lv_label_create(temp_card);
    lv_label_set_text(s_temp_value, "--.- C");
    lv_obj_set_pos(s_temp_value, 168, 94);
    lv_obj_set_style_text_font(s_temp_value, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(s_temp_value, lv_color_white(), 0);
    make_decorative(s_temp_value);

    lv_obj_t *mem_card = create_card(s_dashboard_screen, 322, 72, 454, 222, "MEMORY / UPTIME");

    lv_obj_t *heap_label = lv_label_create(mem_card);
    lv_label_set_text(heap_label, "INTERNAL HEAP");
    lv_obj_set_pos(heap_label, 18, 52);
    lv_obj_set_style_text_font(heap_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(heap_label, lv_color_hex(0x8B949E), 0);
    make_decorative(heap_label);

    s_heap_value = lv_label_create(mem_card);
    lv_label_set_text(s_heap_value, "-- KB");
    lv_obj_set_pos(s_heap_value, 310, 52);
    lv_obj_set_style_text_font(s_heap_value, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_heap_value, lv_color_white(), 0);
    make_decorative(s_heap_value);

    s_heap_bar = lv_bar_create(mem_card);
    lv_obj_set_pos(s_heap_bar, 18, 82);
    lv_obj_set_size(s_heap_bar, 412, 18);
    lv_bar_set_range(s_heap_bar, 0, 100);
    lv_obj_set_style_bg_color(s_heap_bar, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_heap_bar, lv_color_hex(0x3FB950), LV_PART_INDICATOR);
    make_decorative(s_heap_bar);

    lv_obj_t *psram_label = lv_label_create(mem_card);
    lv_label_set_text(psram_label, "PSRAM");
    lv_obj_set_pos(psram_label, 18, 118);
    lv_obj_set_style_text_font(psram_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(psram_label, lv_color_hex(0x8B949E), 0);
    make_decorative(psram_label);

    s_psram_value = lv_label_create(mem_card);
    lv_label_set_text(s_psram_value, "-- KB");
    lv_obj_set_pos(s_psram_value, 310, 118);
    lv_obj_set_style_text_font(s_psram_value, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_psram_value, lv_color_white(), 0);
    make_decorative(s_psram_value);

    s_psram_bar = lv_bar_create(mem_card);
    lv_obj_set_pos(s_psram_bar, 18, 148);
    lv_obj_set_size(s_psram_bar, 412, 18);
    lv_bar_set_range(s_psram_bar, 0, 100);
    lv_obj_set_style_bg_color(s_psram_bar, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_psram_bar, lv_color_hex(0xA371F7), LV_PART_INDICATOR);
    make_decorative(s_psram_bar);

    s_uptime_value = lv_label_create(mem_card);
    lv_label_set_text(s_uptime_value, "UPTIME  00:00:00");
    lv_obj_set_pos(s_uptime_value, 18, 182);
    lv_obj_set_style_text_font(s_uptime_value, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_uptime_value, lv_color_hex(0xD2A8FF), 0);
    make_decorative(s_uptime_value);

    lv_obj_t *chart_card = create_card(s_dashboard_screen, 24, 312, 752, 146, "TEMPERATURE HISTORY - 60 SAMPLES");
    s_chart = lv_chart_create(chart_card);
    lv_obj_set_pos(s_chart, 18, 40);
    lv_obj_set_size(s_chart, 716, 88);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 10, 80);
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x0D1117), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(s_chart, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_line_opa(s_chart, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);
    make_decorative(s_chart);
    s_temp_series = lv_chart_add_series(s_chart, lv_color_hex(0x58A6FF), LV_CHART_AXIS_PRIMARY_Y);
}

static void create_details_screen(void)
{
    s_details_screen = lv_obj_create(NULL);
    style_screen(s_details_screen);

    lv_obj_t *title = lv_label_create(s_details_screen);
    lv_label_set_text(title, "App 03 - Telemetry Details");
    lv_obj_set_pos(title, 24, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    make_decorative(title);

    create_nav_button(s_details_screen, 606, 8, LV_SYMBOL_LEFT "  BACK", nav_back_cb);

    lv_obj_t *card = create_card(s_details_screen, 24, 84, 752, 340, "REAL SYSTEM TELEMETRY");
    s_details_temp = lv_label_create(card);
    s_details_heap = lv_label_create(card);
    s_details_psram = lv_label_create(card);
    s_details_uptime = lv_label_create(card);

    lv_obj_t *labels[] = {s_details_temp, s_details_heap, s_details_psram, s_details_uptime};
    for (int i = 0; i < 4; i++) {
        lv_obj_set_pos(labels[i], 34, 58 + i * 58);
        lv_obj_set_style_text_font(labels[i], &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(0xF0F6FC), 0);
        make_decorative(labels[i]);
    }

    lv_obj_t *note = lv_label_create(card);
    lv_label_set_text(note, "Values come from ESP-IDF runtime APIs. No simulated sensor data in v0.1.0.");
    lv_obj_set_pos(note, 34, 294);
    lv_obj_set_style_text_font(note, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(note, lv_color_hex(0x8B949E), 0);
    make_decorative(note);
}

static void create_ui(void)
{
    create_dashboard_screen();
    create_details_screen();
    lv_screen_load(s_dashboard_screen);
}

static void init_temperature_sensor(void)
{
    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&cfg, &s_temp));
    ESP_ERROR_CHECK(temperature_sensor_enable(s_temp));
}

static void refresh_telemetry(void)
{
    float temp_c = 0.0f;
    esp_err_t temp_err = temperature_sensor_get_celsius(s_temp, &temp_c);
    if (temp_err == ESP_OK) s_last_temp_c = temp_c;

    s_last_heap_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    s_last_psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    s_last_uptime_s = (uint64_t)(esp_timer_get_time() / 1000000ULL);

    const size_t heap_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    const size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    int heap_pct = heap_total ? (int)((s_last_heap_free * 100U) / heap_total) : 0;
    int psram_pct = psram_total ? (int)((s_last_psram_free * 100U) / psram_total) : 0;
    int temp_arc = (int)(s_last_temp_c + 0.5f);
    if (temp_arc < 10) temp_arc = 10;
    if (temp_arc > 80) temp_arc = 80;

    int temp_tenths = (int)(s_last_temp_c * 10.0f);
    int temp_whole = temp_tenths / 10;
    int temp_frac = temp_tenths >= 0 ? temp_tenths % 10 : -(temp_tenths % 10);

    lv_arc_set_value(s_temp_arc, temp_arc);
    lv_label_set_text_fmt(s_temp_value, "%d.%d C", temp_whole, temp_frac);
    lv_label_set_text_fmt(s_heap_value, "%u KB", (unsigned)(s_last_heap_free / 1024U));
    lv_label_set_text_fmt(s_psram_value, "%u KB", (unsigned)(s_last_psram_free / 1024U));
    lv_bar_set_value(s_heap_bar, heap_pct, LV_ANIM_OFF);
    lv_bar_set_value(s_psram_bar, psram_pct, LV_ANIM_OFF);

    uint64_t h = s_last_uptime_s / 3600ULL;
    uint64_t m = (s_last_uptime_s % 3600ULL) / 60ULL;
    uint64_t s = s_last_uptime_s % 60ULL;
    lv_label_set_text_fmt(s_uptime_value, "UPTIME  %02llu:%02llu:%02llu",
                          (unsigned long long)h,
                          (unsigned long long)m,
                          (unsigned long long)s);

    lv_chart_set_next_value(s_chart, s_temp_series, temp_arc);

    lv_label_set_text_fmt(s_details_temp, "Temperature: %d.%d C", temp_whole, temp_frac);
    lv_label_set_text_fmt(s_details_heap, "Internal heap free: %u / %u KB",
                          (unsigned)(s_last_heap_free / 1024U),
                          (unsigned)(heap_total / 1024U));
    lv_label_set_text_fmt(s_details_psram, "PSRAM free: %u / %u KB",
                          (unsigned)(s_last_psram_free / 1024U),
                          (unsigned)(psram_total / 1024U));
    lv_label_set_text_fmt(s_details_uptime, "Uptime: %llu seconds",
                          (unsigned long long)s_last_uptime_s);

    printf("TELEMETRY:TEMP:%d.%d:HEAP_KB:%u:PSRAM_KB:%u:UPTIME_S:%llu\n",
           temp_whole,
           temp_frac,
           (unsigned)(s_last_heap_free / 1024U),
           (unsigned)(s_last_psram_free / 1024U),
           (unsigned long long)s_last_uptime_s);
    fflush(stdout);
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

    lv_display_set_buffers(display, draw_buffer, NULL, draw_buffer_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}

static void app_ui_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "UI task started on core %d with %u-byte stack", xPortGetCoreID(), (unsigned)APP_UI_TASK_STACK_SIZE);
    ESP_LOGI(TAG, "Free INTERNAL heap before init: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "Free PSRAM before init: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    init_display();
    init_touch();
    init_temperature_sensor();
    init_lvgl();
    create_ui();
    refresh_telemetry();
    gpio_set_level(LCD_PIN_BL, 1);

    UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "Ready. UI task stack high-water after init: %u bytes", (unsigned)high_water);

    TickType_t last_sample = xTaskGetTickCount();
    TickType_t last_stack_log = last_sample;
    while (true) {
        lv_timer_handler();
        TickType_t now = xTaskGetTickCount();

        if ((now - last_sample) >= pdMS_TO_TICKS(1000)) {
            refresh_telemetry();
            last_sample = now;
        }

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
    ESP_LOGI(TAG, "App 03 - Live System Dashboard");
    BaseType_t created = xTaskCreatePinnedToCore(app_ui_task,
                                                 "app03_ui",
                                                 APP_UI_TASK_STACK_SIZE,
                                                 NULL,
                                                 APP_UI_TASK_PRIORITY,
                                                 NULL,
                                                 APP_UI_TASK_CORE);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create app03_ui task");
        abort();
    }
    ESP_LOGI(TAG, "Dedicated UI task created; app_main returning");
}
