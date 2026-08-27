/*
  ESP32-8048S043 Lab / 13_LVGL_EspLcdStatic

  Purpose:
    First LVGL 8.3.x test over the native ESP-IDF esp_lcd RGB panel path.

  Why this exists:
    12_DisplayEspLcdRgbPanel_Probe proved that esp_lcd can initialize the panel
    and render correct static colors on Sample A, but naive raw dynamic
    draw_bitmap movement is not acceptable.

    This example removes raw animation and tests whether LVGL over esp_lcd can
    maintain a stable static screen with only rare, small label invalidations.

  What this example checks:
    - esp_lcd_new_rgb_panel() panel initialization;
    - ESP-IDF RGB565 data bit order;
    - PSRAM RGB panel framebuffer path;
    - LVGL 8 draw buffers in PSRAM;
    - LVGL flush callback to esp_lcd_panel_draw_bitmap();
    - stable static LVGL UI without touch;
    - rare label updates without full-screen animation.

  What this example intentionally does NOT use:
    - Arduino_GFX;
    - GT911 touch;
    - moving block animation;
    - slider/button interaction;
    - Wi-Fi/SD/BLE.

  Font note:
    This sketch intentionally uses LV_FONT_DEFAULT only. Some Arduino LVGL
    installations expose only lv_font_montserrat_14 by default.
*/

#include <Arduino.h>
#include <lvgl.h>
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
#error "13_LVGL_EspLcdStatic expects LV_COLOR_DEPTH == 16."
#endif

#define SKETCH_ID "13LVGL-ELCDS2-240827B"

#ifndef ESP_LCD_STATIC_PCLK_HZ
#define ESP_LCD_STATIC_PCLK_HZ 12500000
#endif

#ifndef ESP_LCD_STATIC_DOUBLE_FB
#define ESP_LCD_STATIC_DOUBLE_FB 1
#endif

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 100;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t LABEL_UPDATE_MS = 5000;

static esp_lcd_panel_handle_t panel = nullptr;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_color_t *lvBuf1 = nullptr;
static lv_color_t *lvBuf2 = nullptr;

static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *heapLabel = nullptr;
static lv_obj_t *flushLabel = nullptr;
static lv_obj_t *psramBar = nullptr;
static lv_obj_t *heapBar = nullptr;

static bool panelOk = false;
static bool lvglOk = false;
static bool uiOk = false;

static uint32_t lastTickMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t lastLabelMs = 0;
static uint32_t labelUpdates = 0;
static uint32_t flushCount = 0;
static uint32_t flushPixels = 0;
static uint32_t loopCount = 0;

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
  Serial.println(" ESP32-8048S043 Lab / 13_LVGL_EspLcdStatic");
  Serial.println(" LVGL 8 static UI over native esp_lcd RGB panel");
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
  Serial.printf("%-28s: LVGL static UI over esp_lcd RGB panel\n", "Mode");
  Serial.printf("%-28s: not used\n", "Arduino_GFX");
  Serial.printf("%-28s: not used\n", "GT911 touch");
  Serial.printf("%-28s: not used\n", "Moving animation");
  Serial.printf("%-28s: %dx%d\n", "Resolution", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("%-28s: %u Hz\n", "PCLK", (unsigned)ESP_LCD_STATIC_PCLK_HZ);
  Serial.printf("%-28s: HSYNC 8/4/8, VSYNC 8/4/8\n", "Porches");
  Serial.printf("%-28s: true\n", "Framebuffer in PSRAM");
  Serial.printf("%-28s: %s\n", "Double framebuffer", ESP_LCD_STATIC_DOUBLE_FB ? "true" : "false");
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
  cfg.timings.pclk_hz = ESP_LCD_STATIC_PCLK_HZ;
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
  cfg.num_fbs = ESP_LCD_STATIC_DOUBLE_FB ? 2 : 1;
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
  cfg.flags.double_fb = ESP_LCD_STATIC_DOUBLE_FB ? true : false;
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
  (void)disp;
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

  if (!lv_disp_drv_register(&dispDrv)) {
    Serial.println("[FAIL] LVGL display driver register failed");
    return false;
  }

  Serial.println("[PASS] LVGL display driver registered");
  lvglOk = true;
  return true;
}

