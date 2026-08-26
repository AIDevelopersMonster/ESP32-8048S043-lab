# 09_BLETest

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

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

## What it checks

```text
Bluetooth controller initializes in BLE mode;
Bluedroid host initializes and enables;
local BLE controller address can be read;
BLE active scan starts;
advertising reports are received and printed;
scan completes without reset/brownout/crash;
ALIVE output continues after scan completion.
```

## What it does not check

```text
Bluetooth Classic;
pairing;
GATT service discovery;
connecting to a BLE peripheral;
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

The test requests a 15 second active BLE scan.

## Expected serial output

A good run should include:

```text
ESP32-8048S043 Lab / 09_BLETest
BLE active scan validation

[BUILD PROFILE]
ARDUINO_BOARD
ARDUINO_VARIANT
CONFIG_IDF_TARGET
CONFIG_BT_ENABLED
CONFIG_BLUEDROID_ENABLED
CONFIG_BT_BLE_ENABLED

[BLE INIT]
[PASS] BT controller initialized
[PASS] BT controller enabled in BLE mode
[PASS] Bluedroid initialized
[PASS] Bluedroid enabled
[INFO] Local BLE address: ...
[PASS] BLE GAP callback registered
[PASS] BLE scan parameter request sent
[PASS] BLE scan started

[ADV] #1 addr=... rssi=... name=...
...

BLE SCAN PHYSICAL PASS CANDIDATE
Controller + Bluedroid + active scan + advertisement receive passed.
```

Then the sketch should continue printing:

```text
[ALIVE] uptime=... bleInit=OK scanStarted=YES scanDone=YES advReports=... freeHeap=... psram=... freePsram=...
```

## PASS boundary

```text
BLE SCAN PASS CANDIDATE:
  BT controller initializes;
  controller enables in BLE mode;
  Bluedroid initializes and enables;
  local BLE address is available;
  BLE scan starts;
  one or more advertisement reports are received;
  scan completes;
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

This test validates only BLE controller/host initialization and advertisement receive.

It does not validate pairing, GATT, HID, provisioning or Wi-Fi/BLE coexistence.
