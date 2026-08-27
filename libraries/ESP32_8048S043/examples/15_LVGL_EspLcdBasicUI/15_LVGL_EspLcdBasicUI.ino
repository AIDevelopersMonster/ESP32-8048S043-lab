/*
  ESP32-8048S043 Lab / 15_LVGL_EspLcdBasicUI

  Purpose:
    First combined LVGL 8 basic UI test over the native ESP-IDF esp_lcd RGB
    panel path with GT911 normalized BSP touch input.

  Why this exists:
    13_LVGL_EspLcdStatic proved that LVGL 8 can render a static screen over
    the native esp_lcd RGB panel path on Sample A.

    14_GT911_NormalizedTouch proved that the ESP32_8048S043_Touch BSP can
    detect the GT911 controller and normalize all 9 physical screen zones into
    the 800x480 display coordinate space.

    This example combines those two paths with the smallest possible LVGL UI:
    one central button, one counter label and one diagnostic status label.

  What this example checks:
    - esp_lcd_new_rgb_panel() panel initialization;
    - ESP-IDF RGB565 data bit order;
    - PSRAM RGB panel framebuffer path;
    - LVGL 8 draw buffers in PSRAM;
    - LVGL flush callback to esp_lcd_panel_draw_bitmap();
    - ESP32_8048S043_Touch BSP as LVGL pointer input;
    - one LVGL button press/click path;
    - minimal redraw area without slider or moving animation.

  What this example intentionally does NOT use:
    - Arduino_GFX;
    - slider/backlight interaction;
    - moving animation;
    - full dashboard redraw;
    - Wi-Fi/SD/BLE/Web/OTA.

  Font note:
    This sketch intentionally uses LV_FONT_DEFAULT only. Some Arduino LVGL
    installations expose only lv_font_montserrat_14 by default.
*/

#include <Arduino.h>
#include <lvgl.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

extern "C" {
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_system.h"
}

using namespace esp32_8048s043::pins;

#if LV_COLOR_DEPTH != 16
#error "15_LVGL_EspLcdBasicUI expects LV_COLOR_DEPTH == 16."
#endif

#define SKETCH_ID "15LVGL-ELCDT1-240827A"

#ifndef ESP_LCD_BASICUI_PCLK_HZ
#define ESP_LCD_BASICUI_PCLK_HZ 12500000
#endif

#ifndef ESP_LCD_BASICUI_DOUBLE_FB
#define ESP_LCD_BASICUI_DOUBLE_FB 1
#endif

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 80;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t STATUS_UPDATE_MS = 5000;
static constexpr uint32_t TOUCH_LOG_INTERVAL_MS = 180;

static esp_lcd_panel_handle_t panel = nullptr;
static ESP32_8048S043_Touch touch;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t indevDrv;
static lv_color_t *lvBuf1 = nullptr;
static lv_color_t *lvBuf2 = nullptr;

static lv_obj_t *button = nullptr;
static lv_obj_t *buttonLabel = nullptr;
static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *statusLabel = nullptr;
static lv_obj_t *touchLabel = nullptr;

static bool panelOk = false;
static bool touchOk = false;
static bool lvglOk = false;
static bool uiOk = false;
static bool lastTouchDown = false;

static uint32_t lastTickMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t lastStatusMs = 0;
static uint32_t lastTouchLogMs = 0;
static uint32_t loopCount = 0;
static uint32_t flushCount = 0;
static uint32_t flushPixels = 0;
static uint32_t indevReads = 0;
static uint32_t touchReports = 0;
static uint32_t touchReleases = 0;
static uint32_t pressedEvents = 0;
static uint32_t clickedEvents = 0;
static uint16_t lastTouchX = 0;
static uint16_t lastTouchY = 0;
static uint16_t lastRawX = 0;
static uint16_t lastRawY = 0;
static const char *lastZoneName = "NONE";

static const char *zoneName(uint16_t x, uint16_t y) {
  const uint8_t col = (x < LCD_WIDTH / 3) ? 0 : ((x < (LCD_WIDTH * 2) / 3) ? 1 : 2);
  const uint8_t row = (y < LCD_HEIGHT / 3) ? 0 : ((y < (LCD_HEIGHT * 2) / 3) ? 1 : 2);

  static const char *const names[3][3] = {
    {"TOP_LEFT", "TOP_CENTER", "TOP_RIGHT"},
    {"CENTER_LEFT", "CENTER", "CENTER_RIGHT"},
    {"BOTTOM_LEFT", "BOTTOM_CENTER", "BOTTOM_RIGHT"}
  };

  return names[row][col];
}

