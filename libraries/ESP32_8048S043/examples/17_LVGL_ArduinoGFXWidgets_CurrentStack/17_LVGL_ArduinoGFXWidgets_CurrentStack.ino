/*
  ESP32-8048S043 Lab / 17_LVGL_ArduinoGFXWidgets_CurrentStack

  Purpose:
    Controlled forward-port of the physically successful wegi1 LVGL Widgets
    reference onto the current ESP32-8048S043-lab software stack.

  This is NOT a copy of the third-party application source.
  The sketch independently wires the standard LVGL 8 widgets demo to:

      current Arduino-ESP32 / ESP-IDF 5.x
      current Arduino_GFX
      ESP32-8048S043 Lab custom board profile
      ESP32_8048S043 BSP GT911 driver

  Experimental invariants retained from the known-good historical reference:
    - 800x480 RGB panel;
    - Arduino_GFX RGB path;
    - partial LVGL area flush;
    - 14 MHz PCLK;
    - HSYNC 8/4/8;
    - VSYNC 8/4/8;
    - pclk_active_neg = 1;
    - LVGL 8 standard widgets demo;
    - 5 ms loop cadence.

  Deliberately modernized variables:
    - current board/core/IDF;
    - current Arduino_GFX;
    - current LVGL 8.x;
    - our validated GT911 BSP instead of TAMC_GT911.

  Required LVGL configuration:
    LV_COLOR_DEPTH == 16
    LV_USE_DEMO_WIDGETS == 1
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <demos/lv_demos.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

#include "esp_heap_caps.h"

using namespace esp32_8048s043::pins;

#if LV_COLOR_DEPTH != 16
#error "Test 17 requires LV_COLOR_DEPTH == 16."
#endif

#if !LV_USE_DEMO_WIDGETS
#error "Test 17 requires LV_USE_DEMO_WIDGETS == 1 in lv_conf.h."
#endif

static const char *const SKETCH_ID = "17-LVGL-WIDGETS-CURRENT1-240829A";

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LCD_PCLK_HZ = 14000000;
static constexpr uint32_t LVGL_BUFFER_PIXELS = (LCD_WIDTH * LCD_HEIGHT) / 4;
static constexpr uint32_t LOOP_DELAY_MS = 5;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;

static Arduino_ESP32RGBPanel *rgbPanel = nullptr;
static Arduino_RGB_Display *gfx = nullptr;
static ESP32_8048S043_Touch touch;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t indevDrv;
static lv_color_t *lvBuf = nullptr;

static bool displayOk = false;
static bool touchOk = false;
static bool lvglOk = false;

static uint32_t lastTickMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t loopCount = 0;
static uint32_t flushCount = 0;
static uint32_t indevReadCount = 0;
static uint32_t touchPressCount = 0;
static uint32_t touchReleaseCount = 0;
static bool lastTouchDown = false;
static uint16_t lastTouchX = 0;
static uint16_t lastTouchY = 0;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printBanner() {
  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / Test 17");
  Serial.println(" LVGL Widgets -> Arduino_GFX partial flush -> current stack");
  Serial.println("================================================================");
  Serial.printf("%-28s: %s\n", "Firmware ID", SKETCH_ID);
#ifdef ARDUINO_BOARD
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_BOARD", ARDUINO_BOARD);
#endif
#ifdef ARDUINO_VARIANT
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_VARIANT", ARDUINO_VARIANT);
#endif
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %d.%d.%d\n", "LVGL version", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);
  Serial.printf("%-28s: %d\n", "LV_USE_DEMO_WIDGETS", LV_USE_DEMO_WIDGETS);
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  printDivider();
  Serial.printf("%-28s: Arduino_GFX partial-area\n", "Display path");
  Serial.printf("%-28s: %u Hz\n", "PCLK", static_cast<unsigned>(LCD_PCLK_HZ));
  Serial.printf("%-28s: 8/4/8\n", "HSYNC porches");
  Serial.printf("%-28s: 8/4/8\n", "VSYNC porches");
  Serial.printf("%-28s: %lu pixels (~%lu bytes)\n", "LVGL draw buffer",
                static_cast<unsigned long>(LVGL_BUFFER_PIXELS),
                static_cast<unsigned long>(LVGL_BUFFER_PIXELS * sizeof(lv_color_t)));
  Serial.printf("%-28s: ESP32_8048S043_Touch BSP\n", "GT911 path");
  printDivider();
}

static bool initDisplay() {
  Serial.println("[DISPLAY INIT]");

  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, LOW);

  rgbPanel = new Arduino_ESP32RGBPanel(
    RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
    RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
    RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
    RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
    0 /* hsync polarity */, 8 /* front */, 4 /* pulse */, 8 /* back */,
    0 /* vsync polarity */, 8 /* front */, 4 /* pulse */, 8 /* back */,
    1 /* pclk active neg */, LCD_PCLK_HZ
  );

  gfx = new Arduino_RGB_Display(
    LCD_WIDTH,
    LCD_HEIGHT,
    rgbPanel,
    0 /* rotation */,
    true /* auto_flush */
  );

  if (!gfx->begin()) {
    Serial.println("[FAIL] gfx->begin()");
    return false;
  }

  gfx->fillScreen(0x0000);
  displayOk = true;
  Serial.println("[PASS] gfx->begin()");
  return true;
}

static bool initTouch() {
  Serial.println("[TOUCH INIT]");

  if (!touch.begin(Wire)) {
    Serial.println("[FAIL] ESP32_8048S043_Touch::begin()");
    return false;
  }

  touchOk = true;
  Serial.printf("[PASS] GT911 BSP addr=0x%02X fw=0x%04X res=%ux%u int=%d\n",
                touch.address(),
                touch.firmwareVersion(),
                touch.resolutionX(),
                touch.resolutionY(),
                touch.interruptLevel());
  return true;
}

