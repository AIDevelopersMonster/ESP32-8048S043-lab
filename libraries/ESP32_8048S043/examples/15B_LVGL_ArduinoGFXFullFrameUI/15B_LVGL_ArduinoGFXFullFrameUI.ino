/*
  ESP32-8048S043 Lab / 15B_LVGL_ArduinoGFXFullFrameUI

  Purpose:
    A/B comparison twin for 15_LVGL_EspLcdBasicUI.

    Test 15 proved that the same board, GT911 BSP and LVGL 8 button path are
    functional over native esp_lcd, but periodic redraw and press-time jitter
    were visually unacceptable.

    A physically tested third-party firmware for this board family showed a
    different redraw mechanism that behaved well on Sample A:

      LVGL full-screen/direct buffer
          -> Arduino_GFX
          -> full 800x480 RGB565 frame push in the main loop

    This sketch independently reimplements that architectural mechanism using
    our own pin constants, our own GT911 BSP and our own UI. No third-party
    application source or generated UI code is copied here.

  Key experimental rule:
    Keep the input path and UI behavior close to test 15, but replace the
    display transport/redraw strategy.

  Display mechanism:
    - Arduino_ESP32RGBPanel + Arduino_RGB_Display;
    - 800x480 RGB565;
    - one full-screen LVGL buffer in PSRAM (~768000 bytes);
    - LVGL 8 direct_mode;
    - LVGL flush callback only acknowledges completion;
    - the complete framebuffer is pushed with draw16bitRGBBitmap() each loop.

  Touch mechanism:
    - ESP32_8048S043_Touch BSP;
    - normalized 800x480 coordinates;
    - LVGL pointer input.

  UI stress:
    - normal visible LVGL button pressed state;
    - Pressed and Clicks counters;
    - touch/status labels;
    - 5 second status update;
    - stable white border for visual tearing detection.

  Dependency:
    - Arduino_GFX_Library by moononournation;
    - LVGL 8.x;
    - ESP32_8048S043 local BSP library.
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

#include "esp_heap_caps.h"

using namespace esp32_8048s043::pins;

#if LV_COLOR_DEPTH != 16
#error "15B_LVGL_ArduinoGFXFullFrameUI expects LV_COLOR_DEPTH == 16."
#endif

static const char *const SKETCH_ID = "15B-LVGL-GFXFULL1-240828A";

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LCD_PCLK_HZ = 16000000;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t STATUS_UPDATE_MS = 5000;
static constexpr uint32_t TOUCH_LOG_INTERVAL_MS = 180;
static constexpr uint32_t LOOP_DELAY_MS = 5;

static Arduino_ESP32RGBPanel *rgbPanel = nullptr;
static Arduino_RGB_Display *gfx = nullptr;
static ESP32_8048S043_Touch touch;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t indevDrv;
static lv_color_t *frameBuffer = nullptr;

static lv_obj_t *button = nullptr;
static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *statusLabel = nullptr;
static lv_obj_t *touchLabel = nullptr;

static bool displayOk = false;
static bool touchOk = false;
static bool lvglOk = false;
static bool uiOk = false;
static bool lastTouchDown = false;

static uint32_t lastTickMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t lastStatusMs = 0;
static uint32_t lastTouchLogMs = 0;
static uint32_t loopCount = 0;
static uint32_t flushAckCount = 0;
static uint32_t fullFramePushCount = 0;
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
  Serial.println(" ESP32-8048S043 Lab / 15B_LVGL_ArduinoGFXFullFrameUI");
  Serial.println(" LVGL 8 full-frame direct buffer -> Arduino_GFX + GT911 BSP");
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
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %lu bytes\n", "Free PSRAM", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("%-28s: %lu bytes\n", "Free heap", static_cast<unsigned long>(ESP.getFreeHeap()));
  printDivider();
  Serial.printf("%-28s: Arduino_GFX full-frame push\n", "Display mode");
  Serial.printf("%-28s: enabled\n", "LVGL direct_mode");
  Serial.printf("%-28s: one 800x480 RGB565 buffer\n", "LVGL framebuffer");
  Serial.printf("%-28s: PSRAM\n", "Framebuffer memory");
  Serial.printf("%-28s: every main loop\n", "Full-frame push");
  Serial.printf("%-28s: %u Hz\n", "PCLK", static_cast<unsigned>(LCD_PCLK_HZ));
  Serial.printf("%-28s: HSYNC 8/4/8, VSYNC 8/4/8\n", "Porches");
  Serial.printf("%-28s: ESP32_8048S043_Touch BSP\n", "GT911 touch");
  Serial.printf("%-28s: normal LVGL pressed style\n", "Button style");
  Serial.printf("%-28s: every %lu ms\n", "Status UI update", static_cast<unsigned long>(STATUS_UPDATE_MS));
  printDivider();
}

static bool initDisplay() {
  Serial.println("[DISPLAY INIT]");
  backlightOff();

  rgbPanel = new Arduino_ESP32RGBPanel(
    RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
    RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
    RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
    RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
    0 /* hsync polarity */, 8 /* hsync front porch */, 4 /* hsync pulse */, 8 /* hsync back porch */,
    0 /* vsync polarity */, 8 /* vsync front porch */, 4 /* vsync pulse */, 8 /* vsync back porch */,
    1 /* pclk active neg */, LCD_PCLK_HZ /* prefer speed */
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
  Serial.printf("[PASS] ESP32_8048S043_Touch::begin() addr=0x%02X fw=0x%04X res=%ux%u int=%d\n",
                touch.address(),
                touch.firmwareVersion(),
                touch.resolutionX(),
                touch.resolutionY(),
                touch.interruptLevel());
  return true;
}