static void backlightOff() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, LOW);
}

static void backlightOn() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
}

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printBanner() {
  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 15_LVGL_EspLcdBasicUI");
  Serial.println(" LVGL 8 button UI over esp_lcd RGB + GT911 BSP touch");
  Serial.println("================================================================");
  Serial.printf("%-28s: %s\n", "Firmware ID", SKETCH_ID);
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", esp_get_idf_version());
#ifdef ARDUINO_BOARD
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_BOARD", ARDUINO_BOARD);
#else
  Serial.printf("%-28s: not defined\n", "ARDUINO_BOARD");
#endif
#ifdef ARDUINO_VARIANT
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_VARIANT", ARDUINO_VARIANT);
#else
  Serial.printf("%-28s: not defined\n", "ARDUINO_VARIANT");
#endif
  Serial.printf("%-28s: %d.%d.%d\n", "LVGL version", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %lu bytes\n", "Free PSRAM", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("%-28s: %lu bytes\n", "Free heap", static_cast<unsigned long>(ESP.getFreeHeap()));
  printDivider();
  Serial.printf("%-28s: LVGL button UI over esp_lcd RGB panel\n", "Mode");
  Serial.printf("%-28s: not used\n", "Arduino_GFX");
  Serial.printf("%-28s: ESP32_8048S043_Touch BSP\n", "GT911 touch");
  Serial.printf("%-28s: not used\n", "Moving animation");
  Serial.printf("%-28s: not used\n", "Slider");
  Serial.printf("%-28s: %dx%d\n", "Resolution", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("%-28s: %u Hz\n", "PCLK", (unsigned)ESP_LCD_BASICUI_PCLK_HZ);
  Serial.printf("%-28s: HSYNC 8/4/8, VSYNC 8/4/8\n", "Porches");
  Serial.printf("%-28s: true\n", "Framebuffer in PSRAM");
  Serial.printf("%-28s: %s\n", "Double framebuffer", ESP_LCD_BASICUI_DOUBLE_FB ? "true" : "false");
  Serial.printf("%-28s: LV_FONT_DEFAULT only\n", "Font mode");
  printDivider();
}

static void setDataPins(esp_lcd_rgb_panel_config_t &cfg) {
  // ESP-IDF rgb_panel data_gpio_nums[] are RGB565 bus bits.
  // DATA0..4 = B0..B4, DATA5..10 = G0..G5, DATA11..15 = R0..R4.
  cfg.data_gpio_nums[0]  = RGB_B0;
  cfg.data_gpio_nums[1]  = RGB_B1;
  cfg.data_gpio_nums[2]  = RGB_B2;
  cfg.data_gpio_nums[3]  = RGB_B3;
  cfg.data_gpio_nums[4]  = RGB_B4;
  cfg.data_gpio_nums[5]  = RGB_G0;
  cfg.data_gpio_nums[6]  = RGB_G1;
  cfg.data_gpio_nums[7]  = RGB_G2;
  cfg.data_gpio_nums[8]  = RGB_G3;
  cfg.data_gpio_nums[9]  = RGB_G4;
  cfg.data_gpio_nums[10] = RGB_G5;
  cfg.data_gpio_nums[11] = RGB_R0;
  cfg.data_gpio_nums[12] = RGB_R1;
  cfg.data_gpio_nums[13] = RGB_R2;
  cfg.data_gpio_nums[14] = RGB_R3;
  cfg.data_gpio_nums[15] = RGB_R4;
}

static bool initEspLcdPanel() {
  Serial.println("[DISPLAY INIT]");
  backlightOff();

  esp_lcd_rgb_panel_config_t cfg = {};
  cfg.clk_src = LCD_CLK_SRC_DEFAULT;
  cfg.timings.pclk_hz = ESP_LCD_BASICUI_PCLK_HZ;
  cfg.timings.h_res = LCD_WIDTH;
  cfg.timings.v_res = LCD_HEIGHT;
  cfg.timings.hsync_pulse_width = 4;
  cfg.timings.hsync_back_porch = 8;
  cfg.timings.hsync_front_porch = 8;
  cfg.timings.vsync_pulse_width = 4;
  cfg.timings.vsync_back_porch = 8;
  cfg.timings.vsync_front_porch = 8;
  cfg.timings.flags.hsync_idle_low = false;
  cfg.timings.flags.vsync_idle_low = false;
  cfg.timings.flags.de_idle_high = false;
  cfg.timings.flags.pclk_active_neg = true;
  cfg.timings.flags.pclk_idle_high = false;

  cfg.data_width = 16;
  cfg.bits_per_pixel = 0;
  cfg.num_fbs = ESP_LCD_BASICUI_DOUBLE_FB ? 2 : 1;
  cfg.bounce_buffer_size_px = 0;
  cfg.sram_trans_align = 0;
  cfg.psram_trans_align = 64;
  cfg.hsync_gpio_num = RGB_HSYNC;
  cfg.vsync_gpio_num = RGB_VSYNC;
  cfg.de_gpio_num = RGB_DE;
  cfg.pclk_gpio_num = RGB_PCLK;
  cfg.disp_gpio_num = GPIO_NUM_NC;
  setDataPins(cfg);

  cfg.flags.disp_active_low = 0;
  cfg.flags.refresh_on_demand = 0;
  cfg.flags.fb_in_psram = true;
  cfg.flags.double_fb = ESP_LCD_BASICUI_DOUBLE_FB ? true : false;
  cfg.flags.no_fb = 0;
#if ESP_IDF_VERSION_MAJOR >= 5
  cfg.flags.bb_invalidate_cache = 0;
#endif

  Serial.println("Calling esp_lcd_new_rgb_panel()...");
  esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &panel);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] esp_lcd_new_rgb_panel(): %s\n", esp_err_to_name(err));
    return false;
  }
  Serial.println("[PASS] esp_lcd_new_rgb_panel()");

  err = esp_lcd_panel_reset(panel);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] esp_lcd_panel_reset(): %s\n", esp_err_to_name(err));
    return false;
  }
  Serial.println("[PASS] esp_lcd_panel_reset()");

  err = esp_lcd_panel_init(panel);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] esp_lcd_panel_init(): %s\n", esp_err_to_name(err));
    return false;
  }
  Serial.println("[PASS] esp_lcd_panel_init()");

  panelOk = true;
  return true;
}

