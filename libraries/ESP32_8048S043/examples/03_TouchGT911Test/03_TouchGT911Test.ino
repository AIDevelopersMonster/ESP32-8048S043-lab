/*
  ESP32-8048S043 Lab / 03_TouchGT911Test

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Test order:
    01_BoardInfo       -> PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
    02_DisplayRGBTest  -> PASS: own Arduino_GFX RGB display path
    03_TouchGT911Test  -> this test, first own visual GT911 touch validation

  Purpose:
    First visual GT911 capacitive touch validation from our ESP32_8048S043
    Arduino library.

  What this example checks:
    - RGB display still initializes through Arduino_GFX;
    - source-backed I2C pins SDA=19 and SCL=20;
    - GT911 candidate addresses 0x5D and 0x14;
    - optional GT911 reset/address strap using RST=38 and INT=18;
    - GT911 Product ID register at 0x8140;
    - touch status register at 0x814E;
    - raw touch point coordinates while touching the screen;
    - visual touch trail on the 800x480 display.

  What this example does NOT check:
    - final LVGL touch integration;
    - final coordinate calibration;
    - gestures;
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

  Expected result:
    - the display shows a touch test screen;
    - I2C scan finds GT911 at 0x5D or 0x14;
    - Product ID is readable;
    - touching the screen draws dots/trails on the display;
    - Serial Monitor prints sane x/y coordinates in the 800x480 range;
    - movement on the panel changes x/y and the visual trail.

  Note about coordinate byte order:
    Observed Sample A raw GT911 point bytes use high byte first for x/y:
      raw=.. 00 9B 00 39 .. -> x=155, y=57
    Earlier serial-only code treated those bytes as little-endian and printed
    impossible values such as x=39680/y=14592. This visual version uses the
    observed high-byte-first point decoding.

  PASS boundary:
    PASS here means our own GT911/I2C test detects the controller and shows
    changing touch coordinates visually on Sample A. It does not prove final
    LVGL coordinate mapping or full touch UI integration.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

ESP32_8048S043 board;

static Arduino_ESP32RGBPanel *bus = nullptr;
static Arduino_RGB_Display *gfx = nullptr;

static constexpr uint32_t I2C_SPEED_HZ = 400000;
static constexpr uint16_t GT911_REG_PRODUCT_ID = 0x8140;
static constexpr uint16_t GT911_REG_FW_VERSION = 0x8144;
static constexpr uint16_t GT911_REG_X_RESOLUTION = 0x8146;
static constexpr uint16_t GT911_REG_Y_RESOLUTION = 0x8148;
static constexpr uint16_t GT911_REG_STATUS = 0x814E;
static constexpr uint16_t GT911_REG_POINT1 = 0x8150;
static constexpr uint8_t MAX_TOUCH_POINTS = 5;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_BLUE = 0x001F;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
static constexpr uint16_t COLOR_GRAY = 0x8410;
static constexpr uint16_t COLOR_DARK = 0x2104;

static uint8_t activeAddress = 0;
static uint32_t lastIdlePrint = 0;
static uint32_t lastScreenIdle = 0;
static int lastX = -1;
static int lastY = -1;
static uint32_t touchCounter = 0;

static bool i2cPresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

static bool gt911Read(uint8_t address, uint16_t reg, uint8_t *buffer, size_t length) {
  Wire.beginTransmission(address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const int requested = static_cast<int>(length);
  const int received = Wire.requestFrom(static_cast<int>(address), requested);
  if (received != requested) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t i = 0; i < length; ++i) {
    buffer[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

static bool gt911WriteByte(uint8_t address, uint16_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static uint16_t le16(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static uint16_t be16(const uint8_t *data) {
  return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
}

static char printableOrDot(uint8_t value) {
  return (value >= 32 && value <= 126) ? static_cast<char>(value) : '.';
}

static void backlightOn() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);
}

static void initDisplay() {
  backlightOn();

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

  Serial.println("Display begin: OK");
}

static void screenText(int x, int y, uint16_t color, uint16_t bg, uint8_t size, const char *text) {
  gfx->setTextSize(size);
  gfx->setTextColor(color, bg);
  gfx->setCursor(x, y);
  gfx->print(text);
}

static void drawTarget(int x, int y, const char *label) {
  gfx->drawCircle(x, y, 18, COLOR_CYAN);
  gfx->drawCircle(x, y, 19, COLOR_CYAN);
  gfx->drawFastHLine(x - 26, y, 52, COLOR_CYAN);
  gfx->drawFastVLine(x, y - 26, 52, COLOR_CYAN);
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setCursor(x - 12, y + 24);
  gfx->print(label);
}

static void drawTouchCanvas(const char *stateLine) {
  gfx->fillScreen(COLOR_BLACK);
  gfx->drawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_WHITE);
  gfx->drawRect(1, 1, LCD_WIDTH - 2, LCD_HEIGHT - 2, COLOR_WHITE);

  screenText(20, 18, COLOR_WHITE, COLOR_BLACK, 3, "03 Touch GT911 Visual Test");
  screenText(20, 58, COLOR_CYAN, COLOR_BLACK, 2, "Touch the panel: dots/trails should follow your finger");
  screenText(20, 88, COLOR_GRAY, COLOR_BLACK, 2, stateLine);

  drawTarget(45, 145, "TL");
  drawTarget(LCD_WIDTH - 45, 145, "TR");
  drawTarget(45, LCD_HEIGHT - 95, "BL");
  drawTarget(LCD_WIDTH - 45, LCD_HEIGHT - 95, "BR");
  drawTarget(LCD_WIDTH / 2, LCD_HEIGHT / 2, "CENTER");

  gfx->drawFastHLine(0, LCD_HEIGHT - 58, LCD_WIDTH, COLOR_WHITE);
  screenText(20, LCD_HEIGHT - 43, COLOR_YELLOW, COLOR_BLACK, 2, "Waiting for touch...");
}

static void drawStatusLine(const char *line, uint16_t color = COLOR_YELLOW) {
  gfx->fillRect(2, LCD_HEIGHT - 56, LCD_WIDTH - 4, 54, COLOR_BLACK);
  gfx->drawFastHLine(0, LCD_HEIGHT - 58, LCD_WIDTH, COLOR_WHITE);
  screenText(20, LCD_HEIGHT - 43, color, COLOR_BLACK, 2, line);
}

static int clampX(uint16_t x) {
  if (x >= LCD_WIDTH) {
    return LCD_WIDTH - 1;
  }
  return static_cast<int>(x);
}

static int clampY(uint16_t y) {
  if (y >= LCD_HEIGHT) {
    return LCD_HEIGHT - 1;
  }
  return static_cast<int>(y);
}

static void drawTouchPoint(uint16_t rawX, uint16_t rawY, uint8_t id, uint8_t pointIndex) {
  const int x = clampX(rawX);
  const int y = clampY(rawY);

  if (lastX >= 0 && lastY >= 0) {
    gfx->drawLine(lastX, lastY, x, y, COLOR_GREEN);
  }

  const uint16_t color = (pointIndex == 0) ? COLOR_YELLOW : COLOR_MAGENTA;
  gfx->fillCircle(x, y, 7, color);
  gfx->drawCircle(x, y, 10, COLOR_WHITE);

  lastX = x;
  lastY = y;
  ++touchCounter;

  char line[96];
  snprintf(line, sizeof(line), "touch#%lu id=%u raw=(%u,%u) screen=(%d,%d)",
           static_cast<unsigned long>(touchCounter), id, rawX, rawY, x, y);
  drawStatusLine(line, COLOR_YELLOW);
}

static void printI2CScan() {
  Serial.println("[I2C SCAN]");
  int found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    if (i2cPresent(address)) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      ++found;
    }
  }
  if (found == 0) {
    Serial.println("No I2C devices found.");
  }
  Serial.printf("I2C scan complete, devices=%d\n", found);
}

static void gt911ResetForAddress(uint8_t targetAddress) {
  Serial.printf("GT911 reset/address strap attempt for 0x%02X\n", targetAddress);

  pinMode(TOUCH_RST, OUTPUT);
  pinMode(TOUCH_INT, OUTPUT);

  digitalWrite(TOUCH_RST, LOW);
  delay(10);

  // Common GT911 strap convention:
  // INT high during reset release selects 0x5D, INT low selects 0x14.
  digitalWrite(TOUCH_INT, targetAddress == TOUCH_GT911_ADDR ? HIGH : LOW);
  delay(5);

  digitalWrite(TOUCH_RST, HIGH);
  delay(60);

  pinMode(TOUCH_INT, INPUT);
  delay(80);
}

static uint8_t detectGt911() {
  Serial.println("[GT911 PROBE]");
  Serial.printf("Candidate primary address  : 0x%02X\n", TOUCH_GT911_ADDR);
  Serial.printf("Candidate alternate address: 0x%02X\n", TOUCH_GT911_ADDR_ALT);
  Serial.printf("Pins: SDA=%d SCL=%d RST=%d INT=%d\n", TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);

  if (i2cPresent(TOUCH_GT911_ADDR)) {
    Serial.printf("GT911 candidate present without reset at 0x%02X\n", TOUCH_GT911_ADDR);
    return TOUCH_GT911_ADDR;
  }

  if (i2cPresent(TOUCH_GT911_ADDR_ALT)) {
    Serial.printf("GT911 candidate present without reset at 0x%02X\n", TOUCH_GT911_ADDR_ALT);
    return TOUCH_GT911_ADDR_ALT;
  }

  Serial.println("GT911 not found before reset. Trying address strap reset sequence.");

  gt911ResetForAddress(TOUCH_GT911_ADDR);
  if (i2cPresent(TOUCH_GT911_ADDR)) {
    Serial.printf("GT911 candidate present after reset at 0x%02X\n", TOUCH_GT911_ADDR);
    return TOUCH_GT911_ADDR;
  }

  gt911ResetForAddress(TOUCH_GT911_ADDR_ALT);
  if (i2cPresent(TOUCH_GT911_ADDR_ALT)) {
    Serial.printf("GT911 candidate present after reset at 0x%02X\n", TOUCH_GT911_ADDR_ALT);
    return TOUCH_GT911_ADDR_ALT;
  }

  Serial.println("GT911 candidate not detected at 0x5D or 0x14.");
  return 0;
}

static void printGt911Info(uint8_t address) {
  Serial.println("[GT911 INFO]");

  uint8_t product[4] = {0};
  if (gt911Read(address, GT911_REG_PRODUCT_ID, product, sizeof(product))) {
    Serial.printf("Product ID raw : %02X %02X %02X %02X\n", product[0], product[1], product[2], product[3]);
    Serial.printf("Product ID text: %c%c%c%c\n",
                  printableOrDot(product[0]), printableOrDot(product[1]),
                  printableOrDot(product[2]), printableOrDot(product[3]));
  } else {
    Serial.println("Product ID read: FAIL");
  }

  uint8_t fw[2] = {0};
  if (gt911Read(address, GT911_REG_FW_VERSION, fw, sizeof(fw))) {
    Serial.printf("FW version LE  : 0x%04X (%u)\n", le16(fw), le16(fw));
    Serial.printf("FW version BE  : 0x%04X (%u)\n", be16(fw), be16(fw));
  } else {
    Serial.println("FW version read: FAIL");
  }

  uint8_t xres[2] = {0};
  uint8_t yres[2] = {0};
  bool xOk = gt911Read(address, GT911_REG_X_RESOLUTION, xres, sizeof(xres));
  bool yOk = gt911Read(address, GT911_REG_Y_RESOLUTION, yres, sizeof(yres));
  if (xOk && yOk) {
    Serial.printf("Touch resolution LE: X=%u Y=%u\n", le16(xres), le16(yres));
    Serial.printf("Touch resolution BE: X=%u Y=%u\n", be16(xres), be16(yres));
  } else {
    Serial.println("Touch resolution read: FAIL or unsupported");
  }

  Serial.println("Touch polling started. Touch the screen and watch serial + visual coordinates.");
}

static void pollTouch(uint8_t address) {
  uint8_t status = 0;
  if (!gt911Read(address, GT911_REG_STATUS, &status, 1)) {
    if (millis() - lastIdlePrint > 3000) {
      lastIdlePrint = millis();
      Serial.println("Touch status read: FAIL");
      drawStatusLine("Touch status read: FAIL", COLOR_RED);
    }
    return;
  }

  const bool dataReady = (status & 0x80) != 0;
  const uint8_t pointCount = status & 0x0F;

  if (!dataReady) {
    lastX = -1;
    lastY = -1;
    if (millis() - lastIdlePrint > 3000) {
      lastIdlePrint = millis();
      Serial.println("Touch idle: no new point data");
    }
    if (millis() - lastScreenIdle > 3000) {
      lastScreenIdle = millis();
      drawStatusLine("Touch idle: draw on the panel", COLOR_GRAY);
    }
    return;
  }

  if (pointCount == 0 || pointCount > MAX_TOUCH_POINTS) {
    Serial.printf("Touch status: dataReady=1 unusual pointCount=%u status=0x%02X\n", pointCount, status);
    gt911WriteByte(address, GT911_REG_STATUS, 0x00);
    return;
  }

  Serial.printf("Touch points=%u status=0x%02X\n", pointCount, status);

  for (uint8_t i = 0; i < pointCount; ++i) {
    uint8_t point[8] = {0};
    const uint16_t reg = GT911_REG_POINT1 + static_cast<uint16_t>(i) * 8;
    if (!gt911Read(address, reg, point, sizeof(point))) {
      Serial.printf("  P%u read: FAIL\n", i + 1);
      continue;
    }

    const uint8_t id = point[0];
    const uint16_t x = be16(&point[1]);
    const uint16_t y = be16(&point[3]);
    const uint16_t size = be16(&point[5]);

    Serial.printf("  P%u id=%u x=%u y=%u size=%u raw=%02X %02X %02X %02X %02X %02X %02X %02X\n",
                  i + 1,
                  id,
                  x,
                  y,
                  size,
                  point[0], point[1], point[2], point[3], point[4], point[5], point[6], point[7]);

    drawTouchPoint(x, y, id, i);
  }

  gt911WriteByte(address, GT911_REG_STATUS, 0x00);
}

void setup() {
  Serial.begin(115200);
  delay(800);
  board.begin();

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 03_TouchGT911Test");
  Serial.println(" Visual GT911 touch validation");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Purpose: validate own visual GT911 touch path after 01/02 PASS");
  Serial.println("----------------------------------------------------------------");

  initDisplay();
  drawTouchCanvas("Display OK. Starting I2C/GT911 probe...");

  Wire.begin(TOUCH_SDA, TOUCH_SCL, I2C_SPEED_HZ);
  delay(50);

  Serial.printf("Wire.begin(SDA=%d, SCL=%d, speed=%lu)\n",
                TOUCH_SDA,
                TOUCH_SCL,
                static_cast<unsigned long>(I2C_SPEED_HZ));

  printI2CScan();
  activeAddress = detectGt911();

  if (activeAddress == 0) {
    Serial.println("----------------------------------------------------------------");
    Serial.println("GT911 TEST RESULT: NOT DETECTED");
    Serial.println("Check pins, reset line, address strap, panel cable and power.");
    Serial.println("================================================================");
    drawTouchCanvas("GT911 NOT DETECTED at 0x5D/0x14");
    drawStatusLine("GT911 NOT DETECTED - check cable/pins/reset", COLOR_RED);
    return;
  }

  Serial.println("----------------------------------------------------------------");
  Serial.printf("Active GT911 address: 0x%02X\n", activeAddress);
  printGt911Info(activeAddress);

  char stateLine[96];
  snprintf(stateLine, sizeof(stateLine), "GT911 address 0x%02X active. Touch the screen.", activeAddress);
  drawTouchCanvas(stateLine);

  Serial.println("----------------------------------------------------------------");
  Serial.println("PASS candidate if touches draw visible points/trails and serial x/y changes in 800x480 range.");
  Serial.println("================================================================");
}

void loop() {
  if (activeAddress == 0) {
    delay(1000);
    return;
  }

  pollTouch(activeAddress);
  delay(20);
}
