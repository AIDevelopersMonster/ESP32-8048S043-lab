/*
  ESP32-8048S043 Lab / 04_BacklightTest

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Test order:
    01_BoardInfo       -> PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
    02_DisplayRGBTest  -> PASS: own Arduino_GFX RGB display path
    03_TouchGT911Test  -> PASS: own GT911 polling visual touch test
    04_BacklightTest   -> this test, dedicated backlight ON/OFF/PWM validation

  Purpose:
    Validate the ESP32-8048S043 display backlight control line with our own
    Arduino code after display and touch are already proven separately.

  What this example checks:
    - source-backed backlight pin GPIO2;
    - simple digital OFF / ON behavior;
    - repeated blink behavior;
    - PWM/duty steps using Arduino analogWrite();
    - whether brightness visibly changes without corrupting the RGB display;
    - serial evidence for every duty step.

  What this example does NOT check:
    - final production brightness curve;
    - gamma or perceived brightness linearity;
    - LVGL integration;
    - GT911 touch;
    - SD card;
    - final full BSP status.

  Arduino IDE settings used for Sample A:
    Board                                  : ESP32S3 Dev Module
    Flash Mode                             : QIO 80MHz
    Flash Size                             : 16MB (128Mb)
    Partition Scheme                       : 16M Flash (3MB APP/9.9MB FATFS)
    PSRAM                                  : OPI PSRAM
    Upload Mode                            : UART0 / Hardware CDC
    Upload Speed                           : 921600
    USB CDC On Boot                        : Disabled
    USB Mode                               : Hardware CDC and JTAG
    Serial Monitor                         : 115200 baud

  Dependency:
    Install Arduino_GFX_Library by moononournation from Arduino Library Manager.

  PASS boundary:
    PASS here means the physical Sample A display visibly turns off/on and, if
    supported by the board/backlight circuit, visibly changes brightness across
    PWM duty steps. A digital ON/OFF pass alone is useful evidence but does not
    prove final PWM brightness control.
*/

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

ESP32_8048S043 board;

static constexpr int LCD_W = LCD_WIDTH;
static constexpr int LCD_H = LCD_HEIGHT;
static constexpr int LCD_PCLK_HZ = 16000000;
static constexpr uint32_t STEP_HOLD_MS = 1400;
static constexpr uint32_t BLINK_HOLD_MS = 350;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_BLUE = 0x001F;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
static constexpr uint16_t COLOR_GRAY = 0x8410;
static constexpr uint16_t COLOR_DARKGREY = 0x7BEF;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
  RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
  RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
  RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
  1 /* hsync polarity */, 40 /* hsync front porch */, 48 /* hsync pulse width */, 40 /* hsync back porch */,
  1 /* vsync polarity */, 13 /* vsync front porch */, 3 /* vsync pulse width */, 29 /* vsync back porch */,
  1 /* pclk active neg */, LCD_PCLK_HZ /* prefer speed */
);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  LCD_W,
  LCD_H,
  rgbpanel,
  0 /* rotation */,
  true /* auto_flush */
);

static void setBacklightDigital(bool on) {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, on ? HIGH : LOW);
}

static void setBacklightDuty(uint8_t duty) {
  pinMode(BACKLIGHT, OUTPUT);

  if (duty == 0) {
    digitalWrite(BACKLIGHT, LOW);
  } else if (duty == 255) {
    digitalWrite(BACKLIGHT, HIGH);
  } else {
    // Arduino-ESP32 analogWrite() uses LEDC internally and is less sensitive to
    // core-version API differences than direct ledcSetup()/ledcAttachPin() code.
    analogWrite(BACKLIGHT, duty);
  }
}

