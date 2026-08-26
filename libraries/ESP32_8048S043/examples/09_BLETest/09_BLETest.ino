/*
  ESP32-8048S043 Lab / 09_BLETest

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    Validate the ESP32-S3 BLE radio/stack with a short read-only scan test after
    the board profile, RGB display, GT911 touch, backlight, Wi-Fi, WebServer and
    microSD layers have been brought up.

  What this example checks:
    - Bluetooth controller initializes in BLE mode;
    - Bluedroid host initializes and enables;
    - local BLE controller address can be read;
    - BLE active scan starts;
    - advertising reports are received and printed;
    - scan completes without reset/brownout/crash;
    - ALIVE output continues after scan completion.

  What this example does NOT check:
    - pairing;
    - GATT service discovery;
    - connecting to a BLE peripheral;
    - BLE HID;
    - BLE provisioning;
    - Wi-Fi/BLE coexistence under load;
    - display/LVGL integration.

  Notes:
    - ESP32-S3 supports BLE, not Bluetooth Classic.
    - This test uses ESP-IDF Bluedroid GAP APIs directly rather than the higher-level
      Arduino BLE wrapper, so the output describes the runtime stack more explicitly.
    - A scan with zero devices can happen in a quiet RF environment. For a PASS
      candidate, place a phone, watch, beacon or other BLE advertiser nearby.
*/

#include <Arduino.h>

#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_err.h"

static constexpr uint32_t SCAN_DURATION_SECONDS = 15;
static constexpr uint32_t ALIVE_INTERVAL_MS = 5000;
static constexpr uint32_t MAX_PRINTED_REPORTS = 40;

static volatile bool bleInitOk = false;
static volatile bool scanStarted = false;
static volatile bool scanCompleted = false;
static volatile bool summaryPrinted = false;

