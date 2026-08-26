/*
  ESP32-8048S043 Lab / 11_LVGL_Dashboard

  Purpose:
    First dashboard-style LVGL 8 screen for ESP32-8048S043 after
    10_LVGL_BasicUI proved that the LVGL display path, BSP-backed GT911 touch,
    button events and backlight slider are functional on Sample A.

  Boundary:
    This is still a local HMI example. It does not validate Web setup,
    Widget Runtime, GitHub OTA, SD-backed assets or LVGL 9 compatibility.
*/

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

#include "esp_heap_caps.h"

using namespace esp32_8048s043::pins;

#if LV_COLOR_DEPTH != 16
#error "11_LVGL_Dashboard expects LV_COLOR_DEPTH == 16 for Arduino_GFX RGB565 flush."
#endif

static const char *const SKETCH_ID = "11DASH-MT1-240826C";

static constexpr int LCD_W = LCD_WIDTH;
static constexpr int LCD_H = LCD_HEIGHT;
static constexpr int LCD_PCLK_HZ = 16000000;
static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 40;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;

// Static/manual touch mode:
// - no LVGL pointer driver is registered;
// - GT911 is polled by this sketch;
// - touches are interpreted as coarse hitboxes and X-axis projection;
// - LVGL does not repaint widget pressed/drag states under the finger.
static constexpr bool DASHBOARD_STATIC_REFRESH = true;
static constexpr bool DASHBOARD_MANUAL_TOUCH = true;
static constexpr uint32_t MANUAL_TOUCH_POLL_MS = 25;
static constexpr uint32_t MANUAL_BUTTON_DEBOUNCE_MS = 280;
static constexpr uint32_t MANUAL_SLIDER_UPDATE_MS = 70;
static constexpr int MANUAL_SLIDER_DEADBAND = 4;

// Absolute screen hitboxes for the dashboard layout.
static constexpr int REFRESH_BUTTON_X = 44;
static constexpr int REFRESH_BUTTON_Y = 324;
static constexpr int REFRESH_BUTTON_W = 180;
static constexpr int REFRESH_BUTTON_H = 58;

static constexpr int BACKLIGHT_AXIS_X1 = 288;
static constexpr int BACKLIGHT_AXIS_X2 = 718;
static constexpr int BACKLIGHT_AXIS_Y1 = 300;
static constexpr int BACKLIGHT_AXIS_Y2 = 400;
static constexpr int BACKLIGHT_MIN = 16;
static constexpr int BACKLIGHT_MAX = 255;

static Arduino_ESP32RGBPanel *rgbPanel = nullptr;
static Arduino_RGB_Display *gfx = nullptr;
static ESP32_8048S043_Touch touch;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_color_t *lvBuf1 = nullptr;
static lv_color_t *lvBuf2 = nullptr;

static bool displayOk = false;
static bool touchOk = false;
static bool lvglOk = false;
static bool uiOk = false;

static uint32_t lastAliveMs = 0;
static uint32_t lastLvTickMs = 0;
static uint32_t lastManualTouchPollMs = 0;
static uint32_t lastManualButtonMs = 0;
static uint32_t lastManualSliderMs = 0;
static uint32_t lvglLoops = 0;
static uint32_t refreshClicks = 0;
static uint32_t manualTouchEvents = 0;
static uint32_t manualSliderEvents = 0;
static uint8_t backlightDuty = 220;
static bool buttonTouchHeld = false;
static bool sliderTouchHeld = false;
static float projectedDuty = static_cast<float>(backlightDuty);

static lv_obj_t *uptimeLabel = nullptr;
static lv_obj_t *heapLabel = nullptr;
static lv_obj_t *psramLabel = nullptr;
static lv_obj_t *touchLabel = nullptr;
static lv_obj_t *eventLabel = nullptr;
static lv_obj_t *backlightLabel = nullptr;
static lv_obj_t *heapBar = nullptr;
static lv_obj_t *psramBar = nullptr;
static lv_obj_t *backlightBar = nullptr;
static lv_obj_t *refreshButton = nullptr;
static lv_obj_t *refreshButtonLabel = nullptr;

