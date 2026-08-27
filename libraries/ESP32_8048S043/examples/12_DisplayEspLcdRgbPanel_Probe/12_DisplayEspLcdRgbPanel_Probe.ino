/*
  ESP32-8048S043 Lab / 12_DisplayEspLcdRgbPanel_Probe

  Purpose:
    Isolated ESP-IDF esp_lcd RGB-panel transport probe for ESP32-8048S043.

  Result from first physical run:
    - esp_lcd transport initialized;
    - colors were correct;
    - row-by-row rectangle updates produced flicker and temporary geometry breaks.

  This revision changes the dynamic probe from row-by-row draw_bitmap calls to
  bulk buffer updates:
    - full-screen patterns are drawn from one full PSRAM frame buffer;
    - moving block band is drawn from one full-width band buffer;
    - no LVGL, no Arduino_GFX, no GT911.

  This separates two effects:
    1. panel transport and color order;
    2. update granularity / tearing behavior.
*/

#include <Arduino.h>
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

#define SKETCH_ID "12ELCD-BULK1-240827B"

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

static constexpr int BAND_H = 96;
static constexpr int BAND_Y = (LCD_HEIGHT - BAND_H) / 2;
static constexpr int BLOCK_W = 120;
static constexpr int BLOCK_H = 80;
static constexpr int BLOCK_Y = (BAND_H - BLOCK_H) / 2;

static esp_lcd_panel_handle_t panel = nullptr;
static uint16_t *frameBuffer = nullptr;
static uint16_t *bandBuffer = nullptr;
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
  Serial.println("Update mode              : bulk full-frame / bulk band draw_bitmap calls");
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

static bool allocateDrawBuffers() {
  const size_t frameBytes = (size_t)LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
  const size_t bandBytes = (size_t)LCD_WIDTH * BAND_H * sizeof(uint16_t);

  frameBuffer = static_cast<uint16_t *>(heap_caps_malloc(frameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (frameBuffer == nullptr) {
    Serial.printf("[FAIL] full frame PSRAM buffer allocation failed: %u bytes\n", (unsigned)frameBytes);
    return false;
  }
  Serial.printf("[PASS] full frame buffer allocated in PSRAM: %u bytes at %p\n", (unsigned)frameBytes, frameBuffer);

  bandBuffer = static_cast<uint16_t *>(heap_caps_malloc(bandBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (bandBuffer == nullptr) {
    Serial.printf("[FAIL] band PSRAM buffer allocation failed: %u bytes\n", (unsigned)bandBytes);
    return false;
  }
  Serial.printf("[PASS] band buffer allocated in PSRAM: %u bytes at %p\n", (unsigned)bandBytes, bandBuffer);
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

static void fillPixels(uint16_t *buf, size_t count, uint16_t color) {
  for (size_t i = 0; i < count; ++i) {
    buf[i] = color;
  }
}

static bool drawBitmap(int x, int y, int w, int h, const uint16_t *buf, const char *tag) {
  esp_err_t err = esp_lcd_panel_draw_bitmap(panel, x, y, x + w, y + h, buf);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] draw_bitmap %s: %s\n", tag, esp_err_to_name(err));
    return false;
  }
  return true;
}

static void putPixel(uint16_t *buf, int width, int x, int y, uint16_t color) {
  buf[y * width + x] = color;
}

static void fillRectInBuffer(uint16_t *buf, int bufW, int bufH, int x, int y, int w, int h, uint16_t color) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > bufW) { w = bufW - x; }
  if (y + h > bufH) { h = bufH - y; }
  if (w <= 0 || h <= 0) { return; }

  for (int yy = y; yy < y + h; ++yy) {
    uint16_t *row = buf + yy * bufW + x;
    for (int xx = 0; xx < w; ++xx) {
      row[xx] = color;
    }
  }
}

static void drawFrameInBuffer(uint16_t *buf, int bufW, int bufH, uint16_t color, int thickness) {
  fillRectInBuffer(buf, bufW, bufH, 0, 0, bufW, thickness, color);
  fillRectInBuffer(buf, bufW, bufH, 0, bufH - thickness, bufW, thickness, color);
  fillRectInBuffer(buf, bufW, bufH, 0, 0, thickness, bufH, color);
  fillRectInBuffer(buf, bufW, bufH, bufW - thickness, 0, thickness, bufH, color);
}

static void drawColorBars() {
  Serial.println("[SCREEN] RGB color bars / one full-frame draw");
  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE,
    COLOR_BLACK, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
  };
  const int count = sizeof(colors) / sizeof(colors[0]);
  const int barW = LCD_WIDTH / count;

  fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, COLOR_BLACK);
  for (int i = 0; i < count; ++i) {
    const int x = i * barW;
    const int w = (i == count - 1) ? (LCD_WIDTH - x) : barW;
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, x, 0, w, LCD_HEIGHT, colors[i]);
  }
  drawFrameInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE, 2);
  drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, "colorbars");
  frameCounter++;
}