static void lvglFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  flushCount++;

  if (gfx) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    gfx->draw16bitRGBBitmap(
      area->x1,
      area->y1,
      reinterpret_cast<uint16_t *>(colorP),
      w,
      h
    );
  }

  lv_disp_flush_ready(disp);
}

static void lvglTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  indevReadCount++;

  ESP32_8048S043_TouchPoint point;
  const bool readOk = touchOk && touch.read(point);

  if (readOk && point.touched) {
    lastTouchX = point.x;
    lastTouchY = point.y;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;

    if (!lastTouchDown) {
      lastTouchDown = true;
      touchPressCount++;
      Serial.printf("[TOUCH PRESS] #%lu mapped=(%u,%u) raw=(%u,%u)\n",
                    static_cast<unsigned long>(touchPressCount),
                    point.x, point.y, point.rawX, point.rawY);
    }
    return;
  }

  data->state = LV_INDEV_STATE_REL;
  data->point.x = lastTouchX;
  data->point.y = lastTouchY;

  if (lastTouchDown) {
    lastTouchDown = false;
    touchReleaseCount++;
    Serial.printf("[TOUCH RELEASE] #%lu last=(%u,%u)\n",
                  static_cast<unsigned long>(touchReleaseCount),
                  lastTouchX, lastTouchY);
  }
}

static lv_color_t *allocateHistoricalSizeBuffer() {
  const size_t bytes = static_cast<size_t>(LVGL_BUFFER_PIXELS) * sizeof(lv_color_t);

  // First try internal RAM because the historical known-good reference used it.
  lv_color_t *ptr = static_cast<lv_color_t *>(
    heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
  );

  if (ptr) {
    Serial.printf("[PASS] LVGL buffer in internal RAM: %u bytes at %p\n",
                  static_cast<unsigned>(bytes), ptr);
    return ptr;
  }

  // Current stack fallback: preserve the buffer size but allow PSRAM if current
  // memory layout cannot satisfy the historical internal-RAM request.
  ptr = static_cast<lv_color_t *>(
    heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
  );

  if (ptr) {
    Serial.printf("[WARN] Internal allocation unavailable; LVGL buffer in PSRAM: %u bytes at %p\n",
                  static_cast<unsigned>(bytes), ptr);
    return ptr;
  }

  Serial.printf("[FAIL] LVGL draw buffer allocation failed: %u bytes\n",
                static_cast<unsigned>(bytes));
  return nullptr;
}

static bool initLvgl() {
  Serial.println("[LVGL INIT]");
  lv_init();

  lvBuf = allocateHistoricalSizeBuffer();
  if (!lvBuf) {
    return false;
  }

  lv_disp_draw_buf_init(&drawBuf, lvBuf, nullptr, LVGL_BUFFER_PIXELS);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_WIDTH;
  dispDrv.ver_res = LCD_HEIGHT;
  dispDrv.flush_cb = lvglFlush;
  dispDrv.draw_buf = &drawBuf;

  if (!lv_disp_drv_register(&dispDrv)) {
    Serial.println("[FAIL] LVGL display registration");
    return false;
  }
  Serial.println("[PASS] LVGL partial display driver registered");

  if (touchOk) {
    lv_indev_drv_init(&indevDrv);
    indevDrv.type = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = lvglTouchRead;
    if (!lv_indev_drv_register(&indevDrv)) {
      Serial.println("[FAIL] LVGL pointer registration");
      return false;
    }
    Serial.println("[PASS] LVGL pointer registered through BSP GT911");
  }

  lvglOk = true;
  lastTickMs = millis();
  return true;
}

static void startWidgetsDemo() {
  Serial.println("[UI INIT] lv_demo_widgets()");
  lv_demo_widgets();

  // Give LVGL several cycles to produce the initial frame before enabling BL.
  for (int i = 0; i < 6; ++i) {
    const uint32_t now = millis();
    const uint32_t elapsed = now - lastTickMs;
    if (elapsed > 0) {
      lv_tick_inc(elapsed);
      lastTickMs = now;
    }
    lv_timer_handler();
    delay(5);
  }

  digitalWrite(BACKLIGHT, HIGH);
  Serial.println("[PASS] Backlight ON after initial LVGL render");
  Serial.println("[READY] Judge this visually against the historical wegi1 Widgets PASS.");
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  printBanner();

  if (!initDisplay()) {
    Serial.println("[STOP] Display init failed");
    return;
  }

  initTouch();

  if (!initLvgl()) {
    Serial.println("[STOP] LVGL init failed");
    return;
  }

  startWidgetsDemo();
}

void loop() {
  if (!displayOk || !lvglOk) {
    delay(1000);
    return;
  }

  const uint32_t now = millis();
  const uint32_t elapsed = now - lastTickMs;
  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    lastTickMs = now;
  }

  lv_timer_handler();
  loopCount++;

  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] ms=%lu loop=%lu flush=%lu indev=%lu press=%lu release=%lu freeHeap=%lu freePsram=%lu\n",
                  static_cast<unsigned long>(now),
                  static_cast<unsigned long>(loopCount),
                  static_cast<unsigned long>(flushCount),
                  static_cast<unsigned long>(indevReadCount),
                  static_cast<unsigned long>(touchPressCount),
                  static_cast<unsigned long>(touchReleaseCount),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(LOOP_DELAY_MS);
}
