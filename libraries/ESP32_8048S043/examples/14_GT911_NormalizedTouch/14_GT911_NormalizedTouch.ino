/*
  ESP32-8048S043 Lab / 14_GT911_NormalizedTouch

  Purpose:
    Isolated GT911 BSP normalization diagnostic after the static esp_lcd/LVGL
    display path reached a pass candidate.

  What this example checks:
    - ESP32_8048S043_Touch BSP begin/read path;
    - GT911 address detection: 0x5D or 0x14;
    - GT911 firmware/resolution registers;
    - raw touch coordinates;
    - BSP-normalized 800x480 screen coordinates;
    - basic 3x3 zone sanity for corners, edges and center;
    - touch read counters and failure counters.

  What this example intentionally does NOT use:
    - display initialization;
    - Arduino_GFX;
    - esp_lcd display drawing;
    - LVGL;
    - moving markers;
    - Wi-Fi/SD/BLE.

  How to test:
    Open Serial Monitor at 115200 baud.
    Tap the physical panel in these positions:
      top-left, top-center, top-right,
      center-left, center, center-right,
      bottom-left, bottom-center, bottom-right.
    Watch raw coordinates, mapped coordinates and reported zones.
*/

#include <Arduino.h>
#include <Wire.h>
#include <ESP32_8048S043.h>

using namespace esp32_8048s043::pins;

#define SKETCH_ID "14TOUCH-NORM1-240827A"

static constexpr uint32_t SERIAL_BAUD = 115200;
static constexpr uint32_t TOUCH_POLL_INTERVAL_MS = 15;
static constexpr uint32_t TOUCH_LOG_INTERVAL_MS = 80;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint16_t MOVE_LOG_DEADBAND_PX = 3;

static ESP32_8048S043_Touch touch;

static uint32_t lastPollMs = 0;
static uint32_t lastLogMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t loopCount = 0;
static uint32_t localReadOk = 0;
static uint32_t localReadFail = 0;
static uint32_t localTouchReports = 0;
static uint32_t localReleaseReports = 0;
static uint16_t zoneMask = 0;
static bool wasTouched = false;
static int lastX = -10000;
static int lastY = -10000;
static uint32_t lastTouchMs = 0;

static const char *zoneName(uint8_t index) {
  switch (index) {
    case 0: return "TOP_LEFT";
    case 1: return "TOP_CENTER";
    case 2: return "TOP_RIGHT";
    case 3: return "CENTER_LEFT";
    case 4: return "CENTER";
    case 5: return "CENTER_RIGHT";
    case 6: return "BOTTOM_LEFT";
    case 7: return "BOTTOM_CENTER";
    case 8: return "BOTTOM_RIGHT";
    default: return "UNKNOWN";
  }
}

static uint8_t zoneIndex(uint16_t x, uint16_t y) {
  uint8_t col = 0;
  uint8_t row = 0;

  if (x >= LCD_WIDTH * 2 / 3) {
    col = 2;
  } else if (x >= LCD_WIDTH / 3) {
    col = 1;
  }

  if (y >= LCD_HEIGHT * 2 / 3) {
    row = 2;
  } else if (y >= LCD_HEIGHT / 3) {
    row = 1;
  }

  return static_cast<uint8_t>(row * 3 + col);
}

static uint8_t countZones(uint16_t mask) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < 9; ++i) {
    if (mask & (1U << i)) ++count;
  }
  return count;
}

static void printZoneMask() {
  Serial.print("[ZONES] seen=");
  Serial.print(countZones(zoneMask));
  Serial.print("/9 mask=0b");
  for (int i = 8; i >= 0; --i) {
    Serial.print((zoneMask & (1U << i)) ? '1' : '0');
  }
  Serial.print(" -> ");
  for (uint8_t i = 0; i < 9; ++i) {
    if (zoneMask & (1U << i)) {
      Serial.print(zoneName(i));
      Serial.print(' ');
    }
  }
  Serial.println();
}

static void printBanner() {
  Serial.println();
  Serial.println("================================================================");
  Serial.println(" ESP32-8048S043 Lab / 14_GT911_NormalizedTouch");
  Serial.println(" GT911 BSP normalized touch diagnostic");
  Serial.println("================================================================");
  Serial.printf("%-28s: %s\n", "Firmware ID", SKETCH_ID);
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
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %s rev %u\n", "Chip", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.println("----------------------------------------------------------------");
  Serial.printf("%-28s: GT911 BSP read only\n", "Mode");
  Serial.printf("%-28s: not used\n", "Display");
  Serial.printf("%-28s: not used\n", "Arduino_GFX");
  Serial.printf("%-28s: not used\n", "esp_lcd display");
  Serial.printf("%-28s: not used\n", "LVGL");
  Serial.printf("%-28s: %dx%d\n", "Mapped coordinate space", LCD_WIDTH, LCD_HEIGHT);
  Serial.printf("%-28s: SDA=%d SCL=%d RST=%d INT=%d\n", "GT911 pins", TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
  Serial.println("----------------------------------------------------------------");
  Serial.println("Tap physical screen zones: TL TC TR / CL C CR / BL BC BR.");
  Serial.println("Watch raw=(x,y), mapped=(x,y), zone and counters.");
  Serial.println("================================================================");
}

static void initBacklightSafeOff() {
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, LOW);
  Serial.println("[INFO] Backlight forced OFF: this is a touch-only serial diagnostic.");
}

