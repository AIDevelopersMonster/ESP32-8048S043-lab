/*
  ESP32-8048S043 Lab / 05_TestConsole

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Test order:
    01_BoardInfo       -> PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
    02_DisplayRGBTest  -> PASS: own Arduino_GFX RGB display path
    03_TouchGT911Test  -> PASS: own GT911 polling visual touch test
    04_BacklightTest   -> PASS candidate: GPIO2 backlight ON/OFF/PWM behavior
    05_TestConsole     -> this test, first combined diagnostic console

  Purpose:
    Run the already validated low-level blocks together in one simple local
    diagnostic console before moving to LVGL examples.

  What this example checks together:
    - RGB display static UI through Arduino_GFX;
    - GT911 polling touch at 0x5D or 0x14;
    - raw and mapped touch coordinate reporting;
    - visible touch marker;
    - backlight GPIO2 digital ON/OFF control from a touch button;
    - basic ESP32-S3/flash/PSRAM/heap information;
    - serial diagnostics while the UI remains alive.

  Arduino IDE note:
    This file intentionally avoids user-defined struct types in function
    signatures. Arduino IDE generates hidden prototypes for .ino files and can
    place them before local struct declarations. Keeping signatures primitive
    avoids the 'TouchSample was not declared' / 'Button does not name a type'
    preprocessing failure.

  PASS boundary:
    PASS here means display, GT911 touch polling, visible marker, serial report
    and simple backlight toggle all work together on a named specimen without
    brownout or reset during observation.
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

static constexpr uint32_t I2C_SPEED_HZ = 400000;
static constexpr uint32_t TOUCH_POLL_INTERVAL_MS = 15;
static constexpr uint32_t TOUCH_LOG_INTERVAL_MS = 100;
static constexpr uint32_t TOUCH_DRAW_INTERVAL_MS = 80;
static constexpr uint32_t UI_STATUS_INTERVAL_MS = 1000;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 350;

static constexpr uint16_t GT911_STATUS_REG = 0x814E;
static constexpr uint16_t GT911_POINT_REG = 0x814F;
static constexpr uint16_t GT911_PRODUCT_ID_REG = 0x8140;
static constexpr uint16_t GT911_FW_VERSION_REG = 0x8144;
static constexpr uint16_t GT911_X_RESOLUTION_REG = 0x8146;
static constexpr uint16_t GT911_Y_RESOLUTION_REG = 0x8148;

// Same initial calibration seed as 03_TouchGT911Test.
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
static constexpr uint16_t COLOR_GRAY = 0x8410;
static constexpr uint16_t COLOR_DARKGREY = 0x7BEF;

static constexpr int BTN_BACKLIGHT_X = 40;
static constexpr int BTN_BACKLIGHT_Y = 390;
static constexpr int BTN_BACKLIGHT_W = 190;
static constexpr int BTN_BACKLIGHT_H = 54;

static constexpr int BTN_CLEAR_X = 260;
static constexpr int BTN_CLEAR_Y = 390;
static constexpr int BTN_CLEAR_W = 190;
static constexpr int BTN_CLEAR_H = 54;

static constexpr int BTN_REPORT_X = 480;
static constexpr int BTN_REPORT_Y = 390;
static constexpr int BTN_REPORT_W = 190;
static constexpr int BTN_REPORT_H = 54;

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
  LCD_W,
  LCD_H,
  rgbpanel,
  0 /* rotation */,
  true /* auto_flush */
);

static uint8_t touchAddr = 0;
static bool backlightOn = true;
static bool lastTouchDown = false;
static uint32_t touchEvents = 0;
static uint32_t lastTouchPoll = 0;
static uint32_t lastTouchLog = 0;
static uint32_t lastTouchDraw = 0;
static uint32_t lastStatusUpdate = 0;
static uint32_t lastButtonMs = 0;
static int lastMarkerX = -1;
static int lastMarkerY = -1;
static char lastAction[80] = "Console started";

static uint8_t curStatus = 0;
static uint8_t curPoints = 0;
static uint8_t curTrackId = 0;
static uint16_t curRawX = 0;
static uint16_t curRawY = 0;
static uint16_t curSize = 0;
static int curScreenX = 0;
static int curScreenY = 0;

static uint16_t le16(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

static char printableOrDot(uint8_t value) {
  return (value >= 32 && value <= 126) ? static_cast<char>(value) : '.';
}

static void setBacklight(bool on) {
  backlightOn = on;
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
  return found.length() ? found : String("none");
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
  uint8_t xres[2] = {};
  uint8_t yres[2] = {};

  if (i2cReadReg(touchAddr, GT911_FW_VERSION_REG, fw, sizeof(fw))) {
    Serial.printf("GT911 FW version: 0x%04X (%u)\n", le16(fw), le16(fw));
  }
  if (i2cReadReg(touchAddr, GT911_X_RESOLUTION_REG, xres, sizeof(xres)) &&
      i2cReadReg(touchAddr, GT911_Y_RESOLUTION_REG, yres, sizeof(yres))) {
    Serial.printf("GT911 resolution registers: X=%u Y=%u\n", le16(xres), le16(yres));
  }
}

static int mapTouchX(int rawX, int rawY) {
  const int mapped = static_cast<int>((TOUCH_CAL_X_RX * rawX) +
                                      (TOUCH_CAL_X_RY * rawY) +
                                      TOUCH_CAL_X_C + 0.5f);
  return constrain(mapped, 0, LCD_W - 1);
}

static int mapTouchY(int rawX, int rawY) {
  const int mapped = static_cast<int>((TOUCH_CAL_Y_RX * rawX) +
                                      (TOUCH_CAL_Y_RY * rawY) +
                                      TOUCH_CAL_Y_C + 0.5f);
  return constrain(mapped, 0, LCD_H - 1);
}

static bool readTouchCurrent() {
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
  if (points == 0) {
    gt911ClearStatus();
    return false;
  }
  if (points > 5) {
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

  curStatus = status;
  curPoints = points;
  curTrackId = data[0];
  curRawX = le16(&data[1]);
  curRawY = le16(&data[3]);
  curSize = le16(&data[5]);
  curScreenX = mapTouchX(curRawX, curRawY);
  curScreenY = mapTouchY(curRawX, curRawY);
  return true;
}

static bool insideRect(int bx, int by, int bw, int bh, int x, int y) {
  return (x >= bx) && (x < bx + bw) && (y >= by) && (y < by + bh);
}

static void drawButtonRect(int x, int y, int w, int h, const char *label, uint16_t color, const char *value) {
  const uint16_t bg = rgb565(20, 26, 34);
  gfx->fillRect(x, y, w, h, bg);
  gfx->drawRect(x, y, w, h, color);
  gfx->drawRect(x + 1, y + 1, w - 2, h - 2, color);
  gfx->setTextSize(2);
  gfx->setTextColor(color, bg);
  gfx->setCursor(x + 14, y + 10);
  gfx->print(label);
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE, bg);
  gfx->setCursor(x + 14, y + 36);
  gfx->print(value);
}

static void drawButtons() {
  drawButtonRect(BTN_BACKLIGHT_X, BTN_BACKLIGHT_Y, BTN_BACKLIGHT_W, BTN_BACKLIGHT_H,
                 "BACKLIGHT", backlightOn ? COLOR_GREEN : COLOR_RED, backlightOn ? "ON" : "OFF");
  drawButtonRect(BTN_CLEAR_X, BTN_CLEAR_Y, BTN_CLEAR_W, BTN_CLEAR_H,
                 "CLEAR", COLOR_YELLOW, "reset touch count");
  drawButtonRect(BTN_REPORT_X, BTN_REPORT_Y, BTN_REPORT_W, BTN_REPORT_H,
                 "REPORT", COLOR_CYAN, "print serial report");
}

static void drawStaticConsole() {
  gfx->fillScreen(COLOR_BLACK);

  const uint16_t header = rgb565(26, 33, 42);
  gfx->fillRect(0, 0, LCD_W, 52, header);
  gfx->setTextColor(COLOR_WHITE, header);
  gfx->setTextSize(2);
  gfx->setCursor(16, 16);
  gfx->print("ESP32-8048S043 05_TestConsole");

  gfx->setTextSize(1);
  gfx->setCursor(560, 20);
  gfx->print("RGB + GT911 + BL");

  const uint16_t panel = rgb565(12, 18, 24);
  gfx->fillRect(20, 72, 360, 160, panel);
  gfx->drawRect(20, 72, 360, 160, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_CYAN, panel);
  gfx->setTextSize(2);
  gfx->setCursor(40, 92);
  gfx->print("System");

  gfx->setTextColor(COLOR_WHITE, panel);
  gfx->setTextSize(1);
  gfx->setCursor(40, 128);
  gfx->printf("Chip: %s rev %d, CPU %u MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
  gfx->setCursor(40, 150);
  gfx->printf("Flash: %lu bytes", static_cast<unsigned long>(ESP.getFlashChipSize()));
  gfx->setCursor(40, 172);
  gfx->printf("PSRAM: %lu bytes", static_cast<unsigned long>(ESP.getPsramSize()));
  gfx->setCursor(40, 194);
  gfx->printf("Backlight GPIO: %d", BACKLIGHT);

  gfx->fillRect(420, 72, 350, 160, panel);
  gfx->drawRect(420, 72, 350, 160, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_CYAN, panel);
  gfx->setTextSize(2);
  gfx->setCursor(440, 92);
  gfx->print("Touch");

  gfx->setTextColor(COLOR_WHITE, panel);
  gfx->setTextSize(1);
  gfx->setCursor(440, 128);
  gfx->printf("I2C SDA/SCL: %d/%d", TOUCH_SDA, TOUCH_SCL);
  gfx->setCursor(440, 150);
  gfx->printf("GT911 addr: %s", touchAddr ? "detected" : "not found");
  gfx->setCursor(440, 172);
  if (touchAddr) {
    gfx->printf("Active: 0x%02X, point reg 0x814F", touchAddr);
  } else {
    gfx->print("Active: none");
  }
  gfx->setCursor(440, 194);
  gfx->print("Touch area below, buttons at bottom");

  gfx->fillRect(20, 252, 750, 118, rgb565(5, 10, 16));
  gfx->drawRect(20, 252, 750, 118, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_GRAY, rgb565(5, 10, 16));
  gfx->setTextSize(1);
  gfx->setCursor(34, 266);
  gfx->print("Live touch zone. Red marker follows mapped GT911 coordinates.");

  for (int x = 40; x < 760; x += 40) {
    gfx->drawFastVLine(x, 292, 64, rgb565(25, 35, 48));
  }
  for (int y = 292; y < 360; y += 20) {
    gfx->drawFastHLine(32, y, 720, rgb565(25, 35, 48));
  }

  drawButtons();
}

static void drawStatusLine(bool haveTouch) {
  gfx->fillRect(0, 456, LCD_W, 24, rgb565(18, 24, 31));
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE, rgb565(18, 24, 31));
  gfx->setCursor(12, 464);

  if (haveTouch) {
    gfx->printf("touch=%lu raw=(%u,%u) screen=(%d,%d) bl=%s action=%s",
                static_cast<unsigned long>(touchEvents),
                curRawX,
                curRawY,
                curScreenX,
                curScreenY,
                backlightOn ? "ON" : "OFF",
                lastAction);
  } else {
    gfx->printf("touch=%lu waiting... bl=%s action=%s heap=%lu",
                static_cast<unsigned long>(touchEvents),
                backlightOn ? "ON" : "OFF",
                lastAction,
                static_cast<unsigned long>(ESP.getFreeHeap()));
  }
}

static void drawTouchMarker() {
  if (lastMarkerX >= 0 && lastMarkerY >= 0) {
    gfx->drawCircle(lastMarkerX, lastMarkerY, 14, COLOR_GRAY);
    gfx->drawFastHLine(lastMarkerX - 18, lastMarkerY, 37, COLOR_GRAY);
    gfx->drawFastVLine(lastMarkerX, lastMarkerY - 18, 37, COLOR_GRAY);
  }

  gfx->drawCircle(curScreenX, curScreenY, 16, COLOR_RED);
  gfx->drawCircle(curScreenX, curScreenY, 17, COLOR_RED);
  gfx->drawFastHLine(curScreenX - 24, curScreenY, 49, COLOR_RED);
  gfx->drawFastVLine(curScreenX, curScreenY - 24, 49, COLOR_RED);

  lastMarkerX = curScreenX;
  lastMarkerY = curScreenY;
}

static void clearTouchCounter() {
  touchEvents = 0;
  lastMarkerX = -1;
  lastMarkerY = -1;
  snprintf(lastAction, sizeof(lastAction), "touch counter cleared");

  gfx->fillRect(20, 252, 750, 118, rgb565(5, 10, 16));
  gfx->drawRect(20, 252, 750, 118, COLOR_DARKGREY);
  gfx->setTextColor(COLOR_GRAY, rgb565(5, 10, 16));
  gfx->setTextSize(1);
  gfx->setCursor(34, 266);
  gfx->print("Live touch zone cleared.");
}

static void printReport(bool haveTouch) {
  Serial.println("----------------------------------------------------------------");
  Serial.println("05_TestConsole REPORT");
  Serial.printf("Chip              : %s rev %d, %u MHz\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("Flash             : %lu bytes\n", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("PSRAM             : %lu bytes\n", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("Free heap         : %lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
  Serial.printf("Backlight         : GPIO%d %s\n", BACKLIGHT, backlightOn ? "ON" : "OFF");
  Serial.printf("GT911             : %s", touchAddr ? "DETECTED" : "NOT DETECTED");
  if (touchAddr) {
    Serial.printf(" at 0x%02X", touchAddr);
  }
  Serial.println();
  Serial.printf("Touch events      : %lu\n", static_cast<unsigned long>(touchEvents));
  if (haveTouch) {
    Serial.printf("Last touch        : status=0x%02X points=%u track=%u raw=(%u,%u) screen=(%d,%d) size=%u\n",
                  curStatus,
                  curPoints,
                  curTrackId,
                  curRawX,
                  curRawY,
                  curScreenX,
                  curScreenY,
                  curSize);
  }
  Serial.printf("Last action       : %s\n", lastAction);
  Serial.println("----------------------------------------------------------------");
}

static void handleButtons() {
  const uint32_t now = millis();
  if (now - lastButtonMs < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (insideRect(BTN_BACKLIGHT_X, BTN_BACKLIGHT_Y, BTN_BACKLIGHT_W, BTN_BACKLIGHT_H, curScreenX, curScreenY)) {
    setBacklight(!backlightOn);
    snprintf(lastAction, sizeof(lastAction), "backlight %s", backlightOn ? "ON" : "OFF");
    Serial.printf("Button BACKLIGHT -> %s\n", backlightOn ? "ON" : "OFF");
    drawButtons();
    lastButtonMs = now;
  } else if (insideRect(BTN_CLEAR_X, BTN_CLEAR_Y, BTN_CLEAR_W, BTN_CLEAR_H, curScreenX, curScreenY)) {
    clearTouchCounter();
    Serial.println("Button CLEAR -> touch counter cleared");
    drawButtons();
    lastButtonMs = now;
  } else if (insideRect(BTN_REPORT_X, BTN_REPORT_Y, BTN_REPORT_W, BTN_REPORT_H, curScreenX, curScreenY)) {
    snprintf(lastAction, sizeof(lastAction), "serial report printed");
    printReport(true);
    drawButtons();
    lastButtonMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);
  board.begin();

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 05_TestConsole");
  Serial.println(" Combined RGB + GT911 + backlight diagnostic console");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Build  : Arduino IDE preprocessor-safe version, no struct function signatures");
  Serial.println("----------------------------------------------------------------");

  setBacklight(true);

  Serial.println("gfx->begin() start");
  const bool gfxOk = gfx->begin();
  Serial.printf("gfx->begin(): %s\n", gfxOk ? "OK" : "FAIL");
  if (!gfxOk) {
    Serial.println("Display init failed; stopping.");
    while (true) {
      delay(1000);
    }
  }

  Serial.printf("Wire.begin(SDA=%d, SCL=%d, speed=%lu)\n",
                TOUCH_SDA, TOUCH_SCL, static_cast<unsigned long>(I2C_SPEED_HZ));
  Wire.begin(TOUCH_SDA, TOUCH_SCL, I2C_SPEED_HZ);

  Serial.println("GT911 reset: RST38 toggle, INT18 passive pull-up, polling mode");
  gt911Reset();
  touchAddr = findGT911();

  Serial.print("I2C scan: ");
  const String devices = scanI2C();
  Serial.println(devices);

  if (touchAddr == 0) {
    Serial.println("GT911: NOT DETECTED at 0x5D or 0x14");
    snprintf(lastAction, sizeof(lastAction), "GT911 not detected");
  } else {
    Serial.printf("Active GT911 address: 0x%02X\n", touchAddr);
    printGt911Info();
    snprintf(lastAction, sizeof(lastAction), "GT911 ready at 0x%02X", touchAddr);
  }

  drawStaticConsole();
  drawStatusLine(false);
  printReport(false);

  Serial.println("----------------------------------------------------------------");
  Serial.println("Touch the screen. Buttons: BACKLIGHT, CLEAR, REPORT.");
  Serial.println("================================================================");
}

void loop() {
  const uint32_t now = millis();

  if (now - lastTouchPoll >= TOUCH_POLL_INTERVAL_MS) {
    lastTouchPoll = now;
    const bool haveTouch = readTouchCurrent();

    if (haveTouch) {
      const bool newPress = !lastTouchDown;
      lastTouchDown = true;

      if (newPress) {
        ++touchEvents;
      }

      if (now - lastTouchLog >= TOUCH_LOG_INTERVAL_MS) {
        lastTouchLog = now;
        Serial.printf("Touch #%lu: status=0x%02X points=%u track=%u raw=(%u,%u) screen=(%d,%d) size=%u\n",
                      static_cast<unsigned long>(touchEvents),
                      curStatus,
                      curPoints,
                      curTrackId,
                      curRawX,
                      curRawY,
                      curScreenX,
                      curScreenY,
                      curSize);
      }

      handleButtons();

      if (now - lastTouchDraw >= TOUCH_DRAW_INTERVAL_MS) {
        lastTouchDraw = now;
        drawTouchMarker();
        drawStatusLine(true);
      }
    } else {
      lastTouchDown = false;
    }
  }

  if (now - lastStatusUpdate >= UI_STATUS_INTERVAL_MS) {
    lastStatusUpdate = now;
    drawStatusLine(false);
  }

  delay(3);
}
