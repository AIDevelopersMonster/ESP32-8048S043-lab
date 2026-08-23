/*
  ESP32-8048S043 Lab / 03_TouchGT911Test

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Test order:
    01_BoardInfo       -> PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
    02_DisplayRGBTest  -> PASS: own Arduino_GFX RGB display path
    03_TouchGT911Test  -> this test, known-good-style GT911 display/touch test

  Purpose:
    Validate the ESP32-8048S043 GT911 capacitive touch path with a conservative
    visual Arduino_GFX test.

  Why this version exists:
    The first visual version was too different from working community examples.
    It used the wrong GT911 point-register start and an aggressive touch-trail
    update style. This version follows the known-good ESP32-8048S043C pattern:
      - static display first;
      - GT911 polling, no dependency on INT interrupt;
      - point data starts at 0x814F, not 0x8150;
      - x/y are little-endian after track id;
      - status register 0x814E is cleared after each read;
      - touch marker updates are throttled.

  What this example checks:
    - RGB display static screen through Arduino_GFX;
    - backlight GPIO2 full ON, same simple path as 02_DisplayRGBTest;
    - GT911 on I2C SDA=19 / SCL=20;
    - GT911 candidate addresses 0x5D and 0x14;
    - Product ID register 0x8140;
    - touch status register 0x814E;
    - touch point data from 0x814F;
    - raw coordinates and calibrated screen coordinates;
    - visible touch marker on the 800x480 display.

  What this example does NOT check:
    - final LVGL touch integration;
    - final production calibration;
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

  PASS boundary:
    PASS here means GT911 is detected and touching the screen prints changing
    coordinates and moves the visible marker on Sample A. It does not prove
    final LVGL coordinate mapping or complete BSP status.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

ESP32_8048S043 board;

static constexpr int LCD_W = LCD_WIDTH;
static constexpr int LCD_H = LCD_HEIGHT;
static constexpr int LCD_PCLK_HZ = 16000000;

static constexpr uint32_t TOUCH_POLL_INTERVAL_MS = 15;
static constexpr uint32_t TOUCH_LOG_INTERVAL_MS = 80;
static constexpr uint32_t TOUCH_DRAW_INTERVAL_MS = 120;
static constexpr bool TOUCH_DRAW_ENABLED = true;
static constexpr bool ANIMATION_ENABLED = false;

static constexpr uint32_t I2C_SPEED_HZ = 400000;
static constexpr uint16_t GT911_STATUS_REG = 0x814E;
static constexpr uint16_t GT911_POINT_REG = 0x814F;
static constexpr uint16_t GT911_PRODUCT_ID_REG = 0x8140;
static constexpr uint16_t GT911_FW_VERSION_REG = 0x8144;
static constexpr uint16_t GT911_X_RESOLUTION_REG = 0x8146;
static constexpr uint16_t GT911_Y_RESOLUTION_REG = 0x8148;

// Initial calibration seed taken from a same-class ESP32-8048S043C test.
// It maps compressed GT911 raw values to the 800x480 display for this orientation.
// If the marker is mirrored/rotated on Sample A, keep the raw serial evidence
// and adjust these constants in a later calibration pass.
static constexpr bool TOUCH_USE_CALIBRATION = true;
static constexpr float TOUCH_CAL_X_RX = 1.65867031f;
static constexpr float TOUCH_CAL_X_RY = -0.02261823f;
static constexpr float TOUCH_CAL_X_C = 2.12817001f;
static constexpr float TOUCH_CAL_Y_RX = 0.02082564f;
static constexpr float TOUCH_CAL_Y_RY = 1.79517055f;
static constexpr float TOUCH_CAL_Y_C = 10.62223816f;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_BLUE = 0x001F;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
static constexpr uint16_t COLOR_DARKGREY = 0x7BEF;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  RGB_DE, RGB_VSYNC, RGB_HSYNC, RGB_PCLK,
  RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4,
  RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
  RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
  0 /* hsync_polarity */, 8 /* hsync_front_porch */,
  4 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
  0 /* vsync_polarity */, 4 /* vsync_front_porch */,
  4 /* vsync_pulse_width */, 4 /* vsync_back_porch */,
  1 /* pclk_active_neg */, LCD_PCLK_HZ /* prefer_speed */, false /* useBigEndian */,
  0 /* de_idle_high */, 1 /* pclk_idle_high */
);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  LCD_W, LCD_H, rgbpanel, 0 /* rotation */, true /* auto_flush */
);

static uint8_t touchAddr = 0;
static int lastRawX = -1;
static int lastRawY = -1;
static int lastDrawX = -1;
static int lastDrawY = -1;
static uint32_t touchCount = 0;