static uint32_t scanStartMs = 0;
static uint32_t scanCompleteMs = 0;
static uint32_t lastAliveMs = 0;
static uint32_t advReports = 0;
static uint32_t printedReports = 0;
static uint32_t namedReports = 0;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static void printMac(const uint8_t *addr, char *out, size_t outSize) {
  if (!addr || !out || outSize < 18) {
    return;
  }
  snprintf(out, outSize, "%02X:%02X:%02X:%02X:%02X:%02X",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
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

#ifdef CONFIG_BTDM_CTRL_MODE_BLE_ONLY
  Serial.printf("%-28s: %d\n", "CONFIG_BTDM_CTRL_MODE_BLE_ONLY", CONFIG_BTDM_CTRL_MODE_BLE_ONLY);
#else
  Serial.printf("%-28s: not defined\n", "CONFIG_BTDM_CTRL_MODE_BLE_ONLY");
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
  Serial.printf("%-28s: %lu Hz\n", "CPU frequency", static_cast<unsigned long>(ESP.getCpuFreqMHz() * 1000000UL));
  Serial.printf("%-28s: %lu bytes\n", "Flash", static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("%-28s: %lu bytes\n", "PSRAM", static_cast<unsigned long>(ESP.getPsramSize()));
  Serial.printf("%-28s: %lu bytes\n", "Free PSRAM", static_cast<unsigned long>(ESP.getFreePsram()));
  Serial.printf("%-28s: %lu bytes\n", "Free heap", static_cast<unsigned long>(ESP.getFreeHeap()));
}

static const char *errName(esp_err_t err) {
  const char *name = esp_err_to_name(err);
  return name ? name : "UNKNOWN";
}

static bool printLocalBleAddress() {
  const uint8_t *addr = esp_bt_dev_get_address();
  if (!addr) {
    Serial.println("[WARN] Local BLE address is not available yet");
    return false;
  }

  char mac[18] = {0};
  printMac(addr, mac, sizeof(mac));
  Serial.printf("[INFO] Local BLE address: %s\n", mac);
  return true;
}

static void printAdvName(const uint8_t *advData, uint8_t advLen, uint8_t scanRspLen, char *out, size_t outSize) {
  if (!out || outSize == 0) {
    return;
  }

  out[0] = '\0';

  if (!advData) {
    return;
  }

  uint8_t nameLen = 0;
  uint8_t *name = esp_ble_resolve_adv_data_by_type(const_cast<uint8_t *>(advData),
                                                   advLen + scanRspLen,
                                                   ESP_BLE_AD_TYPE_NAME_CMPL,
                                                   &nameLen);

  if (!name || nameLen == 0) {
    name = esp_ble_resolve_adv_data_by_type(const_cast<uint8_t *>(advData),
                                            advLen + scanRspLen,
                                            ESP_BLE_AD_TYPE_NAME_SHORT,
                                            &nameLen);
  }

  if (!name || nameLen == 0) {
    return;
  }

  const size_t copyLen = min(static_cast<size_t>(nameLen), outSize - 1);
  memcpy(out, name, copyLen);
  out[copyLen] = '\0';
}

static void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      Serial.println("[BLE] Scan parameters set");
      scanStartMs = millis();
      const esp_err_t err = esp_ble_gap_start_scanning(SCAN_DURATION_SECONDS);
      if (err == ESP_OK) {
        Serial.printf("[BLE] Starting active BLE scan for %lu second(s)\n", static_cast<unsigned long>(SCAN_DURATION_SECONDS));
      } else {
        Serial.printf("[FAIL] esp_ble_gap_start_scanning(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
      }
      break;
    }

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
        scanStarted = true;
        Serial.println("[PASS] BLE scan started");
      } else {
        Serial.printf("[FAIL] BLE scan start failed, status=0x%X\n", static_cast<unsigned int>(param->scan_start_cmpl.status));
      }
      break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      esp_ble_gap_cb_param_t *scanResult = param;

      if (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        ++advReports;

        if (printedReports < MAX_PRINTED_REPORTS) {
          char mac[18] = {0};
          char name[40] = {0};
          printMac(scanResult->scan_rst.bda, mac, sizeof(mac));
          printAdvName(scanResult->scan_rst.ble_adv,
                       scanResult->scan_rst.adv_data_len,
                       scanResult->scan_rst.scan_rsp_len,
                       name,
                       sizeof(name));

          if (name[0] != '\0') {
            ++namedReports;
            Serial.printf("[ADV] #%lu addr=%s rssi=%d type=%u event=%u adv=%u scanRsp=%u name=\"%s\"\n",
                          static_cast<unsigned long>(advReports),
                          mac,
                          static_cast<int>(scanResult->scan_rst.rssi),
                          static_cast<unsigned int>(scanResult->scan_rst.ble_addr_type),
                          static_cast<unsigned int>(scanResult->scan_rst.ble_evt_type),
                          static_cast<unsigned int>(scanResult->scan_rst.adv_data_len),
                          static_cast<unsigned int>(scanResult->scan_rst.scan_rsp_len),
                          name);
          } else {
            Serial.printf("[ADV] #%lu addr=%s rssi=%d type=%u event=%u adv=%u scanRsp=%u name=<none>\n",
                          static_cast<unsigned long>(advReports),
                          mac,
                          static_cast<int>(scanResult->scan_rst.rssi),
                          static_cast<unsigned int>(scanResult->scan_rst.ble_addr_type),
                          static_cast<unsigned int>(scanResult->scan_rst.ble_evt_type),
                          static_cast<unsigned int>(scanResult->scan_rst.adv_data_len),
                          static_cast<unsigned int>(scanResult->scan_rst.scan_rsp_len));
          }

          ++printedReports;
        }
      } else if (scanResult->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
        scanCompleted = true;
        scanCompleteMs = millis();
      }
      break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      if (param->scan_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
        scanCompleted = true;
        scanCompleteMs = millis();
        Serial.println("[BLE] Scan stopped");
      } else {
        Serial.printf("[WARN] BLE scan stop status=0x%X\n", static_cast<unsigned int>(param->scan_stop_cmpl.status));
      }
      break;

    default:
      break;
  }
}