static bool allocateFullFrameBuffer() {
  const size_t pixelCount = static_cast<size_t>(LCD_WIDTH) * static_cast<size_t>(LCD_HEIGHT);
  const size_t bytes = pixelCount * sizeof(lv_color_t);

  frameBuffer = static_cast<lv_color_t *>(
      heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  if (!frameBuffer) {
    Serial.printf("[FAIL] full LVGL framebuffer allocation failed: %u bytes\n", static_cast<unsigned>(bytes));
    return false;
  }

  memset(frameBuffer, 0, bytes);
  Serial.printf("[PASS] full LVGL framebuffer allocated in PSRAM: %u bytes at %p\n",
                static_cast<unsigned>(bytes), frameBuffer);
  return true;
}

static void lvglFlushAckOnly(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  (void)area;
  (void)colorP;
  flushAckCount++;

  // In this experiment LVGL renders directly into the full-screen buffer.
  // The complete buffer is transferred to Arduino_GFX from loop().
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
                    static_cast<unsigned long>(touchReports),
                    static_cast<unsigned long>(touch.acceptedPoints()),
                    static_cast<unsigned long>(touch.filteredUpdates()));
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
                  static_cast<unsigned long>(touchReleases),
                  lastTouchX,
                  lastTouchY,
                  lastZoneName);
  }
}

static bool initLvgl() {
  Serial.println("[LVGL INIT]");
  lv_init();

  if (!allocateFullFrameBuffer()) {
    return false;
  }

  const uint32_t pixelCount = static_cast<uint32_t>(LCD_WIDTH) * static_cast<uint32_t>(LCD_HEIGHT);
  lv_disp_draw_buf_init(&drawBuf, frameBuffer, nullptr, pixelCount);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_WIDTH;
  dispDrv.ver_res = LCD_HEIGHT;
  dispDrv.flush_cb = lvglFlushAckOnly;
  dispDrv.draw_buf = &drawBuf;
  dispDrv.direct_mode = 1;

  lv_disp_t *display = lv_disp_drv_register(&dispDrv);
  if (!display) {
    Serial.println("[FAIL] LVGL display driver registration failed");
    return false;
  }
  Serial.println("[PASS] LVGL full-frame direct display driver registered");

  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvglTouchRead;

  lv_indev_t *indev = lv_indev_drv_register(&indevDrv);
  if (!indev) {
    Serial.println("[FAIL] LVGL GT911 pointer driver registration failed");
    return false;
  }
  Serial.println("[PASS] LVGL GT911 BSP pointer driver registered");

  lvglOk = true;
  lastTickMs = millis();
  return true;
}

