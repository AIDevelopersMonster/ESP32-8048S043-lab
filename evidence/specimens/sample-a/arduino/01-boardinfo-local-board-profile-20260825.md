# Sample A / Arduino local board profile / 01_BoardInfo

Status: `LOCAL ARDUINO HARDWARE PROFILE PASS CANDIDATE / SAMPLE A`.

Date: `2026-08-25`

## Purpose

Validate the local sketchbook Arduino hardware platform approach for ESP32-8048S043 Lab without modifying the installed Espressif Arduino-ESP32 core.

Working local platform path:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

Working board profile:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

## Arduino IDE menu profile

Observed working values:

```text
USB CDC On Boot      : Disabled
CPU Frequency        : 240MHz (WiFi)
Core Debug Level     : None
USB DFU On Boot      : Disabled
Erase All Flash      : Disabled
Events Run On        : Core 1
Flash Mode           : QIO 80MHz
Flash Size           : 16MB (128Mb)
JTAG Adapter         : Disabled
Arduino Runs On      : Core 1
USB Firmware MSC     : Disabled
Partition Scheme     : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM                : OPI PSRAM
Upload Mode          : UART0 / Hardware CDC
Upload Speed         : 921600
USB Mode             : Hardware CDC and JTAG
Zigbee Mode          : Disabled
Serial Monitor       : 115200 baud
```

## Runtime evidence

`01_BoardInfo` compiled, uploaded and ran from Arduino IDE using the custom local board profile.

Key runtime output:

```text
Sketch build            : Aug 25 2026 06:49:15
ESP-IDF SDK             : v5.5.5
Reset reason            : 1 (POWERON)

Chip model              : ESP32-S3
Chip revision           : 2
Chip cores              : 2
CPU frequency           : 240 MHz

eFuse MAC bytes         : 3C:69:6C:E6:FC:84

PSRAM size              : 8388608 bytes / 8192 KB / 8 MB
Free PSRAM              : 8384788 bytes / 8188 KB / 7 MB
Max alloc PSRAM         : 8257524 bytes / 8063 KB / 7 MB

Flash chip size         : 16777216 bytes / 16384 KB / 16 MB
Flash chip speed        : 80000000 Hz
Flash chip mode         : QIO (0)

Free sketch space       : 3145728 bytes / 3072 KB / 3 MB
Running app             : label=app0 type=0x00 subtype=0x10 address=0x010000 size=3145728
Boot app                : label=app0 type=0x00 subtype=0x10 address=0x010000 size=3145728

ALIVE uptime=5000 ms freeHeap=356272 freePsram=8384788
ALIVE uptime=10000 ms freeHeap=356272 freePsram=8384788
```

## Result

```text
Custom local board target visible in Arduino IDE : PASS
Compile/upload through COM12 / CH340             : PASS
Chip identity                                    : PASS, ESP32-S3 rev 2
Flash                                            : PASS, 16 MB / QIO 80 MHz
PSRAM                                            : PASS, 8 MB / OPI PSRAM
Partition profile                               : PASS, app0 size 3145728 bytes
Runtime stability                                : PASS candidate, ALIVE observed
```

## Boundary

This evidence validates the local Arduino hardware profile for `01_BoardInfo` only. It does not by itself revalidate RGB display, GT911 touch, backlight, SD, LVGL or Web/OTA examples under the custom board target.

The earlier local STAGE1 board profile can be removed from local `boards.txt`; the working profile is `esp32_8048s043_lab_n16r8`.