static void drawQuadrants() {
  Serial.println("[SCREEN] orientation quadrants / one full-frame draw");
  fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, COLOR_BLACK);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, 0, 0, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_RED);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH / 2, 0, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_GREEN);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, 0, LCD_HEIGHT / 2, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_BLUE);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH / 2, LCD_HEIGHT / 2, LCD_WIDTH / 2, LCD_HEIGHT / 2, COLOR_WHITE);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH / 2 - 2, 0, 4, LCD_HEIGHT, COLOR_BLACK);
  fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, 0, LCD_HEIGHT / 2 - 2, LCD_WIDTH, 4, COLOR_BLACK);
  drawFrameInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK, 4);
  drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, "quadrants");
  frameCounter++;
}

static void drawStripeGrid() {
  Serial.println("[SCREEN] stripe/grid data-line pattern / one full-frame draw");
  fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, COLOR_BLACK);
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
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, 0, y, LCD_WIDTH, min(16, LCD_HEIGHT - y), c);
  }

  for (int x = 0; x < LCD_WIDTH; x += 40) {
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, x, 0, 2, LCD_HEIGHT, COLOR_WHITE);
  }
  for (int y = 0; y < LCD_HEIGHT; y += 40) {
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, 0, y, LCD_WIDTH, 2, COLOR_WHITE);
  }
  drawFrameInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE, 2);
  drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, "stripe-grid");
  frameCounter++;
}

static void drawMovingBandProbe() {
  Serial.println("[SCREEN] moving block / one band draw per step");
  fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, COLOR_BLACK);
  drawFrameInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE, 2);
  drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, "moving-band background");

  for (int x = 0; x <= LCD_WIDTH - BLOCK_W; x += 20) {
    fillPixels(bandBuffer, (size_t)LCD_WIDTH * BAND_H, COLOR_BLACK);
    fillRectInBuffer(bandBuffer, LCD_WIDTH, BAND_H, x, BLOCK_Y, BLOCK_W, BLOCK_H, COLOR_CYAN);
    fillRectInBuffer(bandBuffer, LCD_WIDTH, BAND_H, x + 10, BLOCK_Y + 10, BLOCK_W - 20, BLOCK_H - 20, COLOR_BLUE);
    drawBitmap(0, BAND_Y, LCD_WIDTH, BAND_H, bandBuffer, "moving-band");
    delay(45);
  }
  frameCounter++;
}

static void drawMovingFullFrameProbe() {
  Serial.println("[SCREEN] moving block / one full-frame draw per step");
  for (int x = 0; x <= LCD_WIDTH - BLOCK_W; x += 40) {
    fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, COLOR_BLACK);
    drawFrameInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE, 2);
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, x, (LCD_HEIGHT - BLOCK_H) / 2, BLOCK_W, BLOCK_H, COLOR_CYAN);
    fillRectInBuffer(frameBuffer, LCD_WIDTH, LCD_HEIGHT, x + 10, (LCD_HEIGHT - BLOCK_H) / 2 + 10, BLOCK_W - 20, BLOCK_H - 20, COLOR_BLUE);
    drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, "moving-full-frame");
    delay(70);
  }
  frameCounter++;
}

static void drawSolid(uint16_t color, const char *name) {
  Serial.print("[SCREEN] solid / one full-frame draw: ");
  Serial.println(name);
  fillPixels(frameBuffer, (size_t)LCD_WIDTH * LCD_HEIGHT, color);
  drawBitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer, name);
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

  if (!allocateDrawBuffers()) {
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
  Serial.println("[READY] Watch screen: compare moving band vs moving full-frame behavior.");
}

void loop() {
  drawColorBars();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawQuadrants();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawStripeGrid();
  for (int i = 0; i < 30; ++i) { alive(); delay(100); }

  drawMovingBandProbe();
  for (int i = 0; i < 20; ++i) { alive(); delay(100); }

  drawMovingFullFrameProbe();
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
