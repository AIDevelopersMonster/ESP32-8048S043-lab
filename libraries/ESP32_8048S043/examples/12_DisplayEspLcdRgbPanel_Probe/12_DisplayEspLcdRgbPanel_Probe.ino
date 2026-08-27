/*
  ESP32-8048S043 Lab / 12_DisplayEspLcdRgbPanel_Probe

  Purpose:
    Isolated ESP-IDF esp_lcd RGB-panel transport probe for ESP32-8048S043.

  Why this exists:
    10_LVGL_BasicUI and 11_LVGL_Dashboard proved that LVGL can run on the
    board, but dynamic touch/UI behavior through the current Arduino_GFX path
    is not acceptable for user-facing applications.

    Third-party references for this board family repeatedly use the native
    ESP-IDF esp_lcd RGB panel path with PSRAM framebuffer support. This sketch
    tests that transport layer before we involve LVGL or GT911 again.

  What this example checks:
    - esp_lcd_new_rgb_panel() can initialize the 800x480 RGB panel;
    - ESP-IDF RGB565 data bit order through data_gpio_nums[0..15];
    - PSRAM framebuffer allocation through fb_in_psram=true;
    - double framebuffer mode through num_fbs=2 and double_fb=true;
    - candidate RGB timing: 12.5 MHz or 18 MHz with 8/4/8 porches;
    - backlight GPIO2 after panel initialization;
    - stable static color bars, quadrants and grid patterns.

  What this example intentionally does NOT use:
    - Arduino_GFX;
    - LVGL;
    - GT911 touch;
    - SD/Wi-Fi/BLE;
    - user-facing UI widgets.

  First-run default:
    PCLK 12.5 MHz, 8/4/8 porches, RGB565 bus-bit order.

  If first-run display is stable and colors are correct, the next probe is
  changing ESP_LCD_PROBE_PCLK_HZ from 12500000 to 18000000.
*/

#include <Arduino.h>
#include <ESP32_8048S043_Pins.h>

extern "C" {
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_system.h"
}

using namespace esp32_8048s043::pins;

#define SKETCH_ID "12ELCD-PROBE1-240827A"

#ifndef ESP_LCD_PROBE_PCLK_HZ
#define ESP_LCD_PROBE_PCLK_HZ 12500000
#endif

#ifndef ESP_LCD_PROBE_USE_RGB565_BUS_ORDER
#define ESP_LCD_PROBE_USE_RGB565_BUS_ORDER 1
#endif

#ifndef ESP_LCD_PROBE_DOUBLE_FB
#define ESP_LCD_PROBE_DOUBLE_FB 1
#endif

static constexpr uint16_t COLOR_BLACK   = 0x0000;
static constexpr uint16_t COLOR_WHITE   = 0xFFFF;
static constexpr uint16_t COLOR_RED     = 0xF800;
static constexpr uint16_t COLOR_GREEN   = 0x07E0;
static constexpr uint16_t COLOR_BLUE    = 0x001F;
static constexpr uint16_t COLOR_YELLOW  = 0xFFE0;
static constexpr uint16_t COLOR_CYAN    = 0x07FF;
static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
static constexpr uint16_t COLOR_GRAY    = 0x8410;

static esp_lcd_panel_handle_t panel = nullptr;
static uint16_t *lineBuffer = nullptr;
static uint32_t frameCounter = 0;
static uint32_t lastAliveMs = 0;

static void backlightOff() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, LOW);
}

static void backlightOn() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
}