static void updateCounterLabel() {
  if (!counterLabel) {
    return;
  }

  char text[96];
  snprintf(text, sizeof(text), "Clicks: %lu    Pressed: %lu",
           static_cast<unsigned long>(clickedEvents),
           static_cast<unsigned long>(pressedEvents));
  lv_label_set_text(counterLabel, text);
}

static void updateTouchLabel() {
  if (!touchLabel) {
    return;
  }

  char text[160];
  snprintf(text, sizeof(text),
           "Touch mapped=(%u,%u) raw=(%u,%u) zone=%s",
           lastTouchX,
           lastTouchY,
           lastRawX,
           lastRawY,
           lastZoneName);
  lv_label_set_text(touchLabel, text);
}

static void buttonEvent(lv_event_t *event) {
  const lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_PRESSED) {
    pressedEvents++;
    updateCounterLabel();
    updateTouchLabel();
    Serial.printf("[BUTTON] pressed=%lu touch=(%u,%u) zone=%s framePush=%lu\n",
                  static_cast<unsigned long>(pressedEvents),
                  lastTouchX,
                  lastTouchY,
                  lastZoneName,
                  static_cast<unsigned long>(fullFramePushCount));
  }

  if (code == LV_EVENT_CLICKED) {
    clickedEvents++;
    updateCounterLabel();
    updateTouchLabel();
    Serial.printf("[BUTTON] clicked=%lu pressed=%lu touch=(%u,%u) zone=%s framePush=%lu\n",
                  static_cast<unsigned long>(clickedEvents),
                  static_cast<unsigned long>(pressedEvents),
                  lastTouchX,
                  lastTouchY,
                  lastZoneName,
                  static_cast<unsigned long>(fullFramePushCount));
  }
}