static bool initTouch() {
  Serial.println("[TOUCH INIT]");
  if (!touch.begin(Wire)) {
    Serial.println("[FAIL] ESP32_8048S043_Touch::begin()");
    return false;
  }

  Serial.printf("[PASS] ESP32_8048S043_Touch::begin() addr=0x%02X fw=0x%04X res=%ux%u int=%d\n",
                touch.address(),
                touch.firmwareVersion(),
                touch.resolutionX(),
                touch.resolutionY(),
                touch.interruptLevel());
  touchOk = true;
  return true;
}

static bool allocateLvglBuffers() {
  const size_t pixelCount = static_cast<size_t>(LCD_WIDTH) * LVGL_BUFFER_LINES;
  const size_t bytes = pixelCount * sizeof(lv_color_t);

  lvBuf1 = static_cast<lv_color_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  lvBuf2 = static_cast<lv_color_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  if (!lvBuf1 || !lvBuf2) {
    Serial.printf("[FAIL] LVGL buffer allocation failed: buf1=%p buf2=%p bytes=%u\n", lvBuf1, lvBuf2, (unsigned)bytes);
    return false;
  }

  Serial.printf("[PASS] lvBuf1 allocated in PSRAM: %u bytes at %p\n", (unsigned)bytes, lvBuf1);
  Serial.printf("[PASS] lvBuf2 allocated in PSRAM: %u bytes at %p\n", (unsigned)bytes, lvBuf2);
  return true;
}

static void lvglFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  if (!panel) {
    lv_disp_flush_ready(disp);
    return;
  }

  const int32_t x1 = area->x1;
  const int32_t y1 = area->y1;
  const int32_t x2 = area->x2 + 1;
  const int32_t y2 = area->y2 + 1;
  const uint32_t w = static_cast<uint32_t>(x2 - x1);
  const uint32_t h = static_cast<uint32_t>(y2 - y1);

  flushCount++;
  flushPixels += w * h;

  esp_lcd_panel_draw_bitmap(panel, x1, y1, x2, y2, colorP);
  lv_disp_flush_ready(disp);
}

