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

#define SKETCH_ID "13LVGL-ELCDS1-240827A"

#ifndef ESP_LCD_STATIC_PCLK_HZ
#define ESP_LCD_STATIC_PCLK_HZ 12500000
#endif

#ifndef ESP_LCD_STATIC_USE_RGB565_BUS_ORDER
#define ESP_LCD_STATIC_USE_RGB565_BUS_ORDER 1
#endif

#ifndef ESP_LCD_STATIC_DOUBLE_FB
#define ESP_LCD_STATIC_DOUBLE_FB 1
#endif

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t LVGL_BUFFER_LINES = 100;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t LABEL_UPDATE_INTERVAL_MS = 5000;
static constexpr uint32_t LVGL_LOOP_DELAY_MS = 5;

static esp_lcd_panel_handle_t panel = nullptr;

static lv_disp_draw_buf_t drawBuf;
static lv_disp_drv_t dispDrv;
static lv_color_t *lvBuf1 = nullptr;
static lv_color_t *lvBuf2 = nullptr;

static lv_obj_t *statusLabel = nullptr;
static lv_obj_t *counterLabel = nullptr;
static lv_obj_t *flushLabel = nullptr;
static lv_obj_t *heapLabel = nullptr;

static bool displayOk = false;
static bool lvglOk = false;
static bool uiOk = false;

static uint32_t flushCount = 0;
static uint32_t lvglLoops = 0;
static uint32_t lastAliveMs = 0;
static uint32_t lastLabelUpdateMs = 0;
static uint32_t lastLvTickMs = 0;
static uint32_t labelUpdateCount = 0;
static uint32_t maxFlushPixels = 0;
static lv_area_t lastFlushArea = {0, 0, 0, 0};

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
#ifdef CONFIG_IDF_TARGET
  Serial.printf("%-28s: \"%s\"\n", "CONFIG_IDF_TARGET", CONFIG_IDF_TARGET);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_IDF_TARGET");