static void createUi() {
  Serial.println("[UI INIT]");

  lv_obj_t *screen = lv_scr_act();
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101621), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(screen);
  lv_label_set_text(title, "ESP32-8048S043 / 15B Full-frame Arduino_GFX");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  lv_obj_t *subtitle = lv_label_create(screen);
  lv_label_set_text(subtitle, "LVGL 8 direct buffer -> full 800x480 frame push every loop");
  lv_obj_set_style_text_color(subtitle, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 47);

  lv_obj_t *card = lv_obj_create(screen);
  lv_obj_set_size(card, 620, 300);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 22);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x172131), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x3A465C), 0);
  lv_obj_set_style_radius(card, 12, 0);

  button = lv_btn_create(card);
  lv_obj_set_size(button, 310, 112);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 28);
  lv_obj_add_event_cb(button, buttonEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t *buttonText = lv_label_create(button);
  lv_label_set_text(buttonText, "Tap me");
  lv_obj_center(buttonText);

  counterLabel = lv_label_create(card);
  lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(counterLabel, LV_ALIGN_TOP_MID, 0, 160);
  updateCounterLabel();

  touchLabel = lv_label_create(card);
  lv_obj_set_style_text_color(touchLabel, lv_color_hex(0xB8C7DC), 0);
  lv_obj_align(touchLabel, LV_ALIGN_TOP_MID, 0, 196);
  updateTouchLabel();

  statusLabel = lv_label_create(card);
  lv_label_set_text(statusLabel, "Status: waiting for physical validation");
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x8EE3B0), 0);
  lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 232);

  lv_obj_t *border = lv_obj_create(screen);
  lv_obj_set_pos(border, 4, 4);
  lv_obj_set_size(border, LCD_WIDTH - 8, LCD_HEIGHT - 8);
  lv_obj_clear_flag(border, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(border, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(border, 2, 0);
  lv_obj_set_style_border_color(border, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(border, 0, 0);

  uiOk = true;
  Serial.println("[PASS] LVGL comparison UI objects created");
}

static void tickLvgl() {
  const uint32_t now = millis();
  const uint32_t elapsed = now - lastTickMs;

  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    lastTickMs = now;
  }
}

static void updatePeriodicStatus() {
  const uint32_t now = millis();
  if (!statusLabel || now - lastStatusMs < STATUS_UPDATE_MS) {
    return;
  }

  lastStatusMs = now;

  char text[192];
  snprintf(text, sizeof(text),
           "Status: %lus | framePush=%lu | flushAck=%lu | readFail=%lu | pointFail=%lu",
           static_cast<unsigned long>(now / 1000),
           static_cast<unsigned long>(fullFramePushCount),
           static_cast<unsigned long>(flushAckCount),
           static_cast<unsigned long>(touch.readFailures()),
           static_cast<unsigned long>(touch.pointFailures()));
  lv_label_set_text(statusLabel, text);
}

static void pushFullFrame() {
  if (!displayOk || !gfx || !frameBuffer) {
    return;
  }

  gfx->draw16bitRGBBitmap(
      0,
      0,
      reinterpret_cast<uint16_t *>(frameBuffer),
      LCD_WIDTH,
      LCD_HEIGHT);

  fullFramePushCount++;
}

static void alive() {
  const uint32_t now = millis();
  if (now - lastAliveMs < ALIVE_INTERVAL_MS) {
    return;
  }

  lastAliveMs = now;

  Serial.printf("[ALIVE] fw=%s uptime=%lus display=%s touch=%s lvgl=%s ui=%s clicked=%lu pressed=%lu reports=%lu releases=%lu framePush=%lu flushAck=%lu indev=%lu loops=%lu readFail=%lu pointFail=%lu heap=%lu psram=%lu freePsram=%lu\n",
                SKETCH_ID,
                static_cast<unsigned long>(now / 1000),
                displayOk ? "OK" : "FAIL",
                touchOk ? "OK" : "FAIL",
                lvglOk ? "OK" : "FAIL",
                uiOk ? "OK" : "FAIL",
                static_cast<unsigned long>(clickedEvents),
                static_cast<unsigned long>(pressedEvents),
                static_cast<unsigned long>(touchReports),
                static_cast<unsigned long>(touchReleases),
                static_cast<unsigned long>(fullFramePushCount),
                static_cast<unsigned long>(flushAckCount),
                static_cast<unsigned long>(indevReads),
                static_cast<unsigned long>(loopCount),
                static_cast<unsigned long>(touch.readFailures()),
                static_cast<unsigned long>(touch.pointFailures()),
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getPsramSize()),
                static_cast<unsigned long>(ESP.getFreePsram()));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(900);

  printBanner();

  if (!initDisplay()) {
    Serial.println("[STOP] Arduino_GFX display init failed");
    while (true) {
      delay(1000);
    }
  }

  if (!initTouch()) {
    Serial.println("[STOP] GT911 BSP touch init failed");
    while (true) {
      delay(1000);
    }
  }

  if (!initLvgl()) {
    Serial.println("[STOP] LVGL init failed");
    while (true) {
      delay(1000);
    }
  }

  createUi();

  // Render the initial LVGL scene into the direct framebuffer.
  for (int i = 0; i < 8; ++i) {
    tickLvgl();
    lv_timer_handler();
    delay(5);
  }

  // First full frame is transferred while backlight is still off.
  pushFullFrame();
  backlightOn();

  Serial.println("[PASS] Backlight ON after first full-frame transfer");
  Serial.println("[READY] Compare against test 15: idle redraw, normal press, hard tap and border stability.");
}

void loop() {
  loopCount++;

  tickLvgl();
  lv_timer_handler();
  updatePeriodicStatus();

  // Deliberately transfer the complete 800x480 RGB565 LVGL framebuffer on
  // every loop, mirroring the redraw architecture that physically worked in
  // the external comparison firmware.
  pushFullFrame();

  alive();
  delay(LOOP_DELAY_MS);
}