static void printBanner() {
  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 12_DisplayEspLcdRgbPanel_Probe");
  Serial.println(" Native esp_lcd RGB panel transport probe");
  Serial.println("================================================================");
  Serial.print("Firmware ID              : ");
  Serial.println(SKETCH_ID);
  Serial.print("ESP-IDF SDK              : ");
  Serial.println(esp_get_idf_version());
  Serial.print("Arduino board            : ");
#ifdef ARDUINO_BOARD
  Serial.println(ARDUINO_BOARD);
#else
  Serial.println("<not defined>");
#endif
  Serial.print("Arduino variant          : ");
#ifdef ARDUINO_VARIANT
  Serial.println(ARDUINO_VARIANT);
#else
  Serial.println("<not defined>");
#endif
  Serial.printf("Flash                    : %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("PSRAM                    : %u bytes\n", ESP.getPsramSize());
  Serial.printf("Free heap                : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM               : %u bytes\n", ESP.getFreePsram());
  Serial.println("----------------------------------------------------------------");
  Serial.println("Mode                     : esp_lcd RGB panel only");
  Serial.println("Arduino_GFX              : not used");
  Serial.println("LVGL                     : not used");
  Serial.println("GT911 touch              : not used");
  Serial.printf("Resolution               : %dx%d\n", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("PCLK                     : %u Hz\n", (unsigned)ESP_LCD_PROBE_PCLK_HZ);
  Serial.println("Porches                  : HSYNC 8/4/8, VSYNC 8/4/8");
  Serial.printf("Framebuffer in PSRAM     : true\n");
  Serial.printf("Double framebuffer       : %s\n", ESP_LCD_PROBE_DOUBLE_FB ? "true" : "false");
#if ESP_LCD_PROBE_USE_RGB565_BUS_ORDER
  Serial.println("Data order               : ESP-IDF RGB565 bus bits DATA0..15 = B0..B4,G0..G5,R0..R4");
#else
  Serial.println("Data order               : Arduino_GFX label order DATA0..15 = R0..R4,G0..G5,B0..B4");
#endif
  Serial.println("----------------------------------------------------------------");
}

static void printPinMap() {
  Serial.println("[PIN MAP]");
  Serial.printf("DE=%d HSYNC=%d VSYNC=%d PCLK=%d BL=%d\n", RGB_DE, RGB_HSYNC, RGB_VSYNC, RGB_PCLK, BACKLIGHT);
  Serial.printf("R0..R4=%d,%d,%d,%d,%d\n", RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4);
  Serial.printf("G0..G5=%d,%d,%d,%d,%d,%d\n", RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5);
  Serial.printf("B0..B4=%d,%d,%d,%d,%d\n", RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4);
  Serial.printf("Touch pins present but unused here: SDA=%d SCL=%d RST=%d INT=%d\n", TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
}

static bool allocateLineBuffer() {
  lineBuffer = static_cast<uint16_t *>(heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
  if (lineBuffer == nullptr) {
    Serial.println("[WARN] Internal DMA line buffer allocation failed, trying generic 8-bit heap");
    lineBuffer = static_cast<uint16_t *>(heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_8BIT));
  }

  if (lineBuffer == nullptr) {
    Serial.println("[FAIL] line buffer allocation failed");
    return false;
  }

  Serial.printf("[PASS] line buffer allocated: %u bytes at %p\n", (unsigned)(LCD_WIDTH * sizeof(uint16_t)), lineBuffer);
  return true;
}

static void setDataPins(esp_lcd_rgb_panel_config_t &cfg) {
#if ESP_LCD_PROBE_USE_RGB565_BUS_ORDER
  // ESP-IDF rgb_panel data_gpio_nums[] are RGB565 bus bits, not human color labels.
  // DATA0..4  = B0..B4, DATA5..10 = G0..G5, DATA11..15 = R0..R4.
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
  // Compile-time fallback to compare with the old Arduino_GFX constructor label order.
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
  cfg.timings.pclk_hz = ESP_LCD_PROBE_PCLK_HZ;
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
  cfg.num_fbs = ESP_LCD_PROBE_DOUBLE_FB ? 2 : 1;
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
  cfg.flags.double_fb = ESP_LCD_PROBE_DOUBLE_FB ? true : false;
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

  return true;
}

static void fillRect(int x, int y, int w, int h, uint16_t color) {
  if (panel == nullptr || lineBuffer == nullptr) {
    return;
  }

  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > LCD_WIDTH) { w = LCD_WIDTH - x; }
  if (y + h > LCD_HEIGHT) { h = LCD_HEIGHT - y; }
  if (w <= 0 || h <= 0) { return; }

  for (int i = 0; i < w; ++i) {
    lineBuffer[i] = color;
  }

  for (int row = 0; row < h; ++row) {
    esp_lcd_panel_draw_bitmap(panel, x, y + row, x + w, y + row + 1, lineBuffer);
  }
}

static void drawFrame(uint16_t color, int thickness) {
  fillRect(0, 0, LCD_WIDTH, thickness, color);
  fillRect(0, LCD_HEIGHT - thickness, LCD_WIDTH, thickness, color);
  fillRect(0, 0, thickness, LCD_HEIGHT, color);
  fillRect(LCD_WIDTH - thickness, 0, thickness, LCD_HEIGHT, color);
}

static void drawColorBars() {
  Serial.println("[SCREEN] RGB color bars");
  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE,
    COLOR_BLACK, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
  };
  const int count = sizeof(colors) / sizeof(colors[0]);
  const int barW = LCD_WIDTH / count;

  for (int i = 0; i < count; ++i) {
    const int x = i * barW;
    const int w = (i == count - 1) ? (LCD_WIDTH - x) : barW;
    fillRect(x, 0, w, LCD_HEIGHT, colors[i]);
  }
  drawFrame(COLOR_WHITE, 2);
  frameCounter++;
}

static void drawQuadrants() {
  Serial.println("[SCREEN] orientation quadrants");
  fillRect(0, 0, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_RED);
  fillRect(LCD_WIDTH / 2, 0, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_GREEN);
  fillRect(0, LCD_HEIGHT / 2, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_BLUE);
  fillRect(LCD_WIDTH / 2, LCD_HEIGHT / 2, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_WHITE);

  fillRect(LCD_WIDTH / 2 - 2, 0, 4, LCD_HEIGHT, COLOR_BLACK);
  fillRect(0, LCD_HEIGHT / 2 - 2, LCD_WIDTH, 4, COLOR_BLACK);
  drawFrame(COLOR_BLACK, 4);
  frameCounter++;
}

static void drawStripeGrid() {
  Serial.println("[SCREEN] stripe/grid data-line pattern");
  for (int y = 0; y < LCD_HEIGHT; y += 16) {
    uint16_t c;
    switch ((y / 16) % 8) {
      case 0: c = COLOR_RED; break;
      case 1: c = COLOR_GREEN; break;
      case 2: c = COLOR_BLUE; break;
      case 3: c = COLOR_YELLOW; break;
      case 4: c = COLOR_CYAN; break;
      case 5: c = COLOR_MAGENTA; break;
      case 6: c = COLOR_GRAY; break;
      default: c = COLOR_BLACK; break;
    }
    fillRect(0, y, LCD_WIDTH, min(16, LCD_HEIGHT - y), c);
  }

  for (int x = 0; x < LCD_WIDTH; x += 40) {
    fillRect(x, 0, 2, LCD_HEIGHT, COLOR_WHITE);
  }
  for (int y = 0; y < LCD_HEIGHT; y += 40) {
    fillRect(0, y, LCD_WIDTH, 2, COLOR_WHITE);
  }
  drawFrame(COLOR_WHITE, 2);
  frameCounter++;
}

static void drawMovingBlockProbe() {
  Serial.println("[SCREEN] small update probe");
  fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
  drawFrame(COLOR_WHITE, 2);

  for (int x = 0; x <= LCD_WIDTH - 120; x += 20) {
    fillRect(2, LCD_HEIGHT / 2 - 45, LCD_WIDTH - 4, 90, COLOR_BLACK);
    fillRect(x, LCD_HEIGHT / 2 - 40, 120, 80, COLOR_CYAN);
    fillRect(x + 10, LCD_HEIGHT / 2 - 30, 100, 60, COLOR_BLUE);
    delay(35);
  }
  frameCounter++;
}

static void drawSolid(uint16_t color, const char *name) {
  Serial.print("[SCREEN] solid ");
  Serial.println(name);
  fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
  frameCounter++;
}

static void alive() {
  const uint32_t now = millis();
  if (now - lastAliveMs < 5000) {
    return;
  }
  lastAliveMs = now;

  Serial.printf("[ALIVE] fw=%s uptime=%lus frames=%lu pclk=%u heap=%u psram=%u freePsram=%u\n",
                SKETCH_ID,
                static_cast<unsigned long>(now / 1000),
                static_cast<unsigned long>(frameCounter),
                (unsigned)ESP_LCD_PROBE_PCLK_HZ,
                ESP.getFreeHeap(),
                ESP.getPsramSize(),
                ESP.getFreePsram());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  printBanner();
  printPinMap();

  if (!allocateLineBuffer()) {
    while (true) {
      delay(1000);
    }
  }

  if (!initEspLcdPanel()) {
    Serial.println("[STOP] esp_lcd panel probe failed before backlight-on");
    while (true) {
      alive();
      delay(100);
    }
  }

  drawSolid(COLOR_BLACK, "BLACK before backlight-on");
  backlightOn();
  Serial.println("[PASS] Backlight ON after panel init");
  Serial.println("[READY] Watch screen: correct colors, stable image, no random tearing/noise.");
}

void loop() {
  drawColorBars();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawQuadrants();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawStripeGrid();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawMovingBlockProbe();
  for (int i = 0; i < 20; ++i) { alive(); delay(100); }

  drawSolid(COLOR_RED, "RED");
  for (int i = 0; i < 10; ++i) { alive(); delay(100); }
  drawSolid(COLOR_GREEN, "GREEN");
  for (int i = 0; i < 10; ++i) { alive(); delay(100); }
  drawSolid(COLOR_BLUE, "BLUE");
  for (int i = 0; i < 10; ++i) { alive(); delay(100); }
  drawSolid(COLOR_WHITE, "WHITE");
  for (int i = 0; i < 10; ++i) { alive(); delay(100); }
}
