/*
  ESP32-8048S043 Lab / 19_LVGL9_ArduinoGFX_BounceBufferUI

  Purpose:
    Current-environment companion to Test 18. Reproduce the promising display
    stability pattern seen in the physically successful Robot-Core-Display run,
    but with an independently authored minimal UI and our own GT911 BSP.

  Architecture:

      LVGL 9
        -> two strict INTERNAL-SRAM partial draw buffers
        -> Arduino_GFX partial-area flush
        -> Arduino_ESP32RGBPanel RGB bounce buffer
        -> 800x480 RGB panel

      ESP32_8048S043_Touch BSP
        -> LVGL 9 pointer input

  This source is independently authored and does not copy GPL application code.

  Controlled values:
    - current Arduino-ESP32 / ESP-IDF stack;
    - current Arduino_GFX;
    - LVGL 9.x;
    - our validated ESP32_8048S043 GT911 BSP;
    - 800x480;
    - PCLK 14 MHz;
    - HSYNC 8/4/8;
    - VSYNC 8/4/8;
    - pclk_active_neg = 1;
    - two 20-line RGB565 LVGL buffers in internal SRAM;
    - RGB bounce buffer = 20 display lines;
    - event-driven UI with no periodic invalidation.
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>
#include "esp_heap_caps.h"

using namespace esp32_8048s043::pins;

#if LV_VERSION_MAJOR != 9
#error "Test 19 requires LVGL 9.x. Use Test 18 for the LVGL 8 controlled comparison."
#endif

#if LV_COLOR_DEPTH != 16
#error "Test 19 requires LV_COLOR_DEPTH == 16."
#endif

static const char *const SKETCH_ID = "19-LVGL9-GFX-BOUNCE1-240830A";

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LCD_PCLK_HZ = 14000000;
static constexpr uint16_t LVGL_DRAW_LINES = 20;
static constexpr uint32_t LVGL_BUFFER_PIXELS = LCD_WIDTH * LVGL_DRAW_LINES;
static constexpr size_t LVGL_BUFFER_BYTES = LVGL_BUFFER_PIXELS * sizeof(lv_color_t);
static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;
static constexpr uint32_t LOOP_DELAY_MS = 5;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;

static Arduino_ESP32RGBPanel *rgbPanel = nullptr;
static Arduino_RGB_Display *gfx = nullptr;
static ESP32_8048S043_Touch touch;

static lv_display_t *display = nullptr;
static lv_indev_t *indev = nullptr;
static lv_color_t *lvBufA = nullptr;
static lv_color_t *lvBufB = nullptr;

static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *sliderValueLabel = nullptr;
static lv_obj_t *stateCard = nullptr;
static lv_obj_t *stateLabel = nullptr;

static bool displayOk = false;
static bool touchOk = false;
static bool lvglOk = false;
static bool lastTouchDown = false;
static bool cardState = false;

static uint16_t lastTouchX = 0;
static uint16_t lastTouchY = 0;
static uint32_t clickCount = 0;
static uint32_t flushCount = 0;
static uint32_t indevReadCount = 0;
static uint32_t touchPressCount = 0;
static uint32_t touchReleaseCount = 0;
static uint32_t loopCount = 0;
static uint32_t lastTickMs = 0;
static uint32_t lastAliveMs = 0;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printBanner() {
  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / Test 19");
  Serial.println(" LVGL9 + double internal SRAM buffers + RGB bounce buffer");
  Serial.println("================================================================");
  Serial.printf("%-28s: %s\n", "Firmware ID", SKETCH_ID);
#ifdef ARDUINO_BOARD
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_BOARD", ARDUINO_BOARD);
#endif
#ifdef ARDUINO_VARIANT
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_VARIANT", ARDUINO_VARIANT);
#endif
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %d.%d.%d\n", "LVGL version",
                LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);
  Serial.printf("%-28s: %lu bytes\n", "Flash",
                static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM",
                static_cast<unsigned long>(ESP.getPsramSize()));
  printDivider();
  Serial.printf("%-28s: Arduino_GFX partial-area\n", "Display path");
  Serial.printf("%-28s: %u Hz\n", "PCLK", static_cast<unsigned>(LCD_PCLK_HZ));
  Serial.printf("%-28s: 8/4/8\n", "HSYNC porches");
  Serial.printf("%-28s: 8/4/8\n", "VSYNC porches");
  Serial.printf("%-28s: %u lines / %lu px\n", "RGB bounce buffer",
                20U, static_cast<unsigned long>(RGB_BOUNCE_PIXELS));
  Serial.printf("%-28s: 2 x %u lines\n", "LVGL draw buffers", LVGL_DRAW_LINES);
  Serial.printf("%-28s: %lu bytes each\n", "LVGL buffer bytes",
                static_cast<unsigned long>(LVGL_BUFFER_BYTES));
  Serial.printf("%-28s: INTERNAL SRAM required\n", "LVGL buffer policy");
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
    1 /* pclk active neg */, LCD_PCLK_HZ,
    false /* useBigEndian */,
    0 /* de idle high */,
    0 /* pclk idle high */,
    RGB_BOUNCE_PIXELS
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
  Serial.println("[PASS] gfx->begin() with RGB bounce buffer requested");
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

