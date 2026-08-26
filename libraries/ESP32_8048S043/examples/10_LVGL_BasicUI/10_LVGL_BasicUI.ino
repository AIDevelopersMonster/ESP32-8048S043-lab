/*
  ESP32-8048S043 Lab / 10_LVGL_BasicUI

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    First BSP-style LVGL UI test for the ESP32-8048S043 board.

  What this example checks:
    - Arduino_GFX RGB display driver under LVGL;
    - LVGL draw buffer allocation, preferably in PSRAM;
    - LVGL flush callback to the 800x480 RGB panel;
    - ESP32_8048S043_Touch BSP driver as LVGL pointer input;
    - interactive button + counter;
    - slider-controlled backlight PWM;
    - runtime ALIVE lines while LVGL is active.

  Dependency:
    - Arduino_GFX_Library by moononournation;
    - LVGL 8.x from Arduino Library Manager.
*/

#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043.h>

#include "esp_heap_caps.h"

using namespace esp32_8048s043::pins;

#if LV_COLOR_DEPTH != 16
#error "10_LVGL_BasicUI expects LV_COLOR_DEPTH == 16 for Arduino_GFX RGB565 flush. Set LVGL color depth to 16."
#endif

static const char *const SKETCH_ID = "10LVGL-BSP1-240826C";

static constexpr int LCD_W = LCD_WIDTH;
static constexpr int LCD_H = LCD_HEIGHT;
static constexpr int LCD_PCLK_HZ = 16000000;

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 40;
static constexpr uint32_t LVGL_TICK_PERIOD_MS = 5;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;

static Arduino_ESP32RGBPanel *rgbPanel = nullptr;
static Arduino_RGB_Display *gfx = nullptr;
static ESP32_8048S043_Touch touch;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t indevDrv;
static lv_color_t *lvBuf1 = nullptr;
static lv_color_t *lvBuf2 = nullptr;

static bool displayOk = false;
static bool touchOk = false;
static bool lvglOk = false;
static bool uiOk = false;

static uint32_t buttonClicks = 0;
static uint32_t lvglLoops = 0;
static uint32_t lastAliveMs = 0;
static uint32_t lastLvTickMs = 0;
static int lastSliderLogValue = -1;
static uint32_t lastSliderLogMs = 0;

static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *sliderLabel = nullptr;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printBuildProfile() {
  Serial.println("[BUILD PROFILE]");
  Serial.printf("%-28s: %s\n", "SKETCH_ID", SKETCH_ID);

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

#ifdef CONFIG_IDF_TARGET
  Serial.printf("%-28s: \"%s\"\n", "CONFIG_IDF_TARGET", CONFIG_IDF_TARGET);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_IDF_TARGET");
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
  Serial.printf("%-28s: %d\n", "CONFIG_IDF_TARGET_ESP32S3", CONFIG_IDF_TARGET_ESP32S3);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_IDF_TARGET_ESP32S3");
#endif

#ifdef BOARD_HAS_PSRAM
  Serial.printf("%-28s: defined\n", "BOARD_HAS_PSRAM");
#else
  Serial.printf("%-28s: not defined\n", "BOARD_HAS_PSRAM");
#endif

  Serial.printf("%-28s: %d.%d.%d\n", "LVGL version", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);
}

static void printRuntimeBaseline() {
  Serial.println("[RUNTIME]");
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %s rev %u\n", "Chip", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("%-28s: %lu MHz\n", "CPU frequency", static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %lu bytes\n", "Free PSRAM", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("%-28s: %lu bytes\n", "Free heap", static_cast<unsigned long>(ESP.getFreeHeap()));
}

static void setBacklightDuty(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  static bool attached = false;
  if (!attached) {
    attached = ledcAttach(BACKLIGHT, 5000, 8);
    if (!attached) {
      Serial.println("[WARN] ledcAttach() failed; using digital backlight fallback");
      pinMode(BACKLIGHT, OUTPUT);
    }
  }

  if (attached) {
    ledcWrite(BACKLIGHT, duty);
  } else {
    digitalWrite(BACKLIGHT, duty > 0 ? HIGH : LOW);
  }
#else
  static bool attached = false;
  static constexpr int channel = 0;
  if (!attached) {
    ledcSetup(channel, 5000, 8);
    ledcAttachPin(BACKLIGHT, channel);
    attached = true;
  }
  ledcWrite(channel, duty);
#endif
}