static int clampInt(int value, int lo, int hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

static bool inBox(uint16_t x, uint16_t y, int boxX, int boxY, int boxW, int boxH) {
  return x >= boxX && x < boxX + boxW && y >= boxY && y < boxY + boxH;
}

static int projectBacklightDuty(uint16_t x) {
  const int clampedX = clampInt(static_cast<int>(x), BACKLIGHT_AXIS_X1, BACKLIGHT_AXIS_X2);
  const int span = BACKLIGHT_AXIS_X2 - BACKLIGHT_AXIS_X1;
  const int range = BACKLIGHT_MAX - BACKLIGHT_MIN;
  return BACKLIGHT_MIN + ((clampedX - BACKLIGHT_AXIS_X1) * range) / span;
}

static void setBacklightDuty(uint8_t duty) {
  backlightDuty = duty;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  static bool attached = false;
  if (!attached) {
    attached = ledcAttach(BACKLIGHT, 5000, 8);
    if (!attached) {
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
  setBacklightDuty(backlightDuty);

  rgbPanel = new Arduino_ESP32RGBPanel(
    RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
    RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
    RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
    RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
    1, 40, 48, 40,
    1, 13, 3, 29,
    1, LCD_PCLK_HZ
  );

  gfx = new Arduino_RGB_Display(LCD_W, LCD_H, rgbPanel, 0, true);
  if (!gfx->begin()) {
    Serial.println("[FAIL] gfx->begin()");
    return false;
  }
  gfx->fillScreen(0x0000);
  Serial.println("[PASS] gfx->begin()");
  return true;
}

static void lvglFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorP) {
  if (gfx) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(colorP), w, h);
  }
  lv_disp_flush_ready(disp);
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

  lastLvTickMs = millis();
  Serial.println("[PASS] LVGL display driver registered");
  Serial.println("[PASS] GT911 touch handled manually, LVGL pointer driver disabled");
  return true;
}

static lv_obj_t *makeCard(lv_obj_t *parent, const char *titleText, int x, int y, int w, int h) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, w, h);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1C2E3A), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x3E92CC), 0);
  lv_obj_set_style_border_width(card, 2, 0);

  lv_obj_t *title = lv_label_create(card);
  lv_label_set_text(title, titleText);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);
  return card;
}

static void updateBacklightLabelOnly() {
  if (backlightLabel) {
    lv_label_set_text_fmt(backlightLabel, "Backlight PWM: %u / 255", static_cast<unsigned int>(backlightDuty));
  }
  if (backlightBar) {
    lv_bar_set_value(backlightBar, backlightDuty, LV_ANIM_OFF);
  }
}

static void updateRefreshButtonOnly() {
  if (refreshButtonLabel) {
    lv_label_set_text_fmt(refreshButtonLabel, "Refresh %lu", static_cast<unsigned long>(refreshClicks));
  }
}

static void updateDashboard() {
  if (!uiOk) {
    return;
  }

  const uint32_t now = millis();
  const uint32_t heapSize = ESP.getHeapSize();
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t psramSize = ESP.getPsramSize();
  const uint32_t freePsram = ESP.getFreePsram();

  const int heapUsedPercent = heapSize ? static_cast<int>((100UL * (heapSize - freeHeap)) / heapSize) : 0;
  const int psramUsedPercent = psramSize ? static_cast<int>((100UL * (psramSize - freePsram)) / psramSize) : 0;

  lv_label_set_text_fmt(uptimeLabel, "Uptime snapshot: %lu s | FW %s", static_cast<unsigned long>(now / 1000), SKETCH_ID);
  lv_label_set_text_fmt(heapLabel, "Heap free: %lu KB / used %d%%", static_cast<unsigned long>(freeHeap / 1024UL), heapUsedPercent);
  lv_label_set_text_fmt(psramLabel, "PSRAM free: %lu KB / used %d%%", static_cast<unsigned long>(freePsram / 1024UL), psramUsedPercent);
  lv_label_set_text_fmt(touchLabel, "GT911 manual: addr=0x%02X fw=0x%04X accepted=%lu",
                        touch.address(), touch.firmwareVersion(), static_cast<unsigned long>(touch.acceptedPoints()));
  lv_label_set_text_fmt(eventLabel, "Refresh: %lu | manual touch: %lu | loops: %lu",
                        static_cast<unsigned long>(refreshClicks),
                        static_cast<unsigned long>(manualTouchEvents),
                        static_cast<unsigned long>(lvglLoops));
  lv_bar_set_value(heapBar, heapUsedPercent, LV_ANIM_OFF);
  lv_bar_set_value(psramBar, psramUsedPercent, LV_ANIM_OFF);
  updateBacklightLabelOnly();
  updateRefreshButtonOnly();
}

static void manualRefreshAction() {
  ++refreshClicks;
  Serial.printf("[MANUAL] fw=%s Refresh hitbox: %lu\n", SKETCH_ID, static_cast<unsigned long>(refreshClicks));
  updateRefreshButtonOnly();
  updateDashboard();
}