static lv_color_t *allocateStrictInternalBuffer(const char *name) {
  lv_color_t *ptr = static_cast<lv_color_t *>(
    heap_caps_malloc(LVGL_BUFFER_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
  );

  if (!ptr) {
    Serial.printf("[FAIL] %s internal SRAM allocation failed: %u bytes\n",
                  name, static_cast<unsigned>(LVGL_BUFFER_BYTES));
    return nullptr;
  }

  if (!esp_ptr_internal(ptr)) {
    Serial.printf("[FAIL] %s allocation is not internal SRAM: %p\n", name, ptr);
    heap_caps_free(ptr);
    return nullptr;
  }

  Serial.printf("[PASS] %s INTERNAL SRAM: %u bytes at %p\n",
                name, static_cast<unsigned>(LVGL_BUFFER_BYTES), ptr);
  return ptr;
}

static void lvglFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *pxMap) {
  flushCount++;

  if (gfx) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;

    gfx->draw16bitRGBBitmap(
      area->x1,
      area->y1,
      reinterpret_cast<uint16_t *>(pxMap),
      w,
      h
    );
  }

  lv_display_flush_ready(disp);
}

static void lvglTouchRead(lv_indev_t *input, lv_indev_data_t *data) {
  (void)input;
  indevReadCount++;

  ESP32_8048S043_TouchPoint point;
  const bool readOk = touchOk && touch.read(point);

  if (readOk && point.touched) {
    lastTouchX = point.x;
    lastTouchY = point.y;
    data->state = LV_INDEV_STATE_PRESSED;
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

  data->state = LV_INDEV_STATE_RELEASED;
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

static bool initLvgl() {
  Serial.println("[LVGL INIT]");
  lv_init();

  lvBufA = allocateStrictInternalBuffer("LVGL buffer A");
  lvBufB = allocateStrictInternalBuffer("LVGL buffer B");

  if (!lvBufA || !lvBufB) {
    Serial.println("[STOP] Strict double-buffer internal-SRAM condition not met");
    return false;
  }

  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  if (!display) {
    Serial.println("[FAIL] lv_display_create()");
    return false;
  }

  lv_display_set_flush_cb(display, lvglFlush);
  lv_display_set_buffers(
    display,
    lvBufA,
    lvBufB,
    LVGL_BUFFER_BYTES,
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );
  Serial.println("[PASS] LVGL9 partial display registered with 2 SRAM buffers");

  if (touchOk) {
    indev = lv_indev_create();
    if (!indev) {
      Serial.println("[FAIL] lv_indev_create()");
      return false;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvglTouchRead);
    Serial.println("[PASS] LVGL9 pointer registered through BSP GT911");
  }

  lvglOk = true;
  lastTickMs = millis();
  return true;
}

static void styleCard(lv_obj_t *obj) {
  lv_obj_set_style_radius(obj, 14, LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_all(obj, 16, LV_PART_MAIN);
}

static void onActionButton(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  clickCount++;
  cardState = !cardState;

  if (counterLabel) {
    lv_label_set_text_fmt(counterLabel, "Actions: %lu",
                          static_cast<unsigned long>(clickCount));
  }

  if (stateLabel) {
    lv_label_set_text(stateLabel, cardState ? "State B" : "State A");
  }

  if (stateCard) {
    lv_obj_set_style_bg_opa(stateCard,
                            cardState ? LV_OPA_70 : LV_OPA_30,
                            LV_PART_MAIN);
  }

  Serial.printf("[UI ACTION] click=%lu state=%s flush=%lu\n",
                static_cast<unsigned long>(clickCount),
                cardState ? "B" : "A",
                static_cast<unsigned long>(flushCount));
}

static void onSlider(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(event));
  const int32_t value = lv_slider_get_value(slider);

  if (sliderValueLabel) {
    lv_label_set_text_fmt(sliderValueLabel, "Level: %ld%%", static_cast<long>(value));
  }
}

static void createUi() {
  Serial.println("[UI INIT] independent LVGL9 embedded-HMI sample");

  lv_obj_t *screen = lv_screen_active();
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(screen, 18, LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(screen);
  lv_label_set_text(title, "ESP32-8048S043 | Stable Partial UI");
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *subtitle = lv_label_create(screen);
  lv_label_set_text(subtitle,
                    "LVGL9 + 2x internal SRAM + RGB bounce buffer");
  lv_obj_align_to(subtitle, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

  stateCard = lv_obj_create(screen);
  lv_obj_set_size(stateCard, 360, 185);
  lv_obj_align(stateCard, LV_ALIGN_LEFT_MID, 0, 24);
  styleCard(stateCard);
  lv_obj_remove_flag(stateCard, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(stateCard, LV_OPA_30, LV_PART_MAIN);

  lv_obj_t *cardTitle = lv_label_create(stateCard);
  lv_label_set_text(cardTitle, "Event-driven state");
  lv_obj_align(cardTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  stateLabel = lv_label_create(stateCard);
  lv_label_set_text(stateLabel, "State A");
  lv_obj_align(stateLabel, LV_ALIGN_LEFT_MID, 0, -4);

  counterLabel = lv_label_create(stateCard);
  lv_label_set_text(counterLabel, "Actions: 0");
  lv_obj_align(counterLabel, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  lv_obj_t *controlCard = lv_obj_create(screen);
  lv_obj_set_size(controlCard, 360, 185);
  lv_obj_align(controlCard, LV_ALIGN_RIGHT_MID, 0, 24);
  styleCard(controlCard);
  lv_obj_remove_flag(controlCard, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *button = lv_button_create(controlCard);
  lv_obj_set_size(button, 150, 54);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 0, 18);
  lv_obj_add_event_cb(button, onActionButton, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *buttonLabel = lv_label_create(button);
  lv_label_set_text(buttonLabel, "Toggle state");
  lv_obj_center(buttonLabel);

  sliderValueLabel = lv_label_create(controlCard);
  lv_label_set_text(sliderValueLabel, "Level: 50%");
  lv_obj_align(sliderValueLabel, LV_ALIGN_LEFT_MID, 0, 22);

  lv_obj_t *slider = lv_slider_create(controlCard);
  lv_obj_set_width(slider, 315);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 50, LV_ANIM_OFF);
  lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -5);
  lv_obj_add_event_cb(slider, onSlider, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t *footer = lv_label_create(screen);
  lv_label_set_text(footer,
                    "Rule: redraw changed objects; isolate the RGB scan-out path.");
  lv_obj_align(footer, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  Serial.println("[PASS] UI created");
}

static void renderInitialFrameAndEnableBacklight() {
  for (int i = 0; i < 8; ++i) {
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
  Serial.println("[READY] Judge redraw speed separately from jitter/flicker.");
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

  createUi();
  renderInitialFrameAndEnableBacklight();
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
    Serial.printf(
      "[ALIVE] ms=%lu loop=%lu flush=%lu indev=%lu press=%lu release=%lu clicks=%lu freeHeap=%lu freePsram=%lu\n",
      static_cast<unsigned long>(now),
      static_cast<unsigned long>(loopCount),
      static_cast<unsigned long>(flushCount),
      static_cast<unsigned long>(indevReadCount),
      static_cast<unsigned long>(touchPressCount),
      static_cast<unsigned long>(touchReleaseCount),
      static_cast<unsigned long>(clickCount),
      static_cast<unsigned long>(ESP.getFreeHeap()),
      static_cast<unsigned long>(ESP.getFreePsram())
    );
  }

  delay(LOOP_DELAY_MS);
}
