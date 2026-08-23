/*
  ESP32-8048S043 Lab / 02_DisplayRGBTest

  Purpose:
    First minimal RGB panel validation using the source-backed GPIO map.

  What this example tests:
    - RGB timing and data bus bring-up;
    - backlight GPIO 2 at full ON;
    - RGB color order by full-screen red/green/blue/white/black;
    - orientation by border/corner markers;
    - simple stripe pattern for data-line sanity.

  Boundary:
    PASS here means the minimal Arduino_GFX RGB path renders correctly on
    the named physical specimen. It does not prove touch, SD or final BSP status.

  Dependency:
    Install Arduino_GFX_Library by moononournation from Arduino Library Manager.
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

static Arduino_ESP32RGBPanel *bus = nullptr;
static Arduino_RGB_Display *gfx = nullptr;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED   = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_BLUE  = 0x001F;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
static constexpr uint16_t COLOR_GRAY = 0x8410;

static void backlightOn() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
}

static void printPinMap() {
  Serial.println("[PIN MAP]");
  Serial.printf("LCD %dx%d\n", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("DE=%d VSYNC=%d HSYNC=%d PCLK=%d BL=%d\n", RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK, BACKLIGHT);
  Serial.printf("R0..R4=%d,%d,%d,%d,%d\n", RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4);
  Serial.printf("G0..G5=%d,%d,%d,%d,%d,%d\n", RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5);
  Serial.printf("B0..B4=%d,%d,%d,%d,%d\n", RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4);
}

static void initDisplay() {
  backlightOn();

  // Timing values are intentionally conservative/common for 800x480 RGB panels.
  // The factory demo already proves that this board can drive 800x480; this
  // example validates our own source-backed GPIO map and Arduino_GFX path.
  bus = new Arduino_ESP32RGBPanel(
    RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
    RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
    RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
    RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
    1 /* hsync polarity */, 40 /* hsync front porch */, 48 /* hsync pulse width */, 40 /* hsync back porch */,
    1 /* vsync polarity */, 13 /* vsync front porch */, 3 /* vsync pulse width */, 29 /* vsync back porch */,
    1 /* pclk active neg */, 16000000 /* prefer speed */
  );

  gfx = new Arduino_RGB_Display(
    LCD_WIDTH,
    LCD_HEIGHT,
    bus,
    0 /* rotation */,
    true /* auto_flush */
  );

  if (!gfx->begin()) {
    Serial.println("Display begin: FAIL");
    while (true) {
      delay(1000);
    }
  }

  gfx->fillScreen(COLOR_BLACK);
  Serial.println("Display begin: OK");
}

static void showColor(const char *name, uint16_t color, uint32_t holdMs = 900) {
  Serial.print("Screen: ");
  Serial.println(name);
  gfx->fillScreen(color);
  delay(holdMs);
}

static void drawOrientationFrame() {
  Serial.println("Screen: orientation frame");
  gfx->fillScreen(COLOR_BLACK);

  gfx->drawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE);
  gfx->drawRect(1, 1, LCD_WIDTH - 2, LCD_HEIGHT - 2, COLOR_WHITE);

  // Corner markers. With correct landscape orientation:
  // top-left red, top-right green, bottom-left blue, bottom-right white.
  gfx->fillRect(0, 0, 90, 70, COLOR_RED);
  gfx->fillRect(LCD_WIDTH - 90, 0, 90, 70, COLOR_GREEN);
  gfx->fillRect(0, LCD_HEIGHT - 70, 90, 70, COLOR_BLUE);
  gfx->fillRect(LCD_WIDTH - 90, LCD_HEIGHT - 70, 90, 70, COLOR_WHITE);

  gfx->setTextSize(3);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setCursor(120, 35);
  gfx->print("ESP32-8048S043 RGB TEST");

  gfx->setTextSize(2);
  gfx->setCursor(120, 95);
  gfx->print("Expected: landscape 800x480");
  gfx->setCursor(120, 125);
  gfx->print("TL red / TR green / BL blue / BR white");
  gfx->setCursor(120, 155);
  gfx->print("If colors/orientation look correct: DISPLAY LEAD PASS");

  delay(2500);
}

static void drawColorBars() {
  Serial.println("Screen: RGB color bars");
  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE,
    COLOR_BLACK, COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
  };
  const char *names[] = {
    "RED", "GREEN", "BLUE", "WHITE", "BLACK", "YELLOW", "CYAN", "MAGENTA"
  };
  const int count = sizeof(colors) / sizeof(colors[0]);
  const int barW = LCD_WIDTH / count;

  gfx->fillScreen(COLOR_BLACK);
  for (int i = 0; i < count; ++i) {
    int x = i * barW;
    int w = (i == count - 1) ? (LCD_WIDTH - x) : barW;
    gfx->fillRect(x, 0, w, LCD_HEIGHT, colors[i]);
  }

  gfx->setTextSize(2);
  for (int i = 0; i < count; ++i) {
    int x = i * barW + 8;
    uint16_t textColor = (colors[i] == COLOR_BLACK || colors[i] == COLOR_BLUE || colors[i] == COLOR_MAGENTA) ? COLOR_WHITE : COLOR_BLACK;
    gfx->setTextColor(textColor);
    gfx->setCursor(x, 20);
    gfx->print(names[i]);
  }

  delay(3000);
}

static void drawStripePattern() {
  Serial.println("Screen: stripe/data-line pattern");
  gfx->fillScreen(COLOR_BLACK);

  for (int y = 0; y < LCD_HEIGHT; ++y) {
    uint16_t color;
    switch ((y / 20) % 6) {
      case 0: color = COLOR_RED; break;
      case 1: color = COLOR_GREEN; break;
      case 2: color = COLOR_BLUE; break;
      case 3: color = COLOR_WHITE; break;
      case 4: color = COLOR_GRAY; break;
      default: color = COLOR_BLACK; break;
    }
    gfx->drawFastHLine(0, y, LCD_WIDTH, color);
  }

  for (int x = 0; x < LCD_WIDTH; x += 40) {
    uint16_t color = ((x / 40) % 2 == 0) ? COLOR_WHITE : COLOR_BLACK;
    gfx->drawFastVLine(x, 0, LCD_HEIGHT, color);
  }

  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setCursor(20, LCD_HEIGHT - 35);
  gfx->print("Stripe pattern: check stability, no tearing, no swapped colors");

  delay(3000);
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 02_DisplayRGBTest");
  Serial.println(" Minimal RGB panel + backlight validation");
  Serial.println("================================================================");
  printPinMap();
  initDisplay();

  Serial.println("Test sequence started.");
}

void loop() {
  showColor("RED", COLOR_RED);
  showColor("GREEN", COLOR_GREEN);
  showColor("BLUE", COLOR_BLUE);
  showColor("WHITE", COLOR_WHITE);
  showColor("BLACK", COLOR_BLACK);
  drawOrientationFrame();
  drawColorBars();
  drawStripePattern();

  Serial.println("02_DisplayRGBTest cycle complete. If all screens are correct: DISPLAY RGB TEST VISUAL PASS candidate.");
  delay(1000);
}