static void drawStaticScreen() {
  gfx->fillScreen(COLOR_BLACK);

  const uint16_t header = rgb565(26, 33, 42);
  gfx->fillRect(0, 0, LCD_W, 52, header);
  gfx->setTextColor(COLOR_WHITE, header);
  gfx->setTextSize(2);
  gfx->setCursor(16, 16);
  gfx->print("ESP32-8048S043 04_BacklightTest");

  const int barY = 72;
  const int barH = 90;
  const int barW = LCD_W / 8;
  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_CYAN,
    COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE, COLOR_BLACK
  };
  const char *names[] = {"RED", "GREEN", "BLUE", "CYAN", "MAG", "YELLOW", "WHITE", "BLACK"};

  for (int i = 0; i < 8; ++i) {
    gfx->fillRect(i * barW, barY, barW, barH, colors[i]);
    gfx->drawRect(i * barW, barY, barW, barH, COLOR_DARKGREY);
    gfx->setTextSize(1);
    gfx->setTextColor(i == 6 ? COLOR_BLACK : COLOR_WHITE, colors[i]);
    gfx->setCursor((i * barW) + 10, barY + 12);
    gfx->print(names[i]);
  }

  const uint16_t grid = rgb565(35, 50, 65);
  for (int x = 0; x < LCD_W; x += 40) {
    gfx->drawFastVLine(x, 180, LCD_H - 180, grid);
  }
  for (int y = 180; y < LCD_H; y += 40) {
    gfx->drawFastHLine(0, y, LCD_W, grid);
  }

  gfx->drawRect(0, 0, LCD_W, LCD_H, COLOR_WHITE);
  gfx->fillRect(90, 210, 620, 115, rgb565(12, 18, 24));
  gfx->drawRect(90, 210, 620, 115, rgb565(100, 120, 140));
  gfx->setTextColor(COLOR_WHITE, rgb565(12, 18, 24));
  gfx->setTextSize(2);
  gfx->setCursor(115, 232);
  gfx->print("Backlight GPIO2 test");
  gfx->setTextSize(1);
  gfx->setCursor(115, 268);
  gfx->print("Watch visible OFF/ON, blink and PWM brightness steps.");
  gfx->setCursor(115, 292);
  gfx->print("Serial Monitor: 115200 baud records every stage.");
}

static void drawStage(const char *stage, int duty, const char *note) {
  gfx->fillRect(90, 350, 620, 82, COLOR_BLACK);
  gfx->drawRect(90, 350, 620, 82, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_GREEN, COLOR_BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(115, 368);
  gfx->printf("%s", stage);
  gfx->setTextColor(COLOR_YELLOW, COLOR_BLACK);
  gfx->setCursor(115, 398);
  gfx->printf("duty=%d / 255", duty);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(390, 404);
  gfx->print(note);
}

static void applyStage(const char *stage, uint8_t duty, const char *note, uint32_t holdMs) {
  Serial.printf("Backlight stage: %-18s duty=%3u note=%s\n", stage, duty, note);

  // Draw before changing the backlight so the message is already in framebuffer
  // when brightness changes.
  drawStage(stage, duty, note);
  delay(60);
  setBacklightDuty(duty);
  delay(holdMs);
}

static void runBacklightSequence() {
  Serial.println("[BACKLIGHT SEQUENCE]");
  Serial.printf("Backlight pin: GPIO%d\n", BACKLIGHT);
  Serial.println("Observe the physical screen. Some boards may support only ON/OFF, not visible PWM dimming.");

  applyStage("DIGITAL OFF", 0, "screen should go dark", STEP_HOLD_MS);
  applyStage("DIGITAL ON", 255, "full brightness", STEP_HOLD_MS);

  for (int i = 1; i <= 3; ++i) {
    char label[24];
    snprintf(label, sizeof(label), "BLINK %d OFF", i);
    applyStage(label, 0, "blink check", BLINK_HOLD_MS);
    snprintf(label, sizeof(label), "BLINK %d ON", i);
    applyStage(label, 255, "blink check", BLINK_HOLD_MS);
  }

  const uint8_t dutySteps[] = {0, 16, 32, 64, 96, 128, 160, 192, 224, 255};
  for (size_t i = 0; i < sizeof(dutySteps) / sizeof(dutySteps[0]); ++i) {
    char label[24];
    snprintf(label, sizeof(label), "PWM STEP %u", static_cast<unsigned>(i + 1));
    applyStage(label, dutySteps[i], "look for brightness change", STEP_HOLD_MS);
  }

  applyStage("FINAL ON", 255, "sequence complete", STEP_HOLD_MS);
  Serial.println("Backlight sequence complete. The sequence will repeat.");
}

void setup() {
  Serial.begin(115200);
  delay(800);
  board.begin();

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 04_BacklightTest");
  Serial.println(" Dedicated display backlight ON/OFF/PWM validation");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Target : BACKLIGHT GPIO2");
  Serial.println("----------------------------------------------------------------");

  setBacklightDigital(true);
  delay(200);

  Serial.println("gfx->begin() start");
  const bool gfxOk = gfx->begin();
  Serial.printf("gfx->begin(): %s\n", gfxOk ? "OK" : "FAIL");
  if (!gfxOk) {
    Serial.println("Display init failed; stopping before backlight test.");
    while (true) {
      delay(1000);
    }
  }

  drawStaticScreen();
  Serial.println("Static screen drawn. Starting backlight sequence.");
  Serial.println("----------------------------------------------------------------");
}

void loop() {
  runBacklightSequence();
  delay(1000);
}