static void manualBacklightAction(uint16_t x) {
  const int target = projectBacklightDuty(x);
  projectedDuty = projectedDuty * 0.70f + static_cast<float>(target) * 0.30f;
  const int filteredDuty = clampInt(static_cast<int>(projectedDuty + 0.5f), BACKLIGHT_MIN, BACKLIGHT_MAX);

  if (abs(filteredDuty - static_cast<int>(backlightDuty)) < MANUAL_SLIDER_DEADBAND) {
    return;
  }

  setBacklightDuty(static_cast<uint8_t>(filteredDuty));
  ++manualSliderEvents;
  Serial.printf("[MANUAL] fw=%s Backlight axis: x=%u duty=%d\n", SKETCH_ID, static_cast<unsigned int>(x), filteredDuty);
  updateBacklightLabelOnly();
}

static void pollManualTouch() {
  const uint32_t now = millis();
  if (!touchOk || now - lastManualTouchPollMs < MANUAL_TOUCH_POLL_MS) {
    return;
  }
  lastManualTouchPollMs = now;

  ESP32_8048S043_TouchPoint point;
  if (!touch.read(point) || !point.touched) {
    buttonTouchHeld = false;
    sliderTouchHeld = false;
    projectedDuty = static_cast<float>(backlightDuty);
    return;
  }

  ++manualTouchEvents;

  if (inBox(point.x, point.y, BACKLIGHT_AXIS_X1, BACKLIGHT_AXIS_Y1,
            BACKLIGHT_AXIS_X2 - BACKLIGHT_AXIS_X1, BACKLIGHT_AXIS_Y2 - BACKLIGHT_AXIS_Y1)) {
    buttonTouchHeld = false;
    sliderTouchHeld = true;
    if (now - lastManualSliderMs >= MANUAL_SLIDER_UPDATE_MS) {
      lastManualSliderMs = now;
      manualBacklightAction(point.x);
    }
    return;
  }

  sliderTouchHeld = false;

  if (inBox(point.x, point.y, REFRESH_BUTTON_X, REFRESH_BUTTON_Y, REFRESH_BUTTON_W, REFRESH_BUTTON_H)) {
    if (!buttonTouchHeld && now - lastManualButtonMs >= MANUAL_BUTTON_DEBOUNCE_MS) {
      buttonTouchHeld = true;
      lastManualButtonMs = now;
      manualRefreshAction();
    }
    return;
  }

  buttonTouchHeld = false;
}

