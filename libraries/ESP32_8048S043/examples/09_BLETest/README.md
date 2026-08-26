# 09_BLETest

Status: `BLE SCAN PHYSICAL PASS CANDIDATE / SAMPLE A`.

This example validates the ESP32-S3 BLE radio/stack with an active BLE scan.

It does not pair, connect, discover services or write to any BLE device.

## Why this test exists

The verified stack now has:

```text
01 BoardInfo / profile diagnostics
02 RGB display
03 GT911 touch
04 Backlight
05 Combined test console
06 Wi-Fi infrastructure
07 HTTP WebServer
08 SDCard read-only
```

`09_BLETest` validates the final simple radio layer before moving to LVGL and browser-controlled workflows.

## Sample A evidence

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/09-ble-scan-20260826.md
```

Observed build profile:

```text
ARDUINO_BOARD               : "ESP32_8048S043_LAB"
ARDUINO_VARIANT             : "esp32_8048s043_lab"
CONFIG_IDF_TARGET           : "esp32s3"
CONFIG_IDF_TARGET_ESP32S3   : 1
CONFIG_BT_ENABLED           : 1
BOARD_HAS_PSRAM             : defined
```

Observed runtime baseline:

```text
ESP-IDF SDK                 : v5.5.5
Chip                        : ESP32-S3 rev 2
CPU frequency               : 240 MHz
Flash                       : 16777216 bytes
PSRAM                       : 8388608 bytes
Free PSRAM                  : 8384064 bytes
```

Observed BLE initialization:

```text
[PASS] BLEDevice initialized
[INFO] Local BLE address: 84:fc:e6:6c:69:3d
[PASS] BLE active scan configured
```

Observed scan result:

```text
BLE SCAN PHYSICAL PASS CANDIDATE
Arduino BLE init + active scan + advertisement receive passed.
```

Final observed summary in the supplied log:

```text
Cycle results        : 3
Callback reports     : 3
Named reports        : 0
Total reports        : 30
Total named reports  : 0
```

Final observed ALIVE line:

```text
[ALIVE] uptime=772s bleInit=OK scans=26 lastScan=PASS_CANDIDATE reports=30 freeHeap=251124 psram=8388608 freePsram=8384064
```

## What it checks

```text
Arduino BLE library is available;
BLEDevice initializes;
local BLE address can be printed;
BLE active scan starts;
advertising reports are received and printed;
scan cycles complete without reset/brownout/crash;
ALIVE output continues after scan cycles.
```

## What it does not check

```text
Bluetooth Classic;
pairing;
GATT service discovery;
connecting to a BLE peripheral;
BLE characteristic reads/writes;
BLE HID;
BLE provisioning;
Wi-Fi/BLE coexistence under load;
display/LVGL integration.
```

## Arduino IDE setup

Use the same local profile that passed `01_BoardInfo`:

```text
Board             : ESP32-8048S043 Lab N16R8 FIXED
Flash Size        : 16MB (128Mb)
Flash Mode        : QIO 80MHz
Partition Scheme  : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM             : OPI PSRAM
Serial Monitor    : 115200 baud
```

`ESP32S3 Dev Module` remains a safe fallback profile while debugging.

## Running the test

Open:

```text
File -> Examples -> ESP32_8048S043 -> 09_BLETest
```

Place a BLE advertiser nearby before reset:

```text
phone;
smart watch;
fitness band;
BLE beacon;
Bluetooth keyboard/mouse in advertising mode.
```

The test repeats 15 second active BLE scan cycles.

## Expected serial output

A good run should include:

```text
ESP32-8048S043 Lab / 09_BLETest
BLE active scan validation

[BUILD PROFILE]
ARDUINO_BOARD               : "ESP32_8048S043_LAB"
ARDUINO_VARIANT             : "esp32_8048s043_lab"
CONFIG_IDF_TARGET           : "esp32s3"
CONFIG_BT_ENABLED           : 1
BOARD_HAS_PSRAM             : defined

[BLE INIT]
[PASS] BLEDevice initialized
[INFO] Local BLE address: ...
[PASS] BLE active scan configured

[BLE SCAN] Cycle 1, active scan for 15 second(s)
[ADV] #1 addr=... rssi=... name="..."

BLE SCAN PHYSICAL PASS CANDIDATE
Arduino BLE init + active scan + advertisement receive passed.
```

Then the sketch should continue printing:

```text
[ALIVE] uptime=... bleInit=OK scans=... lastScan=PASS_CANDIDATE reports=... freeHeap=... psram=... freePsram=...
```

## PASS boundary

```text
BLE SCAN PASS CANDIDATE:
  Arduino BLE library is available;
  BLEDevice initializes;
  local BLE address is available;
  BLE scan starts;
  one or more advertisement reports are received;
  scan cycles complete;
  ALIVE continues without reset/brownout/crash.
```

## Quiet RF boundary

If the scan completes with zero advertisements:

```text
BLE stack initialization may still be OK;
RF receive is not yet validated;
place a BLE advertiser nearby and rerun before claiming BLE physical PASS.
```

## Boundary

This test validates only Arduino-level BLE initialization and advertisement receive.

It does not validate pairing, GATT, HID, provisioning, Wi-Fi/BLE coexistence or display/LVGL integration.