#endif
  Serial.printf("%-28s: %d.%d.%d\n", "LVGL version", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("%-28s: %d\n", "LV_COLOR_DEPTH", LV_COLOR_DEPTH);
  Serial.printf("%-28s: %u bytes\n", "Flash", ESP.getFlashChipSize());
  Serial.printf("%-28s: %u bytes\n", "PSRAM", ESP.getPsramSize());
  Serial.printf("%-28s: %u bytes\n", "Free PSRAM", ESP.getFreePsram());
  Serial.printf("%-28s: %u bytes\n", "Free heap", ESP.getFreeHeap());
  printDivider();
  Serial.println("Mode                     : LVGL static UI over esp_lcd RGB panel");
  Serial.println("Arduino_GFX              : not used");
  Serial.println("GT911 touch              : not used");
  Serial.println("Moving animation         : not used");
  Serial.printf("Resolution               : %dx%d\n", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("PCLK                     : %u Hz\n", (unsigned)ESP_LCD_STATIC_PCLK_HZ);
  Serial.println("Porches                  : HSYNC 8/4/8, VSYNC 8/4/8");
  Serial.printf("LVGL buffer lines        : %lu\n", static_cast<unsigned long>(LVGL_BUFFER_LINES));
#if ESP_LCD_STATIC_USE_RGB565_BUS_ORDER
  Serial.println("Data order               : ESP-IDF RGB565 bus bits DATA0..15 = B0..B4,G0..G5,R0..R4");
#else
  Serial.println("Data order               : Arduino_GFX label order DATA0..15 = R0..R4,G0..G5,B0..B4");
#endif
  printDivider();
}

static void printPinMap() {
  Serial.println("[PIN MAP]");
  Serial.printf("DE=%d HSYNC=%d VSYNC=%d PCLK=%d BL=%d\n", RGB_DE, RGB_HSYNC, RGB_VSYNC, RGB_PCLK, BACKLIGHT);
  Serial.printf("R0..R4=%d,%d,%d,%d,%d\n", RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4);
  Serial.printf("G0..G5=%d,%d,%d,%d,%d,%d\n", RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5);
  Serial.printf("B0..B4=%d,%d,%d,%d,%d\n", RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4);
}

static void setDataPins(esp_lcd_rgb_panel_config_t &cfg) {
#if ESP_LCD_STATIC_USE_RGB565_BUS_ORDER
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
#else
  cfg.data_gpio_nums[0]  = RGB_R0;
  cfg.data_gpio_nums[1]  = RGB_R1;
  cfg.data_gpio_nums[2]  = RGB_R2;
  cfg.data_gpio_nums[3]  = RGB_R3;
  cfg.data_gpio_nums[4]  = RGB_R4;
  cfg.data_gpio_nums[5]  = RGB_G0;
  cfg.data_gpio_nums[6]  = RGB_G1;
  cfg.data_gpio_nums[7]  = RGB_G2;
  cfg.data_gpio_nums[8]  = RGB_G3;
  cfg.data_gpio_nums[9]  = RGB_G4;
  cfg.data_gpio_nums[10] = RGB_G5;
  cfg.data_gpio_nums[11] = RGB_B0;
  cfg.data_gpio_nums[12] = RGB_B1;
  cfg.data_gpio_nums[13] = RGB_B2;
  cfg.data_gpio_nums[14] = RGB_B3;
  cfg.data_gpio_nums[15] = RGB_B4;
#endif
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

  return true;
}

static bool allocateLvglBuffers() {
  const size_t pixels = static_cast<size_t>(LCD_WIDTH) * LVGL_BUFFER_LINES;
  const size_t bytes = pixels * sizeof(lv_color_t);

  lvBuf1 = static_cast<lv_color_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  lvBuf2 = static_cast<lv_color_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

  if (lvBuf1 == nullptr || lvBuf2 == nullptr) {
    Serial.println("[FAIL] LVGL PSRAM draw buffer allocation failed");
    Serial.printf("buf1=%p buf2=%p requested=%u bytes each\n", lvBuf1, lvBuf2, static_cast<unsigned>(bytes));
    return false;
  }

  Serial.printf("[PASS] lvBuf1 allocated in PSRAM: %u bytes at %p\n", static_cast<unsigned>(bytes), lvBuf1);
  Serial.printf("[PASS] lvBuf2 allocated in PSRAM: %u bytes at %p\n", static_cast<unsigned>(bytes), lvBuf2);
  return true;
}

static void lvglFlush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorMap) {
  (void)drv;

  if (panel == nullptr) {
    lv_disp_flush_ready(drv);
    return;
  }

  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  const uint32_t pixels = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);

  flushCount++;
  lastFlushArea = *area;
  if (pixels > maxFlushPixels) {
    maxFlushPixels = pixels;
  }

  if (flushCount <= 8 || (flushCount % 100) == 0) {
    Serial.printf("[FLUSH] #%lu area=(%d,%d)-(%d,%d) size=%ldx%ld pixels=%lu\n",
                  static_cast<unsigned long>(flushCount),
                  area->x1, area->y1, area->x2, area->y2,
                  static_cast<long>(w), static_cast<long>(h),
                  static_cast<unsigned long>(pixels));
  }

  esp_err_t err = esp_lcd_panel_draw_bitmap(panel,
                                            area->x1,
                                            area->y1,
                                            area->x2 + 1,
                                            area->y2 + 1,
                                            colorMap);
  if (err != ESP_OK) {
    Serial.printf("[WARN] esp_lcd_panel_draw_bitmap(): %s\n", esp_err_to_name(err));
  }

  lv_disp_flush_ready(drv);
}

static lv_obj_t *makeCard(lv_obj_t *parent, int x, int y, int w, int h, const char *title, const char *body, lv_color_t accent) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, accent, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x111827), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *titleObj = lv_label_create(card);
  lv_label_set_text(titleObj, title);
  lv_obj_set_style_text_color(titleObj, accent, 0);
  lv_obj_set_style_text_font(titleObj, &lv_font_montserrat_18, 0);
  lv_obj_align(titleObj, LV_ALIGN_TOP_LEFT, 8, 4);

  lv_obj_t *bodyObj = lv_label_create(card);
  lv_label_set_text(bodyObj, body);
  lv_obj_set_style_text_color(bodyObj, lv_color_hex(0xE5E7EB), 0);
  lv_obj_set_style_text_font(bodyObj, &lv_font_montserrat_14, 0);
  lv_obj_set_width(bodyObj, w - 18);
  lv_obj_align(bodyObj, LV_ALIGN_TOP_LEFT, 8, 34);

  return card;
}