static bool initDisplay() {
  Serial.println("[DISPLAY INIT]");
  setBacklightDuty(255);

  rgbPanel = new Arduino_ESP32RGBPanel(
    RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
    RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
    RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
    RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
    1 /* hsync polarity */, 40 /* hsync front porch */, 48 /* hsync pulse width */, 40 /* hsync back porch */,
    1 /* vsync polarity */, 13 /* vsync front porch */, 3 /* vsync pulse width */, 29 /* vsync back porch */,
    1 /* pclk active neg */, LCD_PCLK_HZ /* prefer speed */
  );

  gfx = new Arduino_RGB_Display(
    LCD_W,
    LCD_H,
    rgbPanel,
    0 /* rotation */,
    true /* auto_flush */
  );

  if (!gfx->begin()) {
    Serial.println("[FAIL] gfx->begin()");
    return false;
  }

  gfx->fillScreen(0x0000);
  Serial.println("[PASS] gfx->begin()");
  return true;
}

static bool initTouch() {
  Serial.println("[TOUCH BSP INIT]");

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
  return true;
}

static void lvglFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  if (!gfx) {
    lv_disp_flush_ready(disp);
    return;
  }

  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(colorP), w, h);
  lv_disp_flush_ready(disp);
}

static void lvglTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;

  ESP32_8048S043_TouchPoint point;
  if (touchOk && touch.read(point) && point.touched) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void *allocDrawBuffer(size_t bytes, const char *name) {
  void *ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (ptr) {
    Serial.printf("[PASS] %s allocated in PSRAM: %u bytes\n", name, static_cast<unsigned int>(bytes));
    return ptr;
  }

  ptr = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (ptr) {
    Serial.printf("[WARN] %s allocated in internal RAM: %u bytes\n", name, static_cast<unsigned int>(bytes));
    return ptr;
  }

  Serial.printf("[FAIL] %s allocation failed: %u bytes\n", name, static_cast<unsigned int>(bytes));
  return nullptr;
}

static bool initLvgl() {
  Serial.println("[LVGL INIT]");
  lv_init();

  const size_t pixelCount = LCD_W * LVGL_BUFFER_LINES;
  const size_t bufferBytes = pixelCount * sizeof(lv_color_t);

  lvBuf1 = static_cast<lv_color_t *>(allocDrawBuffer(bufferBytes, "lvBuf1"));
  lvBuf2 = static_cast<lv_color_t *>(allocDrawBuffer(bufferBytes, "lvBuf2"));

  if (!lvBuf1 || !lvBuf2) {
    return false;
  }

  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, pixelCount);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_W;
  dispDrv.ver_res = LCD_H;
  dispDrv.flush_cb = lvglFlush;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  if (touchOk) {
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = lvglTouchRead;
    lv_indev_drv_register(&indevDrv);
    Serial.println("[PASS] LVGL touch input registered through BSP");
  } else {
    Serial.println("[WARN] LVGL touch input not registered because BSP touch init failed");
  }

  lastLvTickMs = millis();
  Serial.println("[PASS] LVGL display driver registered");
  return true;
}

static void updateCounterLabel() {
  if (!counterLabel) {
    return;
  }
  lv_label_set_text_fmt(counterLabel, "Button clicks: %lu", static_cast<unsigned long>(buttonClicks));
}

static void buttonEvent(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    ++buttonClicks;
    Serial.printf("[LVGL] fw=%s Button clicked: %lu\n", SKETCH_ID, static_cast<unsigned long>(buttonClicks));
    updateCounterLabel();
  }
}

static void sliderEvent(lv_event_t *event) {
  lv_obj_t *slider = lv_event_get_target(event);
  const int value = static_cast<int>(lv_slider_get_value(slider));
  setBacklightDuty(static_cast<uint8_t>(value));

  if (sliderLabel) {
    lv_label_set_text_fmt(sliderLabel, "Backlight PWM: %d / 255", value);
  }

  const uint32_t now = millis();
  if (lastSliderLogValue < 0 || abs(value - lastSliderLogValue) >= 8 || now - lastSliderLogMs > 700) {
    lastSliderLogValue = value;
    lastSliderLogMs = now;
    Serial.printf("[LVGL] fw=%s Backlight slider: %d\n", SKETCH_ID, value);
  }
}

