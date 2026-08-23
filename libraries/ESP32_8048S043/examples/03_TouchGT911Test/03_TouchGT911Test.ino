/*
  ESP32-8048S043 Lab / 03_TouchGT911Test

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Test order:
    01_BoardInfo       -> PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
    02_DisplayRGBTest  -> PASS: own Arduino_GFX RGB display path
    03_TouchGT911Test  -> this test, first own GT911 touch validation

  Purpose:
    First minimal GT911 capacitive touch validation from our ESP32_8048S043
    Arduino library. This test is serial-first: it does not draw to the LCD.

  What this example checks:
    - source-backed I2C pins SDA=19 and SCL=20;
    - GT911 candidate addresses 0x5D and 0x14;
    - optional GT911 reset/address strap using RST=38 and INT=18;
    - GT911 Product ID register at 0x8140;
    - firmware/config/resolution registers where readable;
    - touch status register at 0x814E;
    - raw touch point coordinates while touching the screen.

  What this example does NOT check:
    - RGB display rendering;
    - coordinate mapping to LVGL/display orientation;
    - multitouch gesture handling;
    - touch calibration;
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

  Expected serial result:
    - I2C scan should find GT911 at 0x5D or 0x14;
    - Product ID should read as a GT911-like ASCII/product value;
    - touching the screen should print raw x/y coordinates;
    - coordinates should change according to finger movement;
    - test should keep running without resets.

  PASS boundary:
    PASS here means our own serial GT911/I2C test detects the touch controller
    and reports changing raw coordinates on Sample A. It does not prove final
    LVGL coordinate mapping or full touch UI integration.
*/

#include <Arduino.h>
#include <Wire.h>
#include <ESP32_8048S043.h>
#include <ESP32_8048S043_Pins.h>

using namespace esp32_8048s043::pins;

ESP32_8048S043 board;

static constexpr uint32_t I2C_SPEED_HZ = 400000;
static constexpr uint16_t GT911_REG_PRODUCT_ID = 0x8140;
static constexpr uint16_t GT911_REG_FW_VERSION = 0x8144;
static constexpr uint16_t GT911_REG_X_RESOLUTION = 0x8146;
static constexpr uint16_t GT911_REG_Y_RESOLUTION = 0x8148;
static constexpr uint16_t GT911_REG_STATUS = 0x814E;
static constexpr uint16_t GT911_REG_POINT1 = 0x8150;
static constexpr uint8_t MAX_TOUCH_POINTS = 5;

static uint8_t activeAddress = 0;
static uint32_t lastIdlePrint = 0;

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
                  isPrintable(product[0]) ? product[0] : '.',
                  isPrintable(product[1]) ? product[1] : '.',
                  isPrintable(product[2]) ? product[2] : '.',
                  isPrintable(product[3]) ? product[3] : '.');
  } else {
    Serial.println("Product ID read: FAIL");
  }

  uint8_t fw[2] = {0};
  if (gt911Read(address, GT911_REG_FW_VERSION, fw, sizeof(fw))) {
    Serial.printf("FW version     : 0x%04X (%u)\n", le16(fw), le16(fw));
  } else {
    Serial.println("FW version read: FAIL");
  }

  uint8_t xres[2] = {0};
  uint8_t yres[2] = {0};
  bool xOk = gt911Read(address, GT911_REG_X_RESOLUTION, xres, sizeof(xres));
  bool yOk = gt911Read(address, GT911_REG_Y_RESOLUTION, yres, sizeof(yres));
  if (xOk && yOk) {
    Serial.printf("Touch resolution: X=%u Y=%u\n", le16(xres), le16(yres));
  } else {
    Serial.println("Touch resolution read: FAIL or unsupported");
  }

  Serial.println("Touch polling started. Touch the screen and watch raw coordinates.");
}

static void pollTouch(uint8_t address) {
  uint8_t status = 0;
  if (!gt911Read(address, GT911_REG_STATUS, &status, 1)) {
    if (millis() - lastIdlePrint > 3000) {
      lastIdlePrint = millis();
      Serial.println("Touch status read: FAIL");
    }
    return;
  }

  const bool dataReady = (status & 0x80) != 0;
  const uint8_t pointCount = status & 0x0F;

  if (!dataReady) {
    if (millis() - lastIdlePrint > 3000) {
      lastIdlePrint = millis();
      Serial.println("Touch idle: no new point data");
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
    const uint16_t x = le16(&point[1]);
    const uint16_t y = le16(&point[3]);
    const uint16_t size = le16(&point[5]);

    Serial.printf("  P%u id=%u x=%u y=%u size=%u raw=%02X %02X %02X %02X %02X %02X %02X %02X\n",
                  i + 1,
                  id,
                  x,
                  y,
                  size,
                  point[0], point[1], point[2], point[3], point[4], point[5], point[6], point[7]);
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
  Serial.println(" GT911 I2C address + raw coordinate validation");
  Serial.println("================================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("Purpose: validate own GT911 touch path after 01/02 PASS");
  Serial.println("----------------------------------------------------------------");

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
    return;
  }

  Serial.println("----------------------------------------------------------------");
  Serial.printf("Active GT911 address: 0x%02X\n", activeAddress);
  printGt911Info(activeAddress);
  Serial.println("----------------------------------------------------------------");
  Serial.println("PASS candidate if Product ID is readable and touches print changing x/y coordinates.");
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
