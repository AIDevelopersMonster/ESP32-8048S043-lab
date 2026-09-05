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

#define TAG "APP01"

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

/* GT911 config version 65 observed on the physical board reports a 480x272 touch space. */
#define TOUCH_RAW_X_MAX 479
#define TOUCH_RAW_Y_MAX 271

#define CARD_COUNT 6
#define PROFILE_COUNT 3

static esp_lcd_panel_handle_t s_panel = NULL;
static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_touch_io = NULL;
static esp_lcd_touch_handle_t s_touch = NULL;

static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_profile_label = NULL;
static uint8_t s_profile_index = 0;

typedef struct {
    const char *label;
    const char *visual;
    const char *command;
    uint32_t color;
} card_definition_t;

typedef struct {
    const char *name;
    card_definition_t cards[CARD_COUNT];
} panel_profile_t;

typedef struct {
    uint8_t index;
    lv_obj_t *button;
    lv_obj_t *visual_label;
    lv_obj_t *caption_label;
    const card_definition_t *definition;
} card_slot_t;

static card_slot_t s_slots[CARD_COUNT];

static const panel_profile_t s_profiles[PROFILE_COUNT] = {
    {
        .name = "HOME",
        .cards = {
            {"Power",    LV_SYMBOL_POWER,    "POWER",    0x2E7D32},
            {"Media",    LV_SYMBOL_AUDIO,    "MEDIA",    0x6A1B9A},
            {"Game",     LV_SYMBOL_PLAY,     "GAME",     0x00838F},
            {"Social",   LV_SYMBOL_WIFI,     "SOCIAL",   0xD84315},
            {"Work",     LV_SYMBOL_EDIT,     "WORK",     0x1565C0},
            {"Settings", LV_SYMBOL_SETTINGS, "SETTINGS", 0x455A64},
        },
    },
    {
        .name = "MEDIA",
        .cards = {
            {"Play",    LV_SYMBOL_PLAY,       "PLAY",     0x2E7D32},
            {"Pause",   LV_SYMBOL_PAUSE,      "PAUSE",    0x6A1B9A},
            {"Previous",LV_SYMBOL_PREV,       "PREVIOUS", 0x1565C0},
            {"Next",    LV_SYMBOL_NEXT,       "NEXT",     0x00838F},
            {"Vol -",   LV_SYMBOL_VOLUME_MID, "VOL_DOWN", 0xD84315},
            {"Vol +",   LV_SYMBOL_VOLUME_MAX, "VOL_UP",   0x455A64},
        },
    },
    {
        .name = "SYSTEM",
        .cards = {
            {"Home",      LV_SYMBOL_HOME,      "HOME",      0x2E7D32},
            {"Wi-Fi",     LV_SYMBOL_WIFI,      "WIFI",      0x1565C0},
            {"Bluetooth", LV_SYMBOL_BLUETOOTH, "BLUETOOTH", 0x00838F},
            {"USB",       LV_SYMBOL_USB,       "USB",       0x6A1B9A},
            {"SD Card",   LV_SYMBOL_SD_CARD,   "SD_CARD",   0xD84315},
            {"Settings",  LV_SYMBOL_SETTINGS,  "SETTINGS",  0x455A64},
        },
    },
};

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

static void card_apply_definition(card_slot_t *slot, const card_definition_t *definition)
{
    slot->definition = definition;
    lv_label_set_text(slot->visual_label, definition->visual);
    lv_label_set_text(slot->caption_label, definition->label);
    lv_obj_set_style_bg_color(slot->button, lv_color_hex(definition->color), LV_PART_MAIN);
}

static void apply_profile(uint8_t profile_index)
{
    if (profile_index >= PROFILE_COUNT) profile_index = 0;
    s_profile_index = profile_index;

    const panel_profile_t *profile = &s_profiles[s_profile_index];
    for (uint8_t i = 0; i < CARD_COUNT; i++) {
        card_apply_definition(&s_slots[i], &profile->cards[i]);
    }

    if (s_profile_label) {
        lv_label_set_text_fmt(s_profile_label, "PROFILE: %s  >", profile->name);
    }
    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label, "Profile %s ready", profile->name);
    }

    printf("PROFILE:%s\n", profile->name);
    fflush(stdout);
}

