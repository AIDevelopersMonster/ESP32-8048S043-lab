#include "display_ota.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#include "network_manager.h"
#include "ota_manager.h"

#define TAG "APP06_UI"

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

static esp_lcd_panel_handle_t s_panel;
static i2c_master_bus_handle_t s_i2c_bus;
static esp_lcd_panel_io_handle_t s_touch_io;
static esp_lcd_touch_handle_t s_touch;

static lv_obj_t *s_screen;
static lv_obj_t *s_status_panel;
static lv_obj_t *s_ota_panel;
static lv_obj_t *s_status_tab;
static lv_obj_t *s_ota_tab;

static lv_obj_t *s_status_version;
static lv_obj_t *s_status_network;
static lv_obj_t *s_status_ip;
static lv_obj_t *s_status_partition;
static lv_obj_t *s_status_image;
static lv_obj_t *s_status_hint;

static lv_obj_t *s_ota_versions;
static lv_obj_t *s_ota_state;
static lv_obj_t *s_ota_message;
static lv_obj_t *s_ota_progress;
static lv_obj_t *s_ota_progress_label;
static lv_obj_t *s_check_button;
static lv_obj_t *s_install_button;
static lv_obj_t *s_confirm_button;
static lv_obj_t *s_rollback_button;
static lv_obj_t *s_recovery_button;

static bool s_showing_ota;
static bool s_first_refresh = true;

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
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void make_decorative(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

static void style_root(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0D1117), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

static lv_obj_t *make_label(lv_obj_t *parent,
                            int32_t x,
                            int32_t y,
                            const char *text,
                            const lv_font_t *font,
                            uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    make_decorative(label);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent,
                             int32_t x,
                             int32_t y,
                             int32_t w,
                             int32_t h,
                             const char *text,
                             lv_event_cb_t cb)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, h);
    lv_obj_set_style_radius(button, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x1F6FEB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x30363D), LV_PART_MAIN | LV_STATE_DISABLED);
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

static void set_button_enabled(lv_obj_t *button, bool enabled)
{
    if (enabled) {
        lv_obj_remove_state(button, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }
}

static void show_status_panel(void)
{
    s_showing_ota = false;
    lv_obj_remove_flag(s_status_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_ota_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(s_status_tab, lv_color_hex(0x1F6FEB), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_tab, lv_color_hex(0x21262D), LV_PART_MAIN);
}

static void show_ota_panel(void)
{
    s_showing_ota = true;
    lv_obj_add_flag(s_status_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ota_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(s_status_tab, lv_color_hex(0x21262D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_tab, lv_color_hex(0x1F6FEB), LV_PART_MAIN);
}

static void status_tab_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) show_status_panel();
}

static void ota_tab_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) show_ota_panel();
}

static void show_action_error(const char *action, esp_err_t err)
{
    if (err == ESP_OK) return;
    ESP_LOGW(TAG, "%s rejected: %s", action, esp_err_to_name(err));
    lv_label_set_text_fmt(s_ota_message, "%s rejected: %s", action, esp_err_to_name(err));
}

static void check_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    show_action_error("CHECK", ota_manager_start_check());
}

static void install_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    show_action_error("INSTALL", ota_manager_start_install());
}

static void confirm_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    show_action_error("CONFIRM", ota_manager_confirm_running());
}

static void rollback_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    show_action_error("ROLLBACK", ota_manager_start_rollback());
}

static void recovery_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    show_action_error("FACTORY", ota_manager_start_recovery());
}

