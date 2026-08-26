/*
  ESP32-8048S043 Lab / 09_BLETest

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    Validate the ESP32-S3 BLE radio/stack with a short read-only active scan
    after the board profile, RGB display, GT911 touch, backlight, Wi-Fi,
    WebServer and microSD layers have been brought up.

  What this example checks:
    - Arduino BLE library is available in the selected ESP32 core;
    - BLEDevice initializes;
    - local BLE address can be printed;
    - active BLE scan starts;
    - advertising reports are received and printed;
    - scan completes without reset/brownout/crash;
    - ALIVE output continues after scan completion.

  What this example does NOT check:
    - pairing;
    - connecting to a BLE peripheral;
    - GATT service discovery;
    - BLE writes;
    - BLE HID;
    - BLE provisioning;
    - Wi-Fi/BLE coexistence under load;
    - display/LVGL integration.

  Notes:
    - ESP32-S3 supports BLE, not Bluetooth Classic.
    - This example intentionally uses the Arduino BLE wrapper headers
      (BLEDevice.h / BLEScan.h) instead of including esp_bt_device.h directly,
      because some Arduino-ESP32 installations do not expose that low-level
      header to sketches.
    - A scan with zero devices can happen in a quiet RF environment. For a PASS
      candidate, place a phone, watch, beacon or other BLE advertiser nearby.
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static constexpr uint32_t SCAN_SECONDS = 15;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t MAX_PRINTED_REPORTS = 40;
static constexpr uint32_t RESCAN_DELAY_MS = 15000;

static BLEScan *bleScan = nullptr;

static uint32_t lastAliveMs = 0;
static uint32_t lastScanMs = 0;
static uint32_t scanCycles = 0;
static uint32_t totalReports = 0;
static uint32_t totalNamedReports = 0;
static uint32_t lastScanReports = 0;
static uint32_t lastScanNamedReports = 0;
static bool bleInitOk = false;
static bool lastScanOk = false;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printMacroLine(const char *name, bool defined) {
  Serial.printf("%-28s: %s\n", name, defined ? "defined" : "not defined");
}

static void printBuildProfile() {
  Serial.println("[BUILD PROFILE]");

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

#ifdef CONFIG_IDF_TARGET
  Serial.printf("%-28s: \"%s\"\n", "CONFIG_IDF_TARGET", CONFIG_IDF_TARGET);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_IDF_TARGET");
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
  Serial.printf("%-28s: %d\n", "CONFIG_IDF_TARGET_ESP32S3", CONFIG_IDF_TARGET_ESP32S3);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_IDF_TARGET_ESP32S3");
#endif

#ifdef CONFIG_BT_ENABLED
  Serial.printf("%-28s: %d\n", "CONFIG_BT_ENABLED", CONFIG_BT_ENABLED);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_BT_ENABLED");
#endif

#ifdef CONFIG_BLUEDROID_ENABLED
  Serial.printf("%-28s: %d\n", "CONFIG_BLUEDROID_ENABLED", CONFIG_BLUEDROID_ENABLED);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_BLUEDROID_ENABLED");
#endif

#ifdef CONFIG_BT_BLE_ENABLED
  Serial.printf("%-28s: %d\n", "CONFIG_BT_BLE_ENABLED", CONFIG_BT_BLE_ENABLED);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_BT_BLE_ENABLED");
#endif

#ifdef BOARD_HAS_PSRAM
  printMacroLine("BOARD_HAS_PSRAM", true);
#else
  printMacroLine("BOARD_HAS_PSRAM", false);
#endif
}

static void printRuntimeBaseline() {
  Serial.println("[RUNTIME]");
  Serial.printf("%-28s: %s\n", "ESP-IDF SDK", ESP.getSdkVersion());
  Serial.printf("%-28s: %s rev %u\n", "Chip", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("%-28s: %lu MHz\n", "CPU frequency", static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %lu bytes\n", "Free PSRAM", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("%-28s: %lu bytes\n", "Free heap", static_cast<unsigned long>(ESP.getFreeHeap()));
}

class BleScanCallbacks final : public BLEAdvertisedDeviceCallbacks {
 public:
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    ++lastScanReports;
    ++totalReports;

    const bool hasName = advertisedDevice.haveName();
    if (hasName) {
      ++lastScanNamedReports;
      ++totalNamedReports;
    }

    if (lastScanReports <= MAX_PRINTED_REPORTS) {
      const String address = advertisedDevice.getAddress().toString().c_str();
      const String name = hasName ? advertisedDevice.getName().c_str() : "<none>";
      const int rssi = advertisedDevice.getRSSI();

      Serial.printf("[ADV] #%lu addr=%s rssi=%d name=\"%s\"\n",
                    static_cast<unsigned long>(lastScanReports),
                    address.c_str(),
                    rssi,
                    name.c_str());
    }
  }
};

static BleScanCallbacks bleCallbacks;

static bool initBle() {
  Serial.println("[BLE INIT]");

  BLEDevice::init("ESP32-8048S043-Lab");
  Serial.println("[PASS] BLEDevice initialized");

  Serial.printf("[INFO] Local BLE address: %s\n", BLEDevice::getAddress().toString().c_str());

  bleScan = BLEDevice::getScan();
  if (!bleScan) {
    Serial.println("[FAIL] BLEDevice::getScan() returned nullptr");
    return false;
  }

  bleScan->setAdvertisedDeviceCallbacks(&bleCallbacks);
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(99);

  Serial.println("[PASS] BLE active scan configured");
  return true;
}

static void runScanCycle() {
  if (!bleInitOk || !bleScan) {
    return;
  }

  ++scanCycles;
  lastScanReports = 0;
  lastScanNamedReports = 0;
  lastScanOk = false;

  printDivider();
  Serial.printf("[BLE SCAN] Cycle %lu, active scan for %lu second(s)\n",
                static_cast<unsigned long>(scanCycles),
                static_cast<unsigned long>(SCAN_SECONDS));

  BLEScanResults *results = bleScan->start(SCAN_SECONDS, false);

  if (!results) {
    Serial.println("[FAIL] BLE scan returned nullptr");
    bleScan->clearResults();
    return;
  }

  const int resultCount = results->getCount();
  lastScanOk = resultCount > 0 || lastScanReports > 0;

  printDivider();
  Serial.println("[BLE SCAN SUMMARY]");
  Serial.printf("Cycle results        : %d\n", resultCount);
  Serial.printf("Callback reports     : %lu\n", static_cast<unsigned long>(lastScanReports));
  Serial.printf("Named reports        : %lu\n", static_cast<unsigned long>(lastScanNamedReports));
  Serial.printf("Total reports        : %lu\n", static_cast<unsigned long>(totalReports));
  Serial.printf("Total named reports  : %lu\n", static_cast<unsigned long>(totalNamedReports));

  if (lastScanOk) {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" BLE SCAN PHYSICAL PASS CANDIDATE");
    Serial.println(" Arduino BLE init + active scan + advertisement receive passed.");
    Serial.println("============================================================");
  } else {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" BLE STACK INITIALIZED / NO ADVERTISEMENTS SEEN");
    Serial.println(" Place a phone/watch/beacon nearby and reset or wait for rescan.");
    Serial.println("============================================================");
  }

  bleScan->clearResults();
  lastScanMs = millis();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32-8048S043 Lab / 09_BLETest");
  Serial.println(" BLE active scan validation");
  Serial.println("============================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("----------------------------------------------------------------");
  Serial.println("Mode   : BLE scan only, no pairing, no connect, no GATT writes");
  Serial.println("Target : ESP32-S3 BLE radio/stack sanity check");
  Serial.println("Serial : 115200 baud");
  Serial.println("----------------------------------------------------------------");

  printBuildProfile();
  printDivider();
  printRuntimeBaseline();
  printDivider();

  bleInitOk = initBle();

  if (!bleInitOk) {
    printDivider();
    Serial.println("============================================================");
    Serial.println(" BLE TEST FAIL / INIT ERROR");
    Serial.println(" Check Arduino ESP32 build profile and BLE library availability.");
    Serial.println("============================================================");
    return;
  }

  runScanCycle();
}

void loop() {
  const uint32_t now = millis();

  if (bleInitOk && now - lastScanMs >= RESCAN_DELAY_MS) {
    runScanCycle();
  }

  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] uptime=%lus bleInit=%s scans=%lu lastScan=%s reports=%lu freeHeap=%lu psram=%lu freePsram=%lu\n",
                  static_cast<unsigned long>(now / 1000),
                  bleInitOk ? "OK" : "FAIL",
                  static_cast<unsigned long>(scanCycles),
                  lastScanOk ? "PASS_CANDIDATE" : "OPEN_OR_NO_ADV",
                  static_cast<unsigned long>(totalReports),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(10);
}
