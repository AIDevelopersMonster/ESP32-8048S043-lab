# 01_BoardInfo

Status: `PHYSICAL PASS / SAMPLE A / PROFILE DIAGNOSTIC V2`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

Confirming videos:

```text
Factory LVGL display/touch visual check:
https://youtube.com/shorts/XVaWqrtXHE4

Arduino 01_BoardInfo first library test:
https://youtube.com/shorts/wELRdRWqlnw
```

This is the first Arduino IDE smoke test and profile diagnostic for the ESP32-8048S043 / ESP32-8048S043C-I board family.

It does not initialize the RGB display, GT911 touch, SD card, Wi-Fi, WebServer or LVGL. It confirms that the board can be flashed and prints an extended runtime passport for the ESP32-S3, flash, PSRAM, sketch, partitions and source-backed board pin profile.

## Why V2 exists

Arduino IDE Tools menu text is not directly readable from a sketch. Earlier documentation-style strings could be confused with real runtime state.

V2 changes the rule:

```text
Do not trust a hard-coded Tools table.
Trust compile-time macros + ESP/IDF runtime values.
```

The sketch now prints:

```text
[ARDUINO BUILD PROFILE]
  ARDUINO_BOARD
  ARDUINO_VARIANT
  ARDUINO_ARCH_ESP32
  Arduino-ESP32 version macros, when exposed
  CONFIG_IDF_TARGET / CONFIG_IDF_TARGET_ESP32S3
  ARDUINO_USB_MODE
  ARDUINO_USB_CDC_ON_BOOT
  BOARD_HAS_PSRAM
  CONFIG_SPIRAM / BOOT_INIT / USE_MALLOC
  CONFIG_SPIRAM_MODE_OCT / CONFIG_SPIRAM_MODE_QUAD

[MEMORY]
  ESP.getPsramSize()
  ESP.getFreePsram()
  ESP.getMaxAllocPsram()
  psramFound()
  IDF heap_caps_get_total_size(MALLOC_CAP_SPIRAM)
  IDF heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
  IDF largest SPIRAM block

[ACCEPTANCE CHECK]
  Chip is ESP32-S3
  Flash is about 16 MB
  PSRAM is about 8 MB
  App partition about 3 MB
  Overall BoardInfo
```

## Current Sample A known-good result

A known-good ESP32S3 Dev Module + OPI PSRAM reference run has shown:

```text
ARDUINO_BOARD               : "ESP32S3_DEV"
ARDUINO_VARIANT             : "esp32s3"
CONFIG_IDF_TARGET_ESP32S3   : 1
BOARD_HAS_PSRAM             : defined
CONFIG_SPIRAM               : 1
CONFIG_SPIRAM_MODE_OCT      : 1
CONFIG_SPIRAM_MODE_QUAD     : not defined

Chip                        : ESP32-S3 rev 2, 2 cores, 240 MHz
Flash size                  : 16777216 bytes / 16 MB
Flash mode / speed          : QIO / 80 MHz
PSRAM                       : 8388608 bytes / 8 MB
psramFound()                : true
IDF SPIRAM total            : 8388608 bytes / 8 MB
Running partition           : app0, address 0x010000, size 3145728
Runtime stability           : ALIVE lines observed with freePsram available up to at least 840 seconds
```

Commit-safe evidence:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
evidence/specimens/sample-a/arduino/01-boardinfo-esp32s3-devmodule-opi-reference-20260825.md
```

## Debug workflow for PSRAM/profile parity

Use this test before moving to LVGL/Web/OTA memory-heavy work.

Run the same `01_BoardInfo` sketch under two profiles:

```text
A. ESP32S3 Dev Module
   Flash Size       : 16MB (128Mb)
   Flash Mode       : QIO 80MHz
   Partition Scheme : 16M Flash (3MB APP/9.9MB FATFS)
   PSRAM            : OPI PSRAM

B. ESP32-8048S043 Lab N16R8 FIXED
   Flash Size       : 16MB (128Mb)
   Flash Mode       : QIO 80MHz
   Partition Scheme : 16M Flash (3MB APP/9.9MB FATFS)
   PSRAM            : OPI PSRAM
```

Expected result for both:

```text
PSRAM size              : 8388608 bytes / 8192 KB / 8 MB
Free PSRAM              : greater than 0
psramFound()            : true
IDF SPIRAM total        : about 8388608 bytes
Overall BoardInfo       : PASS CANDIDATE
ALIVE ... psramSize=8388608 freePsram=...
```

If the custom profile differs from the classic ESP32S3 Dev Module profile, compare the `[ARDUINO BUILD PROFILE]` blocks line by line.

## Running the test

Open:

```text
File -> Examples -> ESP32_8048S043 -> 01_BoardInfo
```

or directly:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/01_BoardInfo.ino
```

Upload the sketch, then open Serial Monitor at:

```text
115200 baud
```

## Expected output highlights

The header should say:

```text
ESP32-8048S043 Lab / 01_BoardInfo
First Arduino IDE smoke test + board/profile diagnostic
This sketch does not trust a hard-coded Tools menu table.
```

The runtime block should include:

```text
[ARDUINO BUILD PROFILE]
ARDUINO_BOARD
ARDUINO_VARIANT
BOARD_HAS_PSRAM
CONFIG_SPIRAM_MODE_OCT
CONFIG_SPIRAM_MODE_QUAD
```

The board information block should report approximately:

```text
Chip model                  : ESP32-S3
Flash chip size             : 16777216 bytes / 16384 KB / 16 MB
PSRAM size                  : 8388608 bytes / 8192 KB / 8 MB
Free PSRAM                  : greater than 0
psramFound()                : true
IDF SPIRAM total            : about 8388608 bytes
Display                     : 800x480 RGB/DPI
GT911 touch                 : SDA=19 SCL=20 RST=38 INT=18 ADDR=0x5D/0x14
microSD SPI                 : CS=10 MOSI=11 CLK=12 MISO=13
Overall BoardInfo           : PASS CANDIDATE
```

Then the sketch should continue printing:

```text
ALIVE uptime=... freeHeap=... psramSize=... freePsram=...
```

every five seconds.

## PASS condition

Mark this test as full PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
chip model is ESP32-S3;
flash is reported as about 16 MB;
PSRAM is reported as about 8 MB;
psramFound() is true;
IDF SPIRAM total is about 8 MB;
running/boot partition data prints without crash;
ALIVE messages continue without resets.
```

## Failure interpretation

If PSRAM is `0 B`:

```text
The board may still be fine.
First assume the selected Arduino build profile or Tools menu is not enabling OPI PSRAM.
Compare [ARDUINO BUILD PROFILE] between ESP32S3 Dev Module and the local ESP32-8048S043 board profile.
Do not continue to LVGL/Web/OTA memory-heavy tests until PSRAM is detected.
```

If flash is `4 MB` or the app partition does not match about `3 MB`:

```text
The selected Tools Flash Size / Partition Scheme is wrong.
Use 16MB flash and 16M Flash (3MB APP/9.9MB FATFS).
```

## Boundary

This test is only a BoardInfo / runtime smoke test. It does not prove:

```text
RGB display output;
GT911 touch operation;
SD card operation;
Wi-Fi/BLE operation;
backlight PWM behavior;
Web server behavior;
LVGL integration;
final BSP pinout.
```

Those are separate validation stages.