static void create_ui(void)
{
    s_screen = lv_obj_create(NULL);
    style_root(s_screen);

    make_label(s_screen, 20, 14, "KONTAKTS App06", &lv_font_montserrat_24, 0xF0F6FC);
    make_label(s_screen, 220, 18, "ESP32-8048S043  |  GitHub OTA / Recovery", &lv_font_montserrat_18, 0x8B949E);

    s_status_tab = make_button(s_screen, 548, 8, 112, 44, "STATUS", status_tab_cb);
    s_ota_tab = make_button(s_screen, 672, 8, 112, 44, "OTA", ota_tab_cb);

    s_status_panel = lv_obj_create(s_screen);
    lv_obj_set_pos(s_status_panel, 16, 64);
    lv_obj_set_size(s_status_panel, 768, 400);
    lv_obj_set_style_radius(s_status_panel, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_status_panel, lv_color_hex(0x161B22), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_status_panel, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_status_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_status_panel, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_status_panel, LV_OBJ_FLAG_SCROLLABLE);

    make_label(s_status_panel, 24, 18, "DEVICE STATUS", &lv_font_montserrat_18, 0x8B949E);
    s_status_version = make_label(s_status_panel, 24, 56, "Version --", &lv_font_montserrat_36, 0xF0F6FC);
    s_status_network = make_label(s_status_panel, 28, 126, "Network: BOOT", &lv_font_montserrat_24, 0x58A6FF);
    s_status_ip = make_label(s_status_panel, 28, 168, "IP: 0.0.0.0", &lv_font_montserrat_24, 0xF0F6FC);
    s_status_partition = make_label(s_status_panel, 28, 210, "Partition: --", &lv_font_montserrat_24, 0xF0F6FC);
    s_status_image = make_label(s_status_panel, 28, 252, "Image: --", &lv_font_montserrat_24, 0xF0F6FC);
    s_status_hint = make_label(s_status_panel, 28, 318, "", &lv_font_montserrat_18, 0x8B949E);
    lv_obj_set_width(s_status_hint, 710);
    lv_label_set_long_mode(s_status_hint, LV_LABEL_LONG_WRAP);

    s_ota_panel = lv_obj_create(s_screen);
    lv_obj_set_pos(s_ota_panel, 16, 64);
    lv_obj_set_size(s_ota_panel, 768, 400);
    lv_obj_set_style_radius(s_ota_panel, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_panel, lv_color_hex(0x161B22), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_ota_panel, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_ota_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_ota_panel, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_ota_panel, LV_OBJ_FLAG_SCROLLABLE);

    make_label(s_ota_panel, 24, 16, "GITHUB OTA CONTROL", &lv_font_montserrat_18, 0x8B949E);
    s_ota_versions = make_label(s_ota_panel, 24, 48, "Installed --   Available --", &lv_font_montserrat_24, 0xF0F6FC);
    s_ota_state = make_label(s_ota_panel, 24, 84, "IDLE", &lv_font_montserrat_24, 0x58A6FF);
    s_ota_message = make_label(s_ota_panel, 24, 120, "Ready", &lv_font_montserrat_18, 0xC9D1D9);
    lv_obj_set_width(s_ota_message, 714);
    lv_label_set_long_mode(s_ota_message, LV_LABEL_LONG_WRAP);

    s_ota_progress = lv_bar_create(s_ota_panel);
    lv_obj_set_pos(s_ota_progress, 24, 174);
    lv_obj_set_size(s_ota_progress, 610, 22);
    lv_bar_set_range(s_ota_progress, 0, 100);
    lv_obj_set_style_bg_color(s_ota_progress, lv_color_hex(0x30363D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_ota_progress, lv_color_hex(0x3FB950), LV_PART_INDICATOR);
    make_decorative(s_ota_progress);
    s_ota_progress_label = make_label(s_ota_panel, 650, 170, "0%", &lv_font_montserrat_18, 0xF0F6FC);

    s_check_button = make_button(s_ota_panel, 24, 224, 220, 54, "CHECK GITHUB", check_cb);
    s_install_button = make_button(s_ota_panel, 260, 224, 220, 54, "INSTALL UPDATE", install_cb);
    s_confirm_button = make_button(s_ota_panel, 496, 224, 220, 54, "CONFIRM", confirm_cb);
    s_rollback_button = make_button(s_ota_panel, 24, 300, 220, 54, "ROLLBACK", rollback_cb);
    s_recovery_button = make_button(s_ota_panel, 260, 300, 220, 54, "FACTORY RECOVERY", recovery_cb);
    make_label(s_ota_panel, 496, 307, "Rollback/Confirm are enabled only\nfor PENDING_VERIFY candidates.", &lv_font_montserrat_18, 0x8B949E);

    show_status_panel();
    lv_screen_load(s_screen);
}

static void refresh_ui(void)
{
    ota_status_t ota;
    ota_manager_get_status(&ota);

    network_state_t network = network_manager_state();
    bool online = network == NETWORK_STATE_STA_ONLINE;

    lv_label_set_text_fmt(s_status_version, "Firmware %s", ota.current_version);
    lv_label_set_text_fmt(s_status_network, "Network: %s", network_manager_state_name());
    lv_label_set_text_fmt(s_status_ip, "IP: %s", network_manager_sta_ip());
    lv_label_set_text_fmt(s_status_partition, "Partition: %s", ota.running_partition);
    lv_label_set_text_fmt(s_status_image, "Image: %s", ota.image_state);

    if (network == NETWORK_STATE_AP_SETUP) {
        lv_label_set_text_fmt(s_status_hint,
                              "Provisioning AP: %s   Open http://192.168.4.1/ to save Wi-Fi.\nGitHub CHECK/INSTALL stays disabled until STA is online.",
                              network_manager_ap_ssid());
    } else if (online) {
        lv_label_set_text(s_status_hint,
                          "STA is online. Open the OTA tab to check GitHub or control rollback/recovery directly from this display.");
    } else {
        lv_label_set_text(s_status_hint, "Waiting for network manager...");
    }

    if (ota.pending_verify) {
        lv_obj_set_style_text_color(s_status_image, lv_color_hex(0xD29922), 0);
    } else if (strcmp(ota.image_state, "VALID") == 0 || strcmp(ota.image_state, "FACTORY") == 0) {
        lv_obj_set_style_text_color(s_status_image, lv_color_hex(0x3FB950), 0);
    } else {
        lv_obj_set_style_text_color(s_status_image, lv_color_hex(0xF0F6FC), 0);
    }

    lv_label_set_text_fmt(s_ota_versions,
                          "Installed %s   |   Available %s",
                          ota.current_version,
                          ota.available_version[0] ? ota.available_version : "-");
    lv_label_set_text_fmt(s_ota_state, "%s   |   %s / %s", ota.state, ota.running_partition, ota.image_state);
    lv_label_set_text(s_ota_message, ota.message);
    lv_bar_set_value(s_ota_progress, ota.progress_percent, LV_ANIM_OFF);
    lv_label_set_text_fmt(s_ota_progress_label, "%d%%", ota.progress_percent);

    set_button_enabled(s_check_button, online && !ota.busy);
    set_button_enabled(s_install_button, online && ota.update_available && !ota.busy);
    set_button_enabled(s_confirm_button, ota.pending_verify && !ota.busy);
    set_button_enabled(s_rollback_button, ota.pending_verify && !ota.busy);
    set_button_enabled(s_recovery_button, !ota.busy && strcmp(ota.running_partition, "factory") != 0);

    if (ota.pending_verify) {
        lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0xD29922), 0);
        if (s_first_refresh) show_ota_panel();
    } else if (strcmp(ota.state, "ERROR") == 0) {
        lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0xF85149), 0);
    } else {
        lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0x58A6FF), 0);
    }

    s_first_refresh = false;
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