static void card_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    card_slot_t *slot = (card_slot_t *)lv_event_get_user_data(event);
    if (!slot || !slot->definition) return;

    printf("CARD:%s\n", slot->definition->command);
    fflush(stdout);

    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label,
                              "Slot %u: %s -> %s",
                              (unsigned)(slot->index + 1),
                              slot->definition->label,
                              slot->definition->command);
    }
}

static void profile_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    apply_profile((uint8_t)((s_profile_index + 1) % PROFILE_COUNT));
}

static lv_obj_t *create_card(lv_obj_t *parent, uint8_t index, int32_t x, int32_t y)
{
    card_slot_t *slot = &s_slots[index];
    slot->index = index;

    lv_obj_t *button = lv_button_create(parent);
    slot->button = button;
    lv_obj_set_size(button, 236, 160);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_radius(button, 22, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(button, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    lv_obj_set_style_opa(button, LV_OPA_70, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, card_event_cb, LV_EVENT_CLICKED, slot);

    /* Decorative children are explicitly non-clickable. The whole card is the hit target. */
    lv_obj_t *visual = lv_label_create(button);
    slot->visual_label = visual;
    lv_obj_remove_flag(visual, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(visual, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(visual, lv_color_white(), 0);
    lv_obj_align(visual, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *caption = lv_label_create(button);
    slot->caption_label = caption;
    lv_obj_remove_flag(caption, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(caption, lv_color_white(), 0);
    lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -24);

    return button;
}

static void create_ui(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101418), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Six-Card Serial Deck");
    lv_obj_remove_flag(title, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8EEF2), 0);
    lv_obj_set_pos(title, 24, 20);

    /* Make the profile selector visibly actionable and wide enough for all profile names. */
    lv_obj_t *profile_button = lv_button_create(screen);
    lv_obj_set_size(profile_button, 200, 48);
    lv_obj_set_pos(profile_button, 576, 8);
    lv_obj_set_style_radius(profile_button, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(profile_button, lv_color_hex(0x37474F), LV_PART_MAIN);
    lv_obj_set_style_border_width(profile_button, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(profile_button, lv_color_hex(0x78909C), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(profile_button, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(profile_button, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_opa(profile_button, LV_OPA_60, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(profile_button, profile_event_cb, LV_EVENT_CLICKED, NULL);

    s_profile_label = lv_label_create(profile_button);
    lv_obj_remove_flag(s_profile_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(s_profile_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_profile_label, lv_color_white(), 0);
    lv_obj_center(s_profile_label);

    const int32_t x_positions[3] = {24, 282, 540};
    const int32_t y_positions[2] = {68, 246};
    uint8_t slot = 0;
    for (uint8_t row = 0; row < 2; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            create_card(screen, slot++, x_positions[col], y_positions[row]);
        }
    }

    s_status_label = lv_label_create(screen);
    lv_obj_remove_flag(s_status_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xB0BEC5), 0);
    lv_obj_set_pos(s_status_label, 24, 438);

    lv_screen_load(screen);
    apply_profile(0);
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
        /* BGR-wired order validated on this board family. */
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
    ESP_LOGI(TAG, "Tap cards; press PROFILE to reassign all six slots.");

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
    ESP_LOGI(TAG, "App 01 - Six-Card Serial Deck");

    BaseType_t created = xTaskCreatePinnedToCore(app_ui_task,
                                                 "app01_ui",
                                                 APP_UI_TASK_STACK_SIZE,
                                                 NULL,
                                                 APP_UI_TASK_PRIORITY,
                                                 NULL,
                                                 APP_UI_TASK_CORE);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create app01_ui task");
        abort();
    }

    ESP_LOGI(TAG, "Dedicated UI task created; app_main returning");
}