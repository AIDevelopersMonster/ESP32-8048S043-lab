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

  What this example does NOT check:
    - LVGL;
    - gestures;
    - SD card;
    - Wi-Fi/BLE;
    - final production UI framework;
    - final calibration quality.

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
static constexpr uint32_t UI_STATUS_INTERVAL_MS = 1000;

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

struct TouchSample {
  uint8_t status;
  uint8_t points;
  uint8_t trackId;
  uint16_t rawX;
  uint16_t rawY;
  uint16_t size;
  int screenX;
  int screenY;
};

struct Button {
  int x;
  int y;
  int w;
  int h;
  const char *label;
};

static const Button BTN_BACKLIGHT = {40, 390, 190, 54, "BACKLIGHT"};
static const Button BTN_CLEAR = {260, 390, 190, 54, "CLEAR"};
static const Button BTN_REPORT = {480, 390, 190, 54, "REPORT"};

static uint8_t touchAddr = 0;
static bool backlightOn = true;
static uint32_t touchEvents = 0;
static uint32_t lastTouchPoll = 0;
static uint32_t lastTouchLog = 0;
static uint32_t lastStatusUpdate = 0;
static int lastMarkerX = -1;
static int lastMarkerY = -1;
static bool lastTouchDown = false;
static uint32_t lastButtonMs = 0;
static char lastAction[64] = "Console started";

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

static bool readTouch(TouchSample &touch) {
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

  touch.status = status;
  touch.points = points;
  touch.trackId = data[0];
  touch.rawX = le16(&data[1]);
  touch.rawY = le16(&data[3]);
  touch.size = le16(&data[5]);
  touch.screenX = mapTouchX(touch.rawX, touch.rawY);
  touch.screenY = mapTouchY(touch.rawX, touch.rawY);
  return true;
}

static bool insideButton(const Button &button, int x, int y) {
  return (x >= button.x) && (x < button.x + button.w) &&
         (y >= button.y) && (y < button.y + button.h);
}

static void drawButton(const Button &button, uint16_t color, const char *value) {
  gfx->fillRect(button.x, button.y, button.w, button.h, rgb565(20, 26, 34));
  gfx->drawRect(button.x, button.y, button.w, button.h, color);
  gfx->drawRect(button.x + 1, button.y + 1, button.w - 2, button.h - 2, color);
  gfx->setTextSize(2);
  gfx->setTextColor(color, rgb565(20, 26, 34));
  gfx->setCursor(button.x + 14, button.y + 10);
  gfx->print(button.label);
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE, rgb565(20, 26, 34));
  gfx->setCursor(button.x + 14, button.y + 36);
  gfx->print(value);
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
  gfx->printf("GT911 addr: %s", touchAddr ? "active" : "not found");
  gfx->setCursor(440, 172);
  gfx->print("Point register: 0x814F");
  gfx->setCursor(440, 194);
  gfx->print("Touch screen to test marker/buttons");

  gfx->fillRect(20, 252, 750, 118, rgb565(8, 13, 18));
  gfx->drawRect(20, 252, 750, 118, COLOR_DARKGREY);
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_GRAY, rgb565(8, 13, 18));
  for (int x = 40; x < 760; x += 40) {
    gfx->drawFastVLine(x, 253, 116, rgb565(30, 42, 52));
  }
  for (int y = 272; y < 368; y += 24) {
    gfx->drawFastHLine(21, y, 748, rgb565(30, 42, 52));
  }
  gfx->setCursor(34, 262);
  gfx->print("touch area / live marker");

  drawButton(BTN_BACKLIGHT, COLOR_YELLOW, backlightOn ? "tap: turn OFF" : "tap: turn ON");
  drawButton(BTN_CLEAR, COLOR_CYAN, "tap: reset counter");
  drawButton(BTN_REPORT, COLOR_GREEN, "tap: print serial");
}

static void drawStatusLine(const TouchSample *touch) {
  gfx->fillRect(20, 454, 760, 22, COLOR_BLACK);
  gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(24, 461);

  if (touch) {
    gfx->printf("touch#%lu raw=(%u,%u) screen=(%d,%d) size=%u bl=%s heap=%lu action=%s",
                static_cast<unsigned long>(touchEvents),
                touch->rawX,
                touch->rawY,
                touch->screenX,
                touch->screenY,
                touch->size,
                backlightOn ? "ON" : "OFF",
                static_cast<unsigned long>(ESP.getFreeHeap()),
                lastAction);
  } else {
    gfx->printf("touch#%lu gt911=%s bl=%s heap=%lu psram_free=%lu action=%s",
                static_cast<unsigned long>(touchEvents),
                touchAddr ? "OK" : "NO",
                backlightOn ? "ON" : "OFF",
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getFreePsram()),
                lastAction);
  }
}

static void drawTouchMarker(int x, int y) {
  // Keep the marker mostly in the live touch area to avoid destroying buttons.
  const int drawX = constrain(x, 24, LCD_W - 25);
  const int drawY = constrain(y, 56, LCD_H - 30);

  if (lastMarkerX >= 0 && lastMarkerY >= 0) {
    gfx->drawCircle(lastMarkerX, lastMarkerY, 15, COLOR_GRAY);
    gfx->drawFastHLine(lastMarkerX - 20, lastMarkerY, 41, COLOR_GRAY);
    gfx->drawFastVLine(lastMarkerX, lastMarkerY - 20, 41, COLOR_GRAY);
  }

  gfx->drawCircle(drawX, drawY, 16, COLOR_RED);
  gfx->drawFastHLine(drawX - 22, drawY, 45, COLOR_RED);
  gfx->drawFastVLine(drawX, drawY - 22, 45, COLOR_RED);
  lastMarkerX = drawX;
  lastMarkerY = drawY;
}

static void printReport(const TouchSample *touch) {
  Serial.println("[TEST CONSOLE REPORT]");
  Serial.printf("Uptime ms        : %lu\n", static_cast<unsigned long>(millis()));
  Serial.printf("Chip             : %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("CPU MHz          : %u\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash bytes      : %lu\n", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("PSRAM bytes      : %lu\n", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("Free heap        : %lu\n", static_cast<unsigned long>(ESP.getFreeHeap()));
  Serial.printf("Free PSRAM       : %lu\n", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("Backlight GPIO2  : %s\n", backlightOn ? "ON" : "OFF");
  Serial.printf("GT911 address    : %s\n", touchAddr ? "active" : "not found");
  if (touchAddr) {
    Serial.printf("GT911 address hex: 0x%02X\n", touchAddr);
  }
  Serial.printf("Touch events     : %lu\n", static_cast<unsigned long>(touchEvents));
  if (touch) {
    Serial.printf("Last touch       : raw=(%u,%u) screen=(%d,%d) size=%u track=%u\n",
                  touch->rawX, touch->rawY, touch->screenX, touch->screenY, touch->size, touch->trackId);
  }
  Serial.println("[/TEST CONSOLE REPORT]");
}

static void handleButtons(const TouchSample &touch) {
  const uint32_t now = millis();
  if (now - lastButtonMs < 450) {
    return;
  }

  if (insideButton(BTN_BACKLIGHT, touch.screenX, touch.screenY)) {
    lastButtonMs = now;
    setBacklight(!backlightOn);
    snprintf(lastAction, sizeof(lastAction), "Backlight %s", backlightOn ? "ON" : "OFF");
    Serial.printf("Button: BACKLIGHT -> %s\n", backlightOn ? "ON" : "OFF");
    drawButton(BTN_BACKLIGHT, COLOR_YELLOW, backlightOn ? "tap: turn OFF" : "tap: turn ON");
  } else if (insideButton(BTN_CLEAR, touch.screenX, touch.screenY)) {
    lastButtonMs = now;
    touchEvents = 0;
    lastMarkerX = -1;
    lastMarkerY = -1;
    snprintf(lastAction, sizeof(lastAction), "Touch counter cleared");
    Serial.println("Button: CLEAR -> touch counter reset");
    drawStaticConsole();
  } else if (insideButton(BTN_REPORT, touch.screenX, touch.screenY)) {
    lastButtonMs = now;
    snprintf(lastAction, sizeof(lastAction), "Report printed");
    Serial.println("Button: REPORT");
    printReport(&touch);
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);
  board.begin();

  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 05_TestConsole");
  Serial.println(" Combined RGB + GT911 + Backlight diagnostic console");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Serial : 115200 baud");
  Serial.println("----------------------------------------------------------------");

  setBacklight(true);
  delay(200);

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
  gt911Reset();
  touchAddr = findGT911();

  Serial.print("I2C scan: ");
  Serial.println(scanI2C());
  if (touchAddr) {
    Serial.printf("Active GT911 address: 0x%02X\n", touchAddr);
    printGt911Info();
    snprintf(lastAction, sizeof(lastAction), "GT911 0x%02X active", touchAddr);
  } else {
    Serial.println("GT911 not detected at 0x5D or 0x14");
    snprintf(lastAction, sizeof(lastAction), "GT911 not detected");
  }

  drawStaticConsole();
  drawStatusLine(nullptr);

  Serial.println("----------------------------------------------------------------");
  Serial.println("Touch screen to move marker. Use buttons: BACKLIGHT / CLEAR / REPORT.");
  Serial.println("PASS candidate when display + touch + backlight button work together.");
  Serial.println("================================================================");
}

void loop() {
  const uint32_t now = millis();

  if (now - lastTouchPoll >= TOUCH_POLL_INTERVAL_MS) {
    lastTouchPoll = now;

    TouchSample touch = {};
    if (readTouch(touch)) {
      const bool firstDown = !lastTouchDown;
      lastTouchDown = true;
      ++touchEvents;

      if (now - lastTouchLog >= TOUCH_LOG_INTERVAL_MS) {
        lastTouchLog = now;
        Serial.printf("Touch #%lu: track=%u raw_x=%u raw_y=%u screen_x=%d screen_y=%d size=%u\n",
                      static_cast<unsigned long>(touchEvents),
                      touch.trackId,
                      touch.rawX,
                      touch.rawY,
                      touch.screenX,
                      touch.screenY,
                      touch.size);
      }

      if (firstDown || now - lastButtonMs > 450) {
        handleButtons(touch);
      }
      drawTouchMarker(touch.screenX, touch.screenY);
      drawStatusLine(&touch);
    } else {
      lastTouchDown = false;
    }
  }

  if (now - lastStatusUpdate >= UI_STATUS_INTERVAL_MS) {
    lastStatusUpdate = now;
    drawStatusLine(nullptr);
  }

  delay(4);
}