static void lvglTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  indevReads++;

  ESP32_8048S043_TouchPoint point;
  const bool ok = touch.read(point);
  if (!ok) {
    data->state = LV_INDEV_STATE_REL;
    data->point.x = lastTouchX;
    data->point.y = lastTouchY;
    return;
  }

  if (point.touched) {
    lastTouchX = point.x;
    lastTouchY = point.y;
    lastRawX = point.rawX;
    lastRawY = point.rawY;
    lastZoneName = zoneName(point.x, point.y);
    touchReports++;

    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;

    const uint32_t now = millis();
    if (now - lastTouchLogMs >= TOUCH_LOG_INTERVAL_MS) {
      lastTouchLogMs = now;
      Serial.printf("[TOUCH] raw=(%3u,%3u) mapped=(%3u,%3u) zone=%s reports=%lu accepted=%lu filtered=%lu\n",
                    point.rawX,
                    point.rawY,
                    point.x,
                    point.y,
                    lastZoneName,
                    (unsigned long)touchReports,
                    (unsigned long)touch.acceptedPoints(),
                    (unsigned long)touch.filteredUpdates());
    }

    lastTouchDown = true;
    return;
  }

  data->state = LV_INDEV_STATE_REL;
  data->point.x = lastTouchX;
  data->point.y = lastTouchY;

  if (lastTouchDown) {
    lastTouchDown = false;
    touchReleases++;
    Serial.printf("[RELEASE] releases=%lu last=(%u,%u) zone=%s\n",
                  (unsigned long)touchReleases,
                  lastTouchX,
                  lastTouchY,
                  lastZoneName);
  }
}

static bool initLvgl() {
  Serial.println("[LVGL INIT]");
  lv_init();

  if (!allocateLvglBuffers()) {
    return false;
  }

  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, LCD_WIDTH * LVGL_BUFFER_LINES);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_WIDTH;
  dispDrv.ver_res = LCD_HEIGHT;
  dispDrv.flush_cb = lvglFlush;
  dispDrv.draw_buf = &drawBuf;
  dispDrv.user_data = panel;

  lv_disp_t *display = lv_disp_drv_register(&dispDrv);
  if (!display) {
    Serial.println("[FAIL] LVGL display driver register failed");
    return false;
  }
  Serial.println("[PASS] LVGL display driver registered");

  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvglTouchRead;
  lv_indev_t *indev = lv_indev_drv_register(&indevDrv);
  if (!indev) {
    Serial.println("[FAIL] LVGL pointer driver register failed");
    return false;
  }
  Serial.println("[PASS] LVGL GT911 pointer driver registered");

  lvglOk = true;
  return true;
}

static void updateButtonLabels() {
  char buf[96];
  snprintf(buf, sizeof(buf), "Clicks: %lu   Pressed: %lu", (unsigned long)clickedEvents, (unsigned long)pressedEvents);
  lv_label_set_text(counterLabel, buf);

  snprintf(buf, sizeof(buf), "Touch: %u,%u  raw:%u,%u  zone:%s", lastTouchX, lastTouchY, lastRawX, lastRawY, lastZoneName);
  lv_label_set_text(touchLabel, buf);
}

static void buttonEvent(lv_event_t *event) {
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_PRESSED) {
    pressedEvents++;
    Serial.printf("[BUTTON] pressed=%lu touch=(%u,%u) zone=%s flush=%lu\n",
                  (unsigned long)pressedEvents,
                  lastTouchX,
                  lastTouchY,
                  lastZoneName,
                  (unsigned long)flushCount);
    updateButtonLabels();
  }

  if (code == LV_EVENT_CLICKED) {
    clickedEvents++;
    Serial.printf("[BUTTON] clicked=%lu pressed=%lu touch=(%u,%u) zone=%s flush=%lu\n",
                  (unsigned long)clickedEvents,
                  (unsigned long)pressedEvents,
                  lastTouchX,
                  lastTouchY,
                  lastZoneName,
                  (unsigned long)flushCount);
    updateButtonLabels();
  }
}

