# Sample A / Arduino / 01_BoardInfo / ESP32S3 Dev Module OPI reference

Status: `BOARDINFO REFERENCE PROFILE PASS CANDIDATE / SAMPLE A`.

Date: `2026-08-25`

Evidence source:

```text
Serial Monitor runtime log supplied by operator after the improved 01_BoardInfo diagnostic patch.
```

## Test target

```text
Example      : 01_BoardInfo
Purpose      : Arduino IDE smoke test + board/profile diagnostic
Board target : ESP32S3 Dev Module
Profile role : known-good reference profile for Sample A
Flash        : 16MB (128Mb)
Flash mode   : QIO 80MHz
Partition    : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM        : OPI PSRAM
```

## Compile-time build profile evidence

The improved diagnostic block reported:

```text
ARDUINO_BOARD               : "ESP32S3_DEV"
ARDUINO_VARIANT             : "esp32s3"
ARDUINO_ARCH_ESP32          : defined
Arduino-ESP32 version       : 3.3.11
CONFIG_IDF_TARGET           : "esp32s3"
CONFIG_IDF_TARGET_ESP32S3   : 1
ARDUINO_USB_MODE            : 1
ARDUINO_USB_CDC_ON_BOOT     : 0
BOARD_HAS_PSRAM             : defined
CONFIG_SPIRAM               : 1
CONFIG_SPIRAM_BOOT_INIT     : 1
CONFIG_SPIRAM_USE_MALLOC    : 1
CONFIG_SPIRAM_MODE_OCT      : 1
CONFIG_SPIRAM_MODE_QUAD     : not defined
```

This is the reference macro set expected for ESP32-S3 + OPI PSRAM on Sample A.

## Runtime identity evidence

```text
Chip model                  : ESP32-S3
Chip revision               : 2
Chip cores                  : 2
CPU frequency               : 240 MHz
eFuse MAC raw               : 0x3C696CE6FC84
eFuse MAC bytes             : 3C:69:6C:E6:FC:84
```

## Runtime memory evidence

```text
Heap size                   : 392888 bytes / 383 KB / 0 MB
Free heap                   : 356380 bytes / 348 KB / 0 MB
Min free heap               : 351232 bytes / 343 KB / 0 MB
Max alloc heap              : 294900 bytes / 287 KB / 0 MB
PSRAM size                  : 8388608 bytes / 8192 KB / 8 MB
Free PSRAM                  : 8384788 bytes / 8188 KB / 7 MB
Max alloc PSRAM             : 8257524 bytes / 8063 KB / 7 MB
psramFound()                : true
PSRAM runtime status        : DETECTED
IDF SPIRAM total            : 8388608 bytes / 8192 KB / 8 MB
IDF SPIRAM free             : 8384788 bytes / 8188 KB / 7 MB
IDF SPIRAM largest block    : 8257524 bytes / 8063 KB / 7 MB
```

## Flash and partition evidence

```text
Flash chip size             : 16777216 bytes / 16384 KB / 16 MB
Flash chip speed            : 80000000 Hz
Flash chip mode             : QIO (0)
Sketch size                 : 292400 bytes / 285 KB / 0 MB
Free sketch space           : 3145728 bytes / 3072 KB / 3 MB
Running app                 : label=app0 type=0x00 subtype=0x10 address=0x010000 size=3145728
Boot app                    : label=app0 type=0x00 subtype=0x10 address=0x010000 size=3145728
```

## Acceptance check evidence

```text
Chip is ESP32-S3            : PASS
Flash is about 16 MB        : PASS
PSRAM is about 8 MB         : PASS
App partition about 3 MB    : PASS
Overall BoardInfo           : PASS CANDIDATE
```

## Stability evidence

The operator reported continued ALIVE output up to at least 840 seconds and included repeated ALIVE lines:

```text
ALIVE uptime=5000 ms freeHeap=355992 psramSize=8388608 freePsram=8384788
ALIVE uptime=130000 ms freeHeap=355992 psramSize=8388608 freePsram=8384788
ALIVE uptime=840000 ms freeHeap=355992 psramSize=8388608 freePsram=8384788
```

## Result

```text
Upload / serial monitor          : PASS
Compile-time target              : PASS, ESP32S3_DEV / esp32s3
ESP32-S3 identity                : PASS
16 MB flash                      : PASS
8 MB OPI PSRAM                   : PASS
3 MB app0 partition              : PASS
Longer ALIVE stability           : PASS candidate, >=840 seconds reported
Overall 01_BoardInfo reference   : BOARDINFO REFERENCE PROFILE PASS CANDIDATE / SAMPLE A
```

## Use of this evidence

This file is the known-good ESP32S3 Dev Module + OPI PSRAM reference point.

Next comparison target:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

The custom local board profile should produce the same essential runtime truth:

```text
CONFIG_IDF_TARGET_ESP32S3   : 1
BOARD_HAS_PSRAM             : defined
CONFIG_SPIRAM               : 1
CONFIG_SPIRAM_MODE_OCT      : 1
CONFIG_SPIRAM_MODE_QUAD     : not defined
PSRAM size                  : 8388608 bytes
psramFound()                : true
```

## Boundary

This evidence validates the BoardInfo/reference profile only. It does not replace the dedicated RGB display, GT911 touch, backlight, Wi-Fi, WebServer, SD or LVGL tests.