static uint16_t le16(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static char printableOrDot(uint8_t value) {
  return (value >= 32 && value <= 126) ? static_cast<char>(value) : '.';
}

static void setBacklight(bool on) {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, on ? HIGH : LOW);
}

static bool i2cReadReg(uint8_t addr, uint16_t reg, uint8_t *buf, size_t len) {
  Wire.beginTransmission(addr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const int requested = static_cast<int>(len);
  const int got = Wire.requestFrom(static_cast<int>(addr), requested, static_cast<int>(true));
  if (got != requested) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    buf[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

static bool i2cWriteRegByte(uint8_t addr, uint16_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static void gt911ClearStatus() {
  if (touchAddr != 0) {
    i2cWriteRegByte(touchAddr, GT911_STATUS_REG, 0x00);
  }
}

static void gt911Reset() {
  // Do not depend on GPIO18 as an interrupt line. Use it only as a passive
  // pull-up during reset, matching a conservative polling setup.
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(120);
}

static String scanI2C() {
  String found;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      char tmp[8];
      snprintf(tmp, sizeof(tmp), "0x%02X ", addr);
      found += tmp;
    }
  }
  if (found.length() == 0) {
    found = "none";
  }
  return found;
}

static uint8_t findGT911() {
  const uint8_t candidates[] = {TOUCH_GT911_ADDR, TOUCH_GT911_ADDR_ALT};
  uint8_t id[4] = {};

  for (uint8_t addr : candidates) {
    if (i2cReadReg(addr, GT911_PRODUCT_ID_REG, id, sizeof(id))) {
      Serial.printf("GT911 at 0x%02X, product id raw: %02X %02X %02X %02X\n",
                    addr, id[0], id[1], id[2], id[3]);
      Serial.printf("GT911 product id text: %c%c%c%c\n",
                    printableOrDot(id[0]), printableOrDot(id[1]),
                    printableOrDot(id[2]), printableOrDot(id[3]));
      return addr;
    }
  }

  return 0;
}

static void printGt911Info() {
  if (touchAddr == 0) {
    Serial.println("GT911 info: no active address");
    return;
  }

  uint8_t fw[2] = {};
  if (i2cReadReg(touchAddr, GT911_FW_VERSION_REG, fw, sizeof(fw))) {
    Serial.printf("GT911 FW version: 0x%04X (%u)\n", le16(fw), le16(fw));
  } else {
    Serial.println("GT911 FW version: read failed");
  }

  uint8_t xres[2] = {};
  uint8_t yres[2] = {};
  if (i2cReadReg(touchAddr, GT911_X_RESOLUTION_REG, xres, sizeof(xres)) &&
      i2cReadReg(touchAddr, GT911_Y_RESOLUTION_REG, yres, sizeof(yres))) {
    Serial.printf("GT911 resolution registers: X=%u Y=%u\n", le16(xres), le16(yres));
  } else {
    Serial.println("GT911 resolution registers: read failed or unsupported");
  }
}

static int mapTouchX(int rawX, int rawY) {
  if (!TOUCH_USE_CALIBRATION) {
    return constrain(rawX, 0, LCD_W - 1);
  }
  const int mapped = static_cast<int>((TOUCH_CAL_X_RX * rawX) +
                                      (TOUCH_CAL_X_RY * rawY) +
                                      TOUCH_CAL_X_C + 0.5f);
  return constrain(mapped, 0, LCD_W - 1);
}

static int mapTouchY(int rawX, int rawY) {
  if (!TOUCH_USE_CALIBRATION) {
    return constrain(rawY, 0, LCD_H - 1);
  }
  const int mapped = static_cast<int>((TOUCH_CAL_Y_RX * rawX) +
                                      (TOUCH_CAL_Y_RY * rawY) +
                                      TOUCH_CAL_Y_C + 0.5f);
  return constrain(mapped, 0, LCD_H - 1);
}

static bool readTouch(int16_t &rawX, int16_t &rawY, uint8_t &trackId, uint16_t &size) {
  if (touchAddr == 0) {
    return false;
  }

  uint8_t status = 0;
  if (!i2cReadReg(touchAddr, GT911_STATUS_REG, &status, 1)) {
    return false;
  }

  if ((status & 0x80) == 0) {
    return false;
  }

  const uint8_t points = status & 0x0F;
  if (points == 0 || points > 5) {
    Serial.printf("Touch status unusual: 0x%02X points=%u\n", status, points);
    gt911ClearStatus();
    return false;
  }

  uint8_t data[8] = {};
  const bool ok = i2cReadReg(touchAddr, GT911_POINT_REG, data, sizeof(data));
  gt911ClearStatus();
  if (!ok) {
    return false;
  }

  // GT911 point data from 0x814F:
  // data[0] = track id, data[1..2] = X little-endian,
  // data[3..4] = Y little-endian, data[5..6] = touch size.
  trackId = data[0];
  rawX = static_cast<int16_t>(le16(&data[1]));
  rawY = static_cast<int16_t>(le16(&data[3]));
  size = le16(&data[5]);

  Serial.printf("Touch raw packet: status=0x%02X points=%u data=%02X %02X %02X %02X %02X %02X %02X %02X\n",
                status, points,
                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
  return true;
}

static void drawCornerTarget(int x, int y, const char *label) {
  gfx->drawCircle(x, y, 22, COLOR_WHITE);
  gfx->drawCircle(x, y, 10, COLOR_WHITE);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(1);

  int textX = x < 100 ? x + 28 : x - 88;
  int textY = y < 100 ? y + 12 : y - 22;
  gfx->setCursor(textX, textY);
  gfx->print(label);
}

static void drawStaticScreen() {
  gfx->fillScreen(COLOR_BLACK);

  const uint16_t header = rgb565(26, 33, 42);
  gfx->fillRect(0, 0, LCD_W, 46, header);
  gfx->setTextColor(COLOR_WHITE, header);
  gfx->setTextSize(2);
  gfx->setCursor(14, 12);
  gfx->print("ESP32-8048S043 GT911 Display + Touch Test");

  const int barY = 64;
  const int barH = 76;
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
    gfx->setCursor(i * barW + 10, barY + 10);
    gfx->print(names[i]);
  }

  const uint16_t grid = rgb565(35, 50, 65);
  for (int x = 0; x < LCD_W; x += 40) {
    gfx->drawFastVLine(x, 160, LCD_H - 160, grid);
  }
  for (int y = 160; y < LCD_H; y += 40) {
    gfx->drawFastHLine(0, y, LCD_W, grid);
  }

  gfx->drawRect(0, 0, LCD_W, LCD_H, COLOR_WHITE);
  drawCornerTarget(34, 184, "top left");
  drawCornerTarget(LCD_W - 35, 184, "top right");
  drawCornerTarget(34, LCD_H - 35, "bottom left");
  drawCornerTarget(LCD_W - 35, LCD_H - 35, "bottom right");

  gfx->fillRect(170, 178, 460, 84, rgb565(12, 18, 24));
  gfx->drawRect(170, 178, 460, 84, rgb565(100, 120, 140));
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_WHITE, rgb565(12, 18, 24));
  gfx->setCursor(190, 196);
  gfx->print("Touch the display");
  gfx->setTextSize(1);
  gfx->setCursor(190, 232);
  gfx->print("Polling GT911: 0x814E status, 0x814F point data.");
}

static void drawStatus(const String &devices) {
  gfx->fillRect(0, 144, LCD_W, 28, rgb565(18, 24, 31));
  gfx->setTextColor(COLOR_WHITE, rgb565(18, 24, 31));
  gfx->setTextSize(1);
  gfx->setCursor(12, 153);
  gfx->printf("Display: RGB 800x480, PCLK %dMHz, BL GPIO2", LCD_PCLK_HZ / 1000000);

  gfx->setCursor(350, 153);
  if (touchAddr != 0) {
    gfx->printf("GT911: 0x%02X, SDA19/SCL20/RST38", touchAddr);
  } else {
    gfx->print("GT911 not found");
  }

  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(190, 340);
  gfx->print("I2C devices: ");
  gfx->print(devices);

  gfx->fillRect(170, 390, 460, 18, COLOR_BLACK);
  gfx->drawRect(170, 390, 460, 18, COLOR_DARKGREY);
  gfx->setCursor(190, 395);
  gfx->print(TOUCH_DRAW_ENABLED ? "Touch drawing ON, Serial ON" : "Touch drawing OFF, Serial ON");
}

static void drawTouchPoint(int screenX, int screenY, int rawX, int rawY, uint32_t count) {
  if (lastDrawX >= 0 && lastDrawY >= 0) {
    gfx->drawCircle(lastDrawX, lastDrawY, 18, rgb565(80, 80, 80));
  }

  gfx->drawCircle(screenX, screenY, 18, COLOR_RED);
  gfx->drawFastHLine(screenX - 24, screenY, 49, COLOR_RED);
  gfx->drawFastVLine(screenX, screenY - 24, 49, COLOR_RED);

  gfx->fillRect(170, 280, 460, 58, COLOR_BLACK);
  gfx->drawRect(170, 280, 460, 58, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_GREEN, COLOR_BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(190, 292);
  gfx->printf("Touch #%lu  x=%d  y=%d", static_cast<unsigned long>(count), screenX, screenY);
  gfx->setTextSize(1);
  gfx->setCursor(190, 318);
  gfx->printf("raw_x=%d raw_y=%d", rawX, rawY);

  lastDrawX = screenX;
  lastDrawY = screenY;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  board.begin();

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 03_TouchGT911Test");
  Serial.println(" Known-good-style Arduino_GFX + GT911 polling test");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Point register: 0x814F, status register: 0x814E");
  Serial.println("----------------------------------------------------------------");

  pinMode(BACKLIGHT, OUTPUT);
  setBacklight(true);
  delay(300);

  Serial.println("gfx->begin() start");
  bool gfxOk = gfx->begin();
  Serial.printf("gfx->begin(): %s\n", gfxOk ? "OK" : "FAIL");
  if (!gfxOk) {
    Serial.println("Display init failed; stopping before touch test.");
    while (true) {
      delay(1000);
    }
  }

  drawStaticScreen();

  Serial.printf("Wire.begin(SDA=%d, SCL=%d, speed=%lu)\n",
                TOUCH_SDA, TOUCH_SCL, static_cast<unsigned long>(I2C_SPEED_HZ));
  Wire.begin(TOUCH_SDA, TOUCH_SCL, I2C_SPEED_HZ);

  Serial.println("GT911 reset: RST38 toggle, INT18 passive pull-up, polling mode");
  gt911Reset();
  touchAddr = findGT911();

  Serial.print("I2C scan: ");
  String devices = scanI2C();
  Serial.println(devices);

  if (touchAddr == 0) {
    Serial.println("GT911 TEST RESULT: NOT DETECTED at 0x5D or 0x14");
  } else {
    Serial.printf("Active GT911 address: 0x%02X\n", touchAddr);
    printGt911Info();
  }

  drawStatus(devices);

  Serial.println("----------------------------------------------------------------");
  Serial.println("Touch the panel. PASS candidate when Serial x/y changes and red marker follows touch.");
  Serial.println("================================================================");
}

void loop() {
  static uint32_t lastTouchPoll = 0;
  static uint32_t lastTouchLog = 0;
  static uint32_t lastTouchDraw = 0;
  static uint32_t lastAnim = 0;
  static int animX = 0;

  const uint32_t now = millis();

  if (now - lastTouchPoll >= TOUCH_POLL_INTERVAL_MS) {
    lastTouchPoll = now;

    int16_t rawX = 0;
    int16_t rawY = 0;
    uint8_t trackId = 0;
    uint16_t touchSize = 0;

    if (readTouch(rawX, rawY, trackId, touchSize)) {
      const int screenX = mapTouchX(rawX, rawY);
      const int screenY = mapTouchY(rawX, rawY);
      const int dx = abs(static_cast<int>(rawX) - lastRawX);
      const int dy = abs(static_cast<int>(rawY) - lastRawY);
      const bool movedEnough = lastRawX < 0 || dx > 2 || dy > 2;

      if ((now - lastTouchLog >= TOUCH_LOG_INTERVAL_MS) && movedEnough) {
        lastTouchLog = now;
        ++touchCount;
        Serial.printf("Touch #%lu: track=%u raw_x=%d raw_y=%d screen_x=%d screen_y=%d size=%u\n",
                      static_cast<unsigned long>(touchCount),
                      trackId,
                      rawX,
                      rawY,
                      screenX,
                      screenY,
                      touchSize);
      }

      if (TOUCH_DRAW_ENABLED &&
          (now - lastTouchDraw >= TOUCH_DRAW_INTERVAL_MS) &&
          (lastDrawX < 0 || abs(screenX - lastDrawX) > 8 || abs(screenY - lastDrawY) > 8)) {
        lastTouchDraw = now;
        drawTouchPoint(screenX, screenY, rawX, rawY, touchCount);
      }

      lastRawX = rawX;
      lastRawY = rawY;
    }
  }

  if (ANIMATION_ENABLED && (now - lastAnim > 120)) {
    lastAnim = now;
    gfx->fillRect(170, 360, 460, 18, COLOR_BLACK);
    gfx->drawRect(170, 360, 460, 18, COLOR_DARKGREY);
    gfx->fillRect(171 + animX, 361, 28, 16, COLOR_CYAN);
    animX += 5;
    if (animX > 430) {
      animX = 0;
    }
  }

  delay(5);
}
