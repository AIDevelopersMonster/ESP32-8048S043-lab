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

static const char *const SKETCH_ID = "11DASH-SRC1-240826A";

static constexpr int LCD_W = LCD_WIDTH;
static constexpr int LCD_H = LCD_HEIGHT;
static constexpr int LCD_PCLK_HZ = 16000000;
static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 40;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t DASHBOARD_UPDATE_MS = 1000;

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

static uint32_t lastAliveMs = 0;
static uint32_t lastDashboardMs = 0;
static uint32_t lastLvTickMs = 0;
static uint32_t lvglLoops = 0;
static uint32_t refreshClicks = 0;
static uint8_t backlightDuty = 220;

static lv_obj_t *uptimeLabel = nullptr;
static lv_obj_t *heapLabel = nullptr;
static lv_obj_t *psramLabel = nullptr;
static lv_obj_t *touchLabel = nullptr;
static lv_obj_t *eventLabel = nullptr;
static lv_obj_t *backlightLabel = nullptr;
static lv_obj_t *heapBar = nullptr;
static lv_obj_t *psramBar = nullptr;

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

  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = lvglTouchRead;
  lv_indev_drv_register(&indevDrv);

  lastLvTickMs = millis();
  Serial.println("[PASS] LVGL display + input drivers registered");
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

  lv_label_set_text_fmt(uptimeLabel, "Uptime: %lu s | FW %s", static_cast<unsigned long>(now / 1000), SKETCH_ID);
  lv_label_set_text_fmt(heapLabel, "Heap free: %lu KB / used %d%%", static_cast<unsigned long>(freeHeap / 1024UL), heapUsedPercent);
  lv_label_set_text_fmt(psramLabel, "PSRAM free: %lu KB / used %d%%", static_cast<unsigned long>(freePsram / 1024UL), psramUsedPercent);
  lv_label_set_text_fmt(touchLabel, "GT911: addr=0x%02X fw=0x%04X res=%ux%u accepted=%lu",
                        touch.address(), touch.firmwareVersion(), touch.resolutionX(), touch.resolutionY(),
                        static_cast<unsigned long>(touch.acceptedPoints()));
  lv_label_set_text_fmt(eventLabel, "Refresh clicks: %lu | LVGL loops: %lu",
                        static_cast<unsigned long>(refreshClicks), static_cast<unsigned long>(lvglLoops));
  lv_label_set_text_fmt(backlightLabel, "Backlight PWM: %u / 255", static_cast<unsigned int>(backlightDuty));

  lv_bar_set_value(heapBar, heapUsedPercent, LV_ANIM_OFF);
  lv_bar_set_value(psramBar, psramUsedPercent, LV_ANIM_OFF);
}

static void refreshEvent(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    ++refreshClicks;
    Serial.printf("[LVGL] fw=%s Refresh clicked: %lu\n", SKETCH_ID, static_cast<unsigned long>(refreshClicks));
    updateDashboard();
  }
}

static void backlightEvent(lv_event_t *event) {
  lv_obj_t *slider = lv_event_get_target(event);
  const int value = static_cast<int>(lv_slider_get_value(slider));
  setBacklightDuty(static_cast<uint8_t>(value));
  if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
    Serial.printf("[LVGL] fw=%s Backlight slider: %d\n", SKETCH_ID, value);
  }
  updateDashboard();
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

  lv_obj_t *touchCard = makeCard(screen, "Touch BSP", 422, 88, 350, 150);
  touchLabel = lv_label_create(touchCard);
  lv_obj_set_style_text_color(touchLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(touchLabel, 310);
  lv_obj_align(touchLabel, LV_ALIGN_TOP_LEFT, 4, 42);
  eventLabel = lv_label_create(touchCard);
  lv_obj_set_style_text_color(eventLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_width(eventLabel, 310);
  lv_obj_align(eventLabel, LV_ALIGN_TOP_LEFT, 4, 88);

  lv_obj_t *controlCard = makeCard(screen, "Controls", 28, 270, 744, 160);
  lv_obj_t *button = lv_btn_create(controlCard);
  lv_obj_set_size(button, 180, 58);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 16, 54);
  lv_obj_add_event_cb(button, refreshEvent, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *buttonLabel = lv_label_create(button);
  lv_label_set_text(buttonLabel, "Refresh");
  lv_obj_center(buttonLabel);

  lv_obj_t *slider = lv_slider_create(controlCard);
  lv_obj_set_width(slider, 430);
  lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 260, 62);
  lv_slider_set_range(slider, 16, 255);
  lv_slider_set_value(slider, backlightDuty, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, backlightEvent, LV_EVENT_VALUE_CHANGED, nullptr);

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
  Serial.println("Mode   : RGB display + ESP32_8048S043_Touch BSP + LVGL dashboard");
  Serial.println("Serial : 115200 baud");
  Serial.println("------------------------------------------------------------");

  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_BOARD", ARDUINO_BOARD);
  Serial.printf("%-28s: \"%s\"\n", "ARDUINO_VARIANT", ARDUINO_VARIANT);
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %s rev %u\n", "Chip", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
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
  Serial.println("============================================================");
}

void loop() {
  const uint32_t now = millis();

  if (lvglOk) {
    const uint32_t elapsed = now - lastLvTickMs;
    if (elapsed >= 5) {
      lv_tick_inc(elapsed);
      lastLvTickMs = now;
    }
    lv_timer_handler();
    ++lvglLoops;
  }

  if (uiOk && now - lastDashboardMs >= DASHBOARD_UPDATE_MS) {
    lastDashboardMs = now;
    updateDashboard();
  }

  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] fw=%s uptime=%lus display=%s touch=%s lvgl=%s ui=%s refresh=%lu accepted=%lu filtered=%lu loops=%lu freeHeap=%lu psram=%lu freePsram=%lu\n",
                  SKETCH_ID,
                  static_cast<unsigned long>(now / 1000),
                  displayOk ? "OK" : "FAIL",
                  touchOk ? "OK" : "OPEN",
                  lvglOk ? "OK" : "FAIL",
                  uiOk ? "OK" : "FAIL",
                  static_cast<unsigned long>(refreshClicks),
                  static_cast<unsigned long>(touch.acceptedPoints()),
                  static_cast<unsigned long>(touch.filteredUpdates()),
                  static_cast<unsigned long>(lvglLoops),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(5);
}