static lv_obj_t *makeCard(lv_obj_t *parent, int x, int y, int w, int h, const char *title, const char *body, lv_color_t accent) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x202634), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, accent, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_pad_all(card, 10, 0);

  lv_obj_t *titleObj = lv_label_create(card);
  lv_label_set_text(titleObj, title);
  lv_obj_set_style_text_color(titleObj, accent, 0);
  lv_obj_align(titleObj, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *bodyObj = lv_label_create(card);
  lv_label_set_text(bodyObj, body);
  lv_label_set_long_mode(bodyObj, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(bodyObj, w - 24);
  lv_obj_set_style_text_color(bodyObj, lv_color_hex(0xE8EDF7), 0);
  lv_obj_align(bodyObj, LV_ALIGN_TOP_LEFT, 0, 28);

  return card;
}

static void createUi() {
  Serial.println("[UI INIT]");

  lv_obj_t *scr = lv_scr_act();
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101621), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "ESP32-8048S043 / LVGL over esp_lcd");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

  lv_obj_t *subtitle = lv_label_create(scr);
  lv_label_set_text(subtitle, "Static UI only: no touch, no animation, rare label updates");
  lv_obj_set_style_text_color(subtitle, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 42);

  makeCard(scr, 30, 80, 230, 110, "DISPLAY", "esp_lcd RGB panel\n12.5 MHz / 8-4-8\nPSRAM double framebuffer", lv_color_hex(0x00C2FF));
  makeCard(scr, 285, 80, 230, 110, "LVGL", "LVGL 8 static screen\n2x 100-line PSRAM buffers\nDefault font only", lv_color_hex(0x7CFF6B));
  makeCard(scr, 540, 80, 230, 110, "BOUNDARY", "No moving block\nNo GT911\nNo user controls", lv_color_hex(0xFFD166));

  lv_obj_t *panel2 = lv_obj_create(scr);
  lv_obj_set_pos(panel2, 30, 220);
  lv_obj_set_size(panel2, 740, 175);
  lv_obj_clear_flag(panel2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(panel2, lv_color_hex(0x151C2A), 0);
  lv_obj_set_style_border_width(panel2, 2, 0);
  lv_obj_set_style_border_color(panel2, lv_color_hex(0x3A465C), 0);
  lv_obj_set_style_radius(panel2, 8, 0);
  lv_obj_set_style_pad_all(panel2, 12, 0);

  counterLabel = lv_label_create(panel2);
  lv_label_set_text(counterLabel, "Counter update: 0");
  lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(counterLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  heapLabel = lv_label_create(panel2);
  lv_label_set_text(heapLabel, "Heap/PSRAM: pending");
  lv_obj_set_style_text_color(heapLabel, lv_color_hex(0xC8D2E6), 0);
  lv_obj_align(heapLabel, LV_ALIGN_TOP_LEFT, 0, 32);

  flushLabel = lv_label_create(panel2);
  lv_label_set_text(flushLabel, "Flush: pending");
  lv_obj_set_style_text_color(flushLabel, lv_color_hex(0xC8D2E6), 0);
  lv_obj_align(flushLabel, LV_ALIGN_TOP_LEFT, 0, 64);

  lv_obj_t *psramCaption = lv_label_create(panel2);
  lv_label_set_text(psramCaption, "Free PSRAM");
  lv_obj_set_style_text_color(psramCaption, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(psramCaption, LV_ALIGN_TOP_LEFT, 0, 105);

  psramBar = lv_bar_create(panel2);
  lv_obj_set_size(psramBar, 300, 18);
  lv_obj_align(psramBar, LV_ALIGN_TOP_LEFT, 120, 103);
  lv_bar_set_range(psramBar, 0, 100);
  lv_bar_set_value(psramBar, 0, LV_ANIM_OFF);

  lv_obj_t *heapCaption = lv_label_create(panel2);
  lv_label_set_text(heapCaption, "Free heap");
  lv_obj_set_style_text_color(heapCaption, lv_color_hex(0xA8B3C7), 0);
  lv_obj_align(heapCaption, LV_ALIGN_TOP_LEFT, 0, 137);

  heapBar = lv_bar_create(panel2);
  lv_obj_set_size(heapBar, 300, 18);
  lv_obj_align(heapBar, LV_ALIGN_TOP_LEFT, 120, 135);
  lv_bar_set_range(heapBar, 0, 100);
  lv_bar_set_value(heapBar, 0, LV_ANIM_OFF);

  lv_obj_t *footer = lv_label_create(scr);
  lv_label_set_text(footer, "Watch for idle stability and 5-second label-update tearing. Border must remain stable.");
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
  Serial.println("[PASS] LVGL static UI objects created");
}

static void updateLabels(bool force = false) {
  const uint32_t now = millis();
  if (!force && now - lastLabelMs < LABEL_UPDATE_MS) {
    return;
  }
  lastLabelMs = now;
  labelUpdates++;

  char buf[128];
  snprintf(buf, sizeof(buf), "Counter update: %lu   uptime: %lu s", (unsigned long)labelUpdates, (unsigned long)(now / 1000));
  lv_label_set_text(counterLabel, buf);

  snprintf(buf, sizeof(buf), "Heap: %lu   PSRAM: %lu   Free PSRAM: %lu",
           (unsigned long)ESP.getFreeHeap(),
           (unsigned long)ESP.getPsramSize(),
           (unsigned long)ESP.getFreePsram());
  lv_label_set_text(heapLabel, buf);

  snprintf(buf, sizeof(buf), "Flush calls: %lu   flushed pixels low32: %lu",
           (unsigned long)flushCount,
           (unsigned long)flushPixels);
  lv_label_set_text(flushLabel, buf);

  const uint32_t psramTotal = ESP.getPsramSize();
  const uint32_t psramFree = ESP.getFreePsram();
  const uint32_t heapFree = ESP.getFreeHeap();

  int psramPct = psramTotal ? static_cast<int>((psramFree * 100ULL) / psramTotal) : 0;
  int heapPct = heapFree > 350000 ? 100 : static_cast<int>((heapFree * 100ULL) / 350000ULL);
  if (heapPct > 100) heapPct = 100;

  lv_bar_set_value(psramBar, psramPct, LV_ANIM_OFF);
  lv_bar_set_value(heapBar, heapPct, LV_ANIM_OFF);

  Serial.printf("[LABEL] update=%lu uptime=%lus flush=%lu heap=%lu psramFree=%lu\n",
                (unsigned long)labelUpdates,
                (unsigned long)(now / 1000),
                (unsigned long)flushCount,
                (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getFreePsram());
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

  Serial.printf("[ALIVE] fw=%s uptime=%lus panel=%s lvgl=%s ui=%s labels=%lu flush=%lu loops=%lu heap=%lu psram=%lu freePsram=%lu\n",
                SKETCH_ID,
                (unsigned long)(now / 1000),
                panelOk ? "OK" : "FAIL",
                lvglOk ? "OK" : "FAIL",
                uiOk ? "OK" : "FAIL",
                (unsigned long)labelUpdates,
                (unsigned long)flushCount,
                (unsigned long)loopCount,
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

  if (!initLvgl()) {
    Serial.println("[STOP] LVGL init failed");
    while (true) {
      alive();
      delay(100);
    }
  }

  createUi();
  updateLabels(true);

  // Force first render before the backlight is enabled.
  for (int i = 0; i < 8; ++i) {
    tickLvgl();
    lv_timer_handler();
    delay(5);
  }

  backlightOn();
  Serial.println("[PASS] Backlight ON after LVGL first draw");
  Serial.println("[READY] Watch idle screen and 5-second label updates. No touch and no animation are active.");
}

void loop() {
  loopCount++;
  tickLvgl();
  updateLabels(false);
  lv_timer_handler();
  alive();
  delay(5);
}