static void createDashboardUi() {
  lv_obj_t *screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);

  lv_obj_t *title = lv_label_create(screen);
  lv_label_set_text(title, "ESP32-8048S043 LVGL Dashboard");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  uptimeLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(uptimeLabel, lv_color_hex(0xC8D8E4), 0);
  lv_obj_align(uptimeLabel, LV_ALIGN_TOP_MID, 0, 48);

  lv_obj_t *memCard = makeCard(screen, "Memory", 28, 88, 350, 150);
  heapLabel = lv_label_create(memCard);
  lv_obj_set_style_text_color(heapLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(heapLabel, LV_ALIGN_TOP_LEFT, 4, 42);
  heapBar = lv_bar_create(memCard);
  lv_obj_set_width(heapBar, 300);
  lv_obj_align(heapBar, LV_ALIGN_TOP_LEFT, 4, 68);
  lv_bar_set_range(heapBar, 0, 100);
  psramLabel = lv_label_create(memCard);
  lv_obj_set_style_text_color(psramLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(psramLabel, LV_ALIGN_TOP_LEFT, 4, 98);
  psramBar = lv_bar_create(memCard);
  lv_obj_set_width(psramBar, 300);
  lv_obj_align(psramBar, LV_ALIGN_TOP_LEFT, 4, 124);
  lv_bar_set_range(psramBar, 0, 100);

  lv_obj_t *touchCard = makeCard(screen, "Manual Touch", 422, 88, 350, 150);
  touchLabel = lv_label_create(touchCard);
  lv_obj_set_style_text_color(touchLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(touchLabel, 310);
  lv_obj_align(touchLabel, LV_ALIGN_TOP_LEFT, 4, 42);
  eventLabel = lv_label_create(touchCard);
  lv_obj_set_style_text_color(eventLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(eventLabel, 310);
  lv_obj_align(eventLabel, LV_ALIGN_TOP_LEFT, 4, 88);

  lv_obj_t *controlCard = makeCard(screen, "Controls", 28, 270, 744, 160);
  refreshButton = lv_btn_create(controlCard);
  lv_obj_set_size(refreshButton, 180, 58);
  lv_obj_align(refreshButton, LV_ALIGN_TOP_LEFT, 16, 54);
  lv_obj_clear_flag(refreshButton, LV_OBJ_FLAG_CLICKABLE);

  refreshButtonLabel = lv_label_create(refreshButton);
  lv_label_set_text(refreshButtonLabel, "Refresh");
  lv_obj_center(refreshButtonLabel);

  lv_obj_t *axisLabel = lv_label_create(controlCard);
  lv_label_set_text(axisLabel, "Backlight axis touch band");
  lv_obj_set_style_text_color(axisLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(axisLabel, LV_ALIGN_TOP_LEFT, 260, 34);

  backlightBar = lv_bar_create(controlCard);
  lv_obj_set_width(backlightBar, 430);
  lv_obj_align(backlightBar, LV_ALIGN_TOP_LEFT, 260, 66);
  lv_bar_set_range(backlightBar, BACKLIGHT_MIN, BACKLIGHT_MAX);
  lv_bar_set_value(backlightBar, backlightDuty, LV_ANIM_OFF);

  backlightLabel = lv_label_create(controlCard);
  lv_obj_set_style_text_color(backlightLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(backlightLabel, LV_ALIGN_TOP_LEFT, 260, 98);

  uiOk = true;
  updateDashboard();
  Serial.println("[PASS] LVGL dashboard UI objects created");
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32-8048S043 Lab / 11_LVGL_Dashboard");
  Serial.println(" LVGL 8 dashboard validation");
  Serial.printf(" Firmware ID: %s\n", SKETCH_ID);
  Serial.println("============================================================");
  Serial.println("Mode   : RGB display + manual GT911 hitboxes + LVGL dashboard");
  Serial.println("Refresh: static screen, manual dashboard refresh only");
  Serial.println("Touch  : LVGL pointer disabled; button/axis handled by sketch");
  Serial.println("Serial : 115200 baud");
  Serial.println("------------------------------------------------------------");

  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_BOARD", ARDUINO_BOARD);
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_VARIANT", ARDUINO_VARIANT);
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %s rev %u\n", "Chip", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %s\n", "Static refresh", DASHBOARD_STATIC_REFRESH ? "enabled" : "disabled");
  Serial.printf("%-28s: %s\n", "Manual touch", DASHBOARD_MANUAL_TOUCH ? "enabled" : "disabled");
  Serial.println("------------------------------------------------------------");

  displayOk = initDisplay();
  if (!displayOk) {
    Serial.println("[FATAL] Display init failed");
    return;
  }

  Serial.println("[TOUCH BSP INIT]");
  touchOk = touch.begin(Wire);
  if (touchOk) {
    Serial.printf("[PASS] ESP32_8048S043_Touch::begin() addr=0x%02X fw=0x%04X res=%ux%u int=%d\n",
                  touch.address(), touch.firmwareVersion(), touch.resolutionX(), touch.resolutionY(), touch.interruptLevel());
  } else {
    Serial.println("[WARN] ESP32_8048S043_Touch::begin() failed; dashboard will run display-only");
  }

  lvglOk = initLvgl();
  if (!lvglOk) {
    Serial.println("[FATAL] LVGL init failed");
    return;
  }

  createDashboardUi();

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" LVGL DASHBOARD READY");
  Serial.printf(" Firmware ID: %s\n", SKETCH_ID);
  Serial.println(" Static refresh mode: no 1 Hz dashboard redraw.");
  Serial.println(" Manual touch mode: no LVGL pressed/drag redraw under finger.");
  Serial.println("============================================================");
}

void loop() {
  const uint32_t now = millis();

  pollManualTouch();

  if (lvglOk) {
    const uint32_t elapsed = now - lastLvTickMs;
    if (elapsed >= 5) {
      lv_tick_inc(elapsed);
      lastLvTickMs = now;
    }
    lv_timer_handler();
    ++lvglLoops;
  }

  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] fw=%s uptime=%lus display=%s touch=%s lvgl=%s ui=%s refresh=%lu manualTouch=%lu slider=%lu accepted=%lu filtered=%lu loops=%lu freeHeap=%lu psram=%lu freePsram=%lu\n",
                  SKETCH_ID,
                  static_cast<unsigned long>(now / 1000),
                  displayOk ? "OK" : "FAIL",
                  touchOk ? "OK" : "OPEN",
                  lvglOk ? "OK" : "FAIL",
                  uiOk ? "OK" : "FAIL",
                  static_cast<unsigned long>(refreshClicks),
                  static_cast<unsigned long>(manualTouchEvents),
                  static_cast<unsigned long>(manualSliderEvents),
                  static_cast<unsigned long>(touch.acceptedPoints()),
                  static_cast<unsigned long>(touch.filteredUpdates()),
                  static_cast<unsigned long>(lvglLoops),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(5);
}