static void createUi() {
  Serial.println("[LVGL UI]");

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x030712), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "ESP32-8048S043 / LVGL over esp_lcd / STATIC");
  lv_obj_set_style_text_color(title, lv_color_hex(0xF9FAFB), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 16);

  statusLabel = lv_label_create(scr);
  lv_label_set_text(statusLabel, "No touch. No moving block. Rare label updates only.");
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x9CA3AF), 0);
  lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(statusLabel, LV_ALIGN_TOP_LEFT, 26, 48);

  makeCard(scr, 24, 86, 230, 128, "Display", "esp_lcd RGB panel\nPCLK 12.5 MHz\nPSRAM framebuffer", lv_color_hex(0x38BDF8));
  makeCard(scr, 284, 86, 230, 128, "LVGL", "LVGL 8.x\n2x 100-line PSRAM buffers\nstatic UI", lv_color_hex(0xA78BFA));
  makeCard(scr, 544, 86, 230, 128, "Boundary", "No touch\nNo raw animation\nWatch idle stability", lv_color_hex(0x34D399));

  lv_obj_t *bar1 = lv_bar_create(scr);
  lv_obj_set_pos(bar1, 44, 252);
  lv_obj_set_size(bar1, 690, 18);
  lv_bar_set_range(bar1, 0, 100);
  lv_bar_set_value(bar1, 80, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar1, lv_color_hex(0x1F2937), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar1, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);

  lv_obj_t *bar2 = lv_bar_create(scr);
  lv_obj_set_pos(bar2, 44, 292);
  lv_obj_set_size(bar2, 690, 18);
  lv_bar_set_range(bar2, 0, 100);
  lv_bar_set_value(bar2, 60, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar2, lv_color_hex(0x1F2937), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar2, lv_color_hex(0xA78BFA), LV_PART_INDICATOR);

  lv_obj_t *barLabel1 = lv_label_create(scr);
  lv_label_set_text(barLabel1, "Static bars: no animation, no touch, no periodic redraw");
  lv_obj_set_style_text_color(barLabel1, lv_color_hex(0xD1D5DB), 0);
  lv_obj_set_style_text_font(barLabel1, &lv_font_montserrat_14, 0);
  lv_obj_align_to(barLabel1, bar2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

  counterLabel = lv_label_create(scr);
  lv_label_set_text(counterLabel, "counter: 0");
  lv_obj_set_style_text_color(counterLabel, lv_color_hex(0xFBBF24), 0);
  lv_obj_set_style_text_font(counterLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(counterLabel, LV_ALIGN_BOTTOM_LEFT, 26, -58);

  flushLabel = lv_label_create(scr);
  lv_label_set_text(flushLabel, "flush: boot");
  lv_obj_set_style_text_color(flushLabel, lv_color_hex(0x93C5FD), 0);
  lv_obj_set_style_text_font(flushLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(flushLabel, LV_ALIGN_BOTTOM_LEFT, 26, -30);

  heapLabel = lv_label_create(scr);
  lv_label_set_text(heapLabel, "heap: boot");
  lv_obj_set_style_text_color(heapLabel, lv_color_hex(0x86EFAC), 0);
  lv_obj_set_style_text_font(heapLabel, &lv_font_montserrat_14, 0);
  lv_obj_align(heapLabel, LV_ALIGN_BOTTOM_RIGHT, -26, -30);

  lv_obj_t *frame = lv_obj_create(scr);
  lv_obj_set_pos(frame, 8, 8);
  lv_obj_set_size(frame, LCD_WIDTH - 16, LCD_HEIGHT - 16);
  lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(frame, 2, 0);
  lv_obj_set_style_border_color(frame, lv_color_hex(0xF9FAFB), 0);
  lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

  uiOk = true;
  Serial.println("[PASS] LVGL static UI objects created");
}

static bool initLvgl() {
  Serial.println("[LVGL INIT]");

  if (!allocateLvglBuffers()) {
    return false;
  }

  lv_init();

  const uint32_t drawPixels = LCD_WIDTH * LVGL_BUFFER_LINES;
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, drawPixels);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_WIDTH;
  dispDrv.ver_res = LCD_HEIGHT;
  dispDrv.flush_cb = lvglFlush;
  dispDrv.draw_buf = &drawBuf;
  dispDrv.user_data = panel;
  lv_disp_drv_register(&dispDrv);

  lvglOk = true;
  Serial.println("[PASS] LVGL display driver registered");

  createUi();

  // Draw first screen before turning the backlight on.
  for (int i = 0; i < 10; ++i) {
    lv_timer_handler();
    delay(10);
  }

  return true;
}

static void updateLabelsIfNeeded() {
  const uint32_t now = millis();
  if (now - lastLabelUpdateMs < LABEL_UPDATE_INTERVAL_MS) {
    return;
  }
  lastLabelUpdateMs = now;
  labelUpdateCount++;

  char buf[96];
  snprintf(buf, sizeof(buf), "counter: %lu / uptime: %lus", static_cast<unsigned long>(labelUpdateCount), static_cast<unsigned long>(now / 1000));
  lv_label_set_text(counterLabel, buf);

  snprintf(buf, sizeof(buf), "flush: %lu / max area: %lu px / last: (%d,%d)-(%d,%d)",
           static_cast<unsigned long>(flushCount),
           static_cast<unsigned long>(maxFlushPixels),
           lastFlushArea.x1, lastFlushArea.y1, lastFlushArea.x2, lastFlushArea.y2);
  lv_label_set_text(flushLabel, buf);

  snprintf(buf, sizeof(buf), "heap %lu / psram %lu",
           static_cast<unsigned long>(ESP.getFreeHeap()),
           static_cast<unsigned long>(ESP.getFreePsram()));
  lv_label_set_text(heapLabel, buf);

  Serial.printf("[LABEL] update=%lu uptime=%lus flush=%lu maxFlushPixels=%lu freeHeap=%u freePsram=%u\n",
                static_cast<unsigned long>(labelUpdateCount),
                static_cast<unsigned long>(now / 1000),
                static_cast<unsigned long>(flushCount),
                static_cast<unsigned long>(maxFlushPixels),
                ESP.getFreeHeap(),
                ESP.getFreePsram());
}

static void tickLvgl() {
  const uint32_t now = millis();
  if (lastLvTickMs == 0) {
    lastLvTickMs = now;
    return;
  }

  const uint32_t elapsed = now - lastLvTickMs;
  if (elapsed > 0) {
    lv_tick_inc(elapsed);
    lastLvTickMs = now;
  }
}

static void alive() {
  const uint32_t now = millis();
  if (now - lastAliveMs < ALIVE_INTERVAL_MS) {
    return;
  }
  lastAliveMs = now;

  Serial.printf("[ALIVE] fw=%s uptime=%lus display=%s lvgl=%s ui=%s loops=%lu labels=%lu flush=%lu maxFlushPixels=%lu heap=%u psram=%u freePsram=%u\n",
                SKETCH_ID,
                static_cast<unsigned long>(now / 1000),
                displayOk ? "OK" : "FAIL",
                lvglOk ? "OK" : "FAIL",
                uiOk ? "OK" : "FAIL",
                static_cast<unsigned long>(lvglLoops),
                static_cast<unsigned long>(labelUpdateCount),
                static_cast<unsigned long>(flushCount),
                static_cast<unsigned long>(maxFlushPixels),
                ESP.getFreeHeap(),
                ESP.getPsramSize(),
                ESP.getFreePsram());
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  printBanner();
  printPinMap();

  displayOk = initEspLcdPanel();
  if (!displayOk) {
    Serial.println("[STOP] Display init failed; backlight remains off.");
    while (true) {
      alive();
      delay(100);
    }
  }

  if (!initLvgl()) {
    Serial.println("[STOP] LVGL init failed; backlight remains off.");
    while (true) {
      alive();
      delay(100);
    }
  }

  backlightOn();
  Serial.println("[PASS] Backlight ON after LVGL first draw");
  Serial.println("[READY] Watch idle screen and 5-second label updates. No touch and no animation are active.");
}

void loop() {
  tickLvgl();
  updateLabelsIfNeeded();
  lv_timer_handler();
  lvglLoops++;
  alive();
  delay(LVGL_LOOP_DELAY_MS);
}