static bool initTouch() {
  Serial.println("[TOUCH INIT]");

  if (!touch.begin(Wire)) {
    Serial.println("[FAIL] ESP32_8048S043_Touch::begin()");
    return false;
  }

  Serial.printf("[PASS] ESP32_8048S043_Touch::begin() addr=0x%02X fw=0x%04X res=%ux%u int=%d\n",
                touch.address(),
                touch.firmwareVersion(),
                touch.resolutionX(),
                touch.resolutionY(),
                touch.interruptLevel());
  Serial.println("[READY] Touch glass. Serial will report raw and normalized coordinates.");
  return true;
}

static void printAlive() {
  const uint32_t now = millis();
  if (now - lastAliveMs < ALIVE_INTERVAL_MS) return;
  lastAliveMs = now;

  Serial.printf("[ALIVE] fw=%s uptime=%lus loops=%lu readOk=%lu readFail=%lu touchReports=%lu releases=%lu zones=%u/9 statusReads=%lu ready=%lu zeroReady=%lu accepted=%lu filtered=%lu drvReadFail=%lu drvPointFail=%lu lastStatus=0x%02X int=%d heap=%lu psram=%lu freePsram=%lu\n",
                SKETCH_ID,
                static_cast<unsigned long>(now / 1000),
                static_cast<unsigned long>(loopCount),
                static_cast<unsigned long>(localReadOk),
                static_cast<unsigned long>(localReadFail),
                static_cast<unsigned long>(localTouchReports),
                static_cast<unsigned long>(localReleaseReports),
                countZones(zoneMask),
                static_cast<unsigned long>(touch.statusReads()),
                static_cast<unsigned long>(touch.readyReads()),
                static_cast<unsigned long>(touch.zeroPointReadyReads()),
                static_cast<unsigned long>(touch.acceptedPoints()),
                static_cast<unsigned long>(touch.filteredUpdates()),
                static_cast<unsigned long>(touch.readFailures()),
                static_cast<unsigned long>(touch.pointFailures()),
                touch.lastStatus(),
                touch.interruptLevel(),
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getPsramSize()),
                static_cast<unsigned long>(ESP.getFreePsram()));
}

static void processTouchPoint(const ESP32_8048S043_TouchPoint &p) {
  const uint32_t now = millis();
  const uint8_t zi = zoneIndex(p.x, p.y);
  const uint16_t oldMask = zoneMask;
  zoneMask |= static_cast<uint16_t>(1U << zi);

  const bool movedEnough = abs(static_cast<int>(p.x) - lastX) >= MOVE_LOG_DEADBAND_PX ||
                           abs(static_cast<int>(p.y) - lastY) >= MOVE_LOG_DEADBAND_PX;
  const bool timeEnough = now - lastLogMs >= TOUCH_LOG_INTERVAL_MS;
  const bool firstAfterRelease = !wasTouched;

  if (firstAfterRelease || movedEnough || timeEnough || oldMask != zoneMask) {
    Serial.printf("[TOUCH] raw=(%3u,%3u) mapped=(%3u,%3u) zone=%s size=%u id=%u status=0x%02X accepted=%lu filtered=%lu\n",
                  p.rawX,
                  p.rawY,
                  p.x,
                  p.y,
                  zoneName(zi),
                  p.size,
                  p.trackId,
                  p.status,
                  static_cast<unsigned long>(touch.acceptedPoints()),
                  static_cast<unsigned long>(touch.filteredUpdates()));
    lastLogMs = now;
    lastX = p.x;
    lastY = p.y;
    ++localTouchReports;
  }

  if (oldMask != zoneMask) {
    printZoneMask();
  }

  wasTouched = true;
  lastTouchMs = now;
}

static void processRelease() {
  const uint32_t now = millis();
  if (!wasTouched) return;
  if (now - lastTouchMs < 120) return;

  wasTouched = false;
  lastX = -10000;
  lastY = -10000;
  ++localReleaseReports;
  Serial.printf("[RELEASE] releases=%lu zones=%u/9\n",
                static_cast<unsigned long>(localReleaseReports),
                countZones(zoneMask));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  printBanner();
  initBacklightSafeOff();

  if (!initTouch()) {
    Serial.println("[STOP] Touch init failed. Check SDA/SCL/RST, power and board profile.");
    while (true) {
      printAlive();
      delay(100);
    }
  }
}

void loop() {
  ++loopCount;
  const uint32_t now = millis();

  if (now - lastPollMs >= TOUCH_POLL_INTERVAL_MS) {
    lastPollMs = now;

    ESP32_8048S043_TouchPoint point;
    const bool ok = touch.read(point);
    if (ok) {
      ++localReadOk;
      if (point.touched) {
        processTouchPoint(point);
      } else {
        processRelease();
      }
    } else {
      ++localReadFail;
      processRelease();
    }
  }

  printAlive();
  delay(1);
}