static void createUi() {
  Serial.println("[UI INIT]");

  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);

  lv_obj_t *title = lv_label_create(screen);
  lv_label_set_text(title, "ESP32-8048S043 / LVGL BasicUI");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  lv_obj_t *statusLabel = lv_label_create(screen);
  lv_label_set_text_fmt(statusLabel,
                        "FW %s | BSP touch | PSRAM: %lu MB | GT911: %s",
                        SKETCH_ID,
                        static_cast<unsigned long>(ESP.getPsramSize() / (1024UL * 1024UL)),
                        touchOk ? "OK" : "OPEN");
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xC8D8E4), 0);
  lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 55);

  lv_obj_t *panel = lv_obj_create(screen);
  lv_obj_set_size(panel, 700, 310);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 25);
  lv_obj_set_style_radius(panel, 16, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x1C2E3A), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x3E92CC), 0);
  lv_obj_set_style_border_width(panel, 2, 0);

  lv_obj_t *hint = lv_label_create(panel);
  lv_label_set_text(hint, "BSP touch path: GT911 -> ESP32_8048S043_Touch -> LVGL pointer");
  lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 18);

  lv_obj_t *button = lv_btn_create(panel);
  lv_obj_set_size(button, 220, 72);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 65);
  lv_obj_add_event_cb(button, buttonEvent, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *buttonLabel = lv_label_create(button);
  lv_label_set_text(buttonLabel, "Tap me");
  lv_obj_center(buttonLabel);

  counterLabel = lv_label_create(panel);
  lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(counterLabel, LV_ALIGN_TOP_MID, 0, 155);
  updateCounterLabel();

  lv_obj_t *slider = lv_slider_create(panel);
  lv_obj_set_width(slider, 520);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 205);
  lv_slider_set_range(slider, 16, 255);
  lv_slider_set_value(slider, 255, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, sliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  sliderLabel = lv_label_create(panel);
  lv_label_set_text(sliderLabel, "Backlight PWM: 255 / 255");
  lv_obj_set_style_text_color(sliderLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(sliderLabel, LV_ALIGN_TOP_MID, 0, 245);

  lv_obj_t *footer = lv_label_create(screen);
  lv_label_set_text(footer, "10_LVGL_BasicUI: BSP-style touch bridge, display/backlight still local");
  lv_obj_set_style_text_color(footer, lv_color_hex(0x8DA9C4), 0);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -12);

  uiOk = true;
  Serial.println("[PASS] LVGL UI objects created");
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32-8048S043 Lab / 10_LVGL_BasicUI");
  Serial.println(" LVGL 8 BSP-touch basic UI validation");
  Serial.printf(" Firmware ID: %s\n", SKETCH_ID);
  Serial.println("============================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("----------------------------------------------------------------");
  Serial.println("Mode   : RGB display + ESP32_8048S043_Touch BSP + LVGL button/slider");
  Serial.println("Target : first cleaner HMI shell after low-level hardware tests");
  Serial.println("Serial : 115200 baud");
  Serial.println("----------------------------------------------------------------");

  printBuildProfile();
  printDivider();
  printRuntimeBaseline();
  printDivider();

  displayOk = initDisplay();
  if (!displayOk) {
    Serial.println("[FATAL] Display init failed; LVGL test stopped");
    return;
  }

  printDivider();
  touchOk = initTouch();

  printDivider();
  lvglOk = initLvgl();
  if (!lvglOk) {
    Serial.println("[FATAL] LVGL init failed; LVGL test stopped");
    return;
  }

  createUi();

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" LVGL BASIC UI READY");
  Serial.printf(" Firmware ID: %s\n", SKETCH_ID);
  Serial.println(" Touch path is now hidden behind ESP32_8048S043_Touch BSP.");
  Serial.println("============================================================");
}

void loop() {
  const uint32_t now = millis();

  if (lvglOk) {
    const uint32_t elapsed = now - lastLvTickMs;
    if (elapsed >= LVGL_TICK_PERIOD_MS) {
      lv_tick_inc(elapsed);
      lastLvTickMs = now;
    }

    lv_timer_handler();
    ++lvglLoops;
  }

  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] fw=%s uptime=%lus display=%s touch=%s lvgl=%s ui=%s clicks=%lu accepted=%lu filtered=%lu statusReads=%lu ready=%lu zeroReady=%lu lastStatus=0x%02X readFail=%lu pointFail=%lu lvglLoops=%lu freeHeap=%lu psram=%lu freePsram=%lu\n",
                  SKETCH_ID,
                  static_cast<unsigned long>(now / 1000),
                  displayOk ? "OK" : "FAIL",
                  touchOk ? "OK" : "OPEN",
                  lvglOk ? "OK" : "FAIL",
                  uiOk ? "OK" : "FAIL",
                  static_cast<unsigned long>(buttonClicks),
                  static_cast<unsigned long>(touch.acceptedPoints()),
                  static_cast<unsigned long>(touch.filteredUpdates()),
                  static_cast<unsigned long>(touch.statusReads()),
                  static_cast<unsigned long>(touch.readyReads()),
                  static_cast<unsigned long>(touch.zeroPointReadyReads()),
                  static_cast<unsigned int>(touch.lastStatus()),
                  static_cast<unsigned long>(touch.readFailures()),
                  static_cast<unsigned long>(touch.pointFailures()),
                  static_cast<unsigned long>(lvglLoops),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(5);
}