static void createUi() {
  Serial.println("[UI INIT]");

  lv_obj_t *scr = lv_scr_act();
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101621), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "ESP32-8048S043 / LVGL esp_lcd + GT911");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  lv_obj_t *subtitle = lv_label_create(scr);
  lv_label_set_text(subtitle, "Minimal button test: no slider, no animation, no dashboard redraw");
  lv_obj_set_style_text_color(subtitle, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 44);

  lv_obj_t *card = lv_obj_create(scr);
  lv_obj_set_pos(card, 95, 90);
  lv_obj_set_size(card, 610, 295);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x151C2A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x3A465C), 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_pad_all(card, 16, 0);

  button = lv_btn_create(card);
  lv_obj_set_size(button, 310, 112);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 32);
  lv_obj_add_event_cb(button, buttonEvent, LV_EVENT_ALL, nullptr);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x0066CC), LV_PART_MAIN);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x00A2FF), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(button, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(button, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_radius(button, 10, LV_PART_MAIN);

  buttonLabel = lv_label_create(button);
  lv_label_set_text(buttonLabel, "Tap me");
  lv_obj_center(buttonLabel);

  counterLabel = lv_label_create(card);
  lv_label_set_text(counterLabel, "Clicks: 0   Pressed: 0");
  lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(counterLabel, LV_ALIGN_TOP_MID, 0, 166);

  touchLabel = lv_label_create(card);
  lv_label_set_text(touchLabel, "Touch: none yet");
  lv_obj_set_style_text_color(touchLabel, lv_color_hex(0xC8D2E6), 0);
  lv_obj_align(touchLabel, LV_ALIGN_TOP_MID, 0, 196);

  statusLabel = lv_label_create(card);
  lv_label_set_text(statusLabel, "Status: esp_lcd + LVGL + GT911 BSP ready");
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 230);

  lv_obj_t *footer = lv_label_create(scr);
  lv_label_set_text(footer, "Acceptance: button reacts, labels update, idle screen and border stay stable.");
  lv_obj_set_style_text_color(footer, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -14);

  lv_obj_t *border = lv_obj_create(scr);
  lv_obj_set_pos(border, 4, 4);
  lv_obj_set_size(border, LCD_WIDTH - 8, LCD_HEIGHT - 8);
  lv_obj_clear_flag(border, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(border, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(border, 2, 0);
  lv_obj_set_style_border_color(border, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(border, 0, 0);

  uiOk = true;
  Serial.println("[PASS] LVGL minimal button UI objects created");
}

static void updateStatus() {
  const uint32_t now = millis();
  if (now - lastStatusMs < STATUS_UPDATE_MS) {
    return;
  }
  lastStatusMs = now;

  char buf[160];
  snprintf(buf, sizeof(buf), "Status: uptime %lus | flush %lu | indev %lu | heap %lu | psram %lu",
           (unsigned long)(now / 1000),
           (unsigned long)flushCount,
           (unsigned long)indevReads,
           (unsigned long)ESP.getFreeHeap(),
           (unsigned long)ESP.getFreePsram());
  lv_label_set_text(statusLabel, buf);
}

static void tickLvgl() {
  const uint32_t now = millis();
  if (lastTickMs == 0) {
    lastTickMs = now;
    return;
  }

  const uint32_t elapsed = now - lastTickMs;
  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    lastTickMs = now;
  }
}

static void alive() {
  const uint32_t now = millis();
  if (now - lastAliveMs < ALIVE_INTERVAL_MS) {
    return;
  }
  lastAliveMs = now;

  Serial.printf("[ALIVE] fw=%s uptime=%lus panel=%s touch=%s lvgl=%s ui=%s clicked=%lu pressed=%lu reports=%lu releases=%lu flush=%lu indev=%lu loops=%lu readFail=%lu pointFail=%lu heap=%lu psram=%lu freePsram=%lu\n",
                SKETCH_ID,
                (unsigned long)(now / 1000),
                panelOk ? "OK" : "FAIL",
                touchOk ? "OK" : "FAIL",
                lvglOk ? "OK" : "FAIL",
                uiOk ? "OK" : "FAIL",
                (unsigned long)clickedEvents,
                (unsigned long)pressedEvents,
                (unsigned long)touchReports,
                (unsigned long)touchReleases,
                (unsigned long)flushCount,
                (unsigned long)indevReads,
                (unsigned long)loopCount,
                (unsigned long)touch.readFailures(),
                (unsigned long)touch.pointFailures(),
                (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getPsramSize(),
                (unsigned long)ESP.getFreePsram());
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  printBanner();

  if (!initEspLcdPanel()) {
    Serial.println("[STOP] esp_lcd panel init failed");
    while (true) {
      alive();
      delay(100);
    }
  }

  if (!initTouch()) {
    Serial.println("[STOP] GT911 BSP touch init failed");
    while (true) {
      alive();
      delay(100);
    }
  }

  if (!initLvgl()) {
    Serial.println("[STOP] LVGL init failed");
    while (true) {
      alive();
      delay(100);
    }
  }

  createUi();
  updateButtonLabels();

  // Force first render before the backlight is enabled.
  for (int i = 0; i < 8; ++i) {
    tickLvgl();
    lv_timer_handler();
    delay(5);
  }

  backlightOn();
  Serial.println("[PASS] Backlight ON after LVGL first draw");
  Serial.println("[READY] Tap the central button. Watch button reaction, labels and border stability.");
}

void loop() {
  loopCount++;
  tickLvgl();
  updateStatus();
  lv_timer_handler();
  alive();
  delay(5);
}