static void ui_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Starting 800x480 LVGL OTA UI on core %d", xPortGetCoreID());
    ESP_LOGI(TAG, "Internal heap before display init: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "PSRAM before display init: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    init_display();
    init_touch();
    init_lvgl();
    create_ui();
    refresh_ui();
    gpio_set_level(LCD_PIN_BL, 1);

    ESP_LOGI(TAG, "Display OTA controls ready; UI stack high-water=%u bytes",
             (unsigned)uxTaskGetStackHighWaterMark(NULL));

    TickType_t last_refresh = 0;
    TickType_t last_stack_log = xTaskGetTickCount();
    while (true) {
        lv_timer_handler();
        TickType_t now = xTaskGetTickCount();
        if ((now - last_refresh) >= pdMS_TO_TICKS(250)) {
            refresh_ui();
            last_refresh = now;
        }
        if ((now - last_stack_log) >= pdMS_TO_TICKS(10000)) {
            ESP_LOGI(TAG, "UI task stack high-water=%u bytes",
                     (unsigned)uxTaskGetStackHighWaterMark(NULL));
            last_stack_log = now;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

esp_err_t display_ota_start(void)
{
    BaseType_t created = xTaskCreatePinnedToCore(ui_task,
                                                 "app06_ui",
                                                 APP_UI_TASK_STACK_SIZE,
                                                 NULL,
                                                 APP_UI_TASK_PRIORITY,
                                                 NULL,
                                                 APP_UI_TASK_CORE);
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