static bool initBle() {
  Serial.println("[BLE INIT]");

  esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  if (err == ESP_OK) {
    Serial.println("[INFO] Released Classic BT memory; ESP32-S3 BLE-only path");
  } else {
    Serial.printf("[INFO] Classic BT memory release skipped/returned: %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
  }

  esp_bt_controller_status_t controllerStatus = esp_bt_controller_get_status();

  if (controllerStatus == ESP_BT_CONTROLLER_STATUS_IDLE) {
    esp_bt_controller_config_t btConfig = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&btConfig);
    if (err != ESP_OK) {
      Serial.printf("[FAIL] esp_bt_controller_init(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
      return false;
    }
    Serial.println("[PASS] BT controller initialized");
  } else {
    Serial.printf("[INFO] BT controller status before init: %d\n", static_cast<int>(controllerStatus));
  }

  controllerStatus = esp_bt_controller_get_status();
  if (controllerStatus != ESP_BT_CONTROLLER_STATUS_ENABLED) {
    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK) {
      Serial.printf("[FAIL] esp_bt_controller_enable(ESP_BT_MODE_BLE): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
      return false;
    }
    Serial.println("[PASS] BT controller enabled in BLE mode");
  }

  esp_bluedroid_status_t bluedroidStatus = esp_bluedroid_get_status();
  if (bluedroidStatus == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
    esp_bluedroid_config_t bluedroidConfig = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    err = esp_bluedroid_init_with_cfg(&bluedroidConfig);
    if (err != ESP_OK) {
      Serial.printf("[FAIL] esp_bluedroid_init_with_cfg(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
      return false;
    }
    Serial.println("[PASS] Bluedroid initialized");
  } else {
    Serial.printf("[INFO] Bluedroid status before init: %d\n", static_cast<int>(bluedroidStatus));
  }

  bluedroidStatus = esp_bluedroid_get_status();
  if (bluedroidStatus != ESP_BLUEDROID_STATUS_ENABLED) {
    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
      Serial.printf("[FAIL] esp_bluedroid_enable(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
      return false;
    }
    Serial.println("[PASS] Bluedroid enabled");
  }

  printLocalBleAddress();

  err = esp_ble_gap_register_callback(gapCallback);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] esp_ble_gap_register_callback(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
    return false;
  }
  Serial.println("[PASS] BLE GAP callback registered");

  esp_ble_scan_params_t scanParams = {};
  scanParams.scan_type = BLE_SCAN_TYPE_ACTIVE;
  scanParams.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  scanParams.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
  scanParams.scan_interval = 0x50;
  scanParams.scan_window = 0x30;
  scanParams.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;

  err = esp_ble_gap_set_scan_params(&scanParams);
  if (err != ESP_OK) {
    Serial.printf("[FAIL] esp_ble_gap_set_scan_params(): %s / 0x%X\n", errName(err), static_cast<unsigned int>(err));
    return false;
  }

  Serial.println("[PASS] BLE scan parameter request sent");
  return true;
}

static void printSummaryOnce() {
  if (summaryPrinted || !scanCompleted) {
    return;
  }

  summaryPrinted = true;

  printDivider();
  Serial.println("[BLE SCAN SUMMARY]");
  Serial.printf("Scan duration request : %lu second(s)\n", static_cast<unsigned long>(SCAN_DURATION_SECONDS));
  Serial.printf("Scan elapsed          : %lu ms\n", static_cast<unsigned long>(scanCompleteMs > scanStartMs ? scanCompleteMs - scanStartMs : 0));
  Serial.printf("Advertisement reports : %lu\n", static_cast<unsigned long>(advReports));
  Serial.printf("Printed reports       : %lu\n", static_cast<unsigned long>(printedReports));
  Serial.printf("Named reports         : %lu\n", static_cast<unsigned long>(namedReports));

  if (advReports > 0) {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" BLE SCAN PHYSICAL PASS CANDIDATE");
    Serial.println(" Controller + Bluedroid + active scan + advertisement receive passed.");
    Serial.println("============================================================");
  } else {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" BLE STACK INITIALIZED / NO ADVERTISEMENTS SEEN");
    Serial.println(" Place a phone/watch/beacon nearby and reset to validate RF receive.");
    Serial.println("============================================================");
  }
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
    Serial.println(" Check Arduino ESP32 build profile and Bluetooth support.");
    Serial.println("============================================================");
  }
}

void loop() {
  printSummaryOnce();

  const uint32_t now = millis();
  if (now - lastAliveMs >= ALIVE_INTERVAL_MS) {
    lastAliveMs = now;
    Serial.printf("[ALIVE] uptime=%lus bleInit=%s scanStarted=%s scanDone=%s advReports=%lu freeHeap=%lu psram=%lu freePsram=%lu\n",
                  static_cast<unsigned long>(now / 1000),
                  bleInitOk ? "OK" : "FAIL",
                  scanStarted ? "YES" : "NO",
                  scanCompleted ? "YES" : "NO",
                  static_cast<unsigned long>(advReports),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
  }

  delay(10);
}
