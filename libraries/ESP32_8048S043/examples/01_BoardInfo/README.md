# 01_BoardInfo

Status: `SOURCE IMPLEMENTED / PARTIAL PHYSICAL PASS`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

Confirming video / visual board check:

```text
https://youtube.com/shorts/XVaWqrtXHE4
```

This is the first Arduino IDE smoke test for the ESP32-8048S043 / ESP32-8048S043C-I board family.

It does not initialize the RGB display, GT911 touch, SD card or LVGL. It confirms that the board can be flashed and prints an extended runtime passport for the ESP32-S3, flash, PSRAM, sketch, partitions and source-backed board pin profile.

## Current Sample A result

Observed result from the first extended `01_BoardInfo` run:

```text
Upload / serial monitor : PASS
Flash size              : PASS, 16777216 bytes / 16 MB
Flash mode / speed      : QIO / 80 MHz
Running partition       : app0, address 0x010000, size 3145728
Runtime stability       : PASS, ALIVE lines observed through at least 100000 ms
PSRAM                   : FAIL / RETEST, Arduino runtime reported 0 bytes with QSPI PSRAM selected
Overall 01_BoardInfo    : PARTIAL PASS
```

Commit-safe evidence:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
```

## Why this test comes first

Run this example before display or touch tests because it verifies the basic programming path, memory profile and Arduino runtime configuration.

Expected board family:

```text
ESP32-S3
16 MB flash
8 MB PSRAM expected/source-backed
CH340C USB-UART bridge on the capacitive-touch Jingcai/TinyTronics variant
```

## Arduino IDE setup used for Sample A first run

Install the ESP32 board package:

```text
Boards Manager -> esp32 by Espressif Systems
```

The following table mirrors the Arduino IDE Tools menu used for the first Sample A run.

| Arduino IDE menu | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Port | COM12 / CH340 USB-SERIAL port of the board |
| USB CDC On Boot | Disabled |
| CPU Frequency | 240MHz (WiFi) |
| Core Debug Level | None |
| USB DFU On Boot | Disabled |
| Erase All Flash Before Sketch Upload | Disabled |
| Events Run On | Core 1 |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| JTAG Adapter | Disabled |
| Arduino Runs On | Core 1 |
| USB Firmware MSC On Boot | Disabled |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | QSPI PSRAM — produced PSRAM size 0 in the first run |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |
| Zigbee Mode | Disabled |
| Serial Monitor | 115200 baud |

## Required PSRAM retest

The first run detected flash and stayed stable, but PSRAM was not detected:

```text
PSRAM size              : 0 bytes / 0 KB / 0 MB
Free PSRAM              : 0 bytes / 0 KB / 0 MB
Max alloc PSRAM         : 0 bytes / 0 KB / 0 MB
```

For the next run, keep all settings the same except PSRAM:

```text
Change PSRAM from QSPI PSRAM to OPI PSRAM / Enabled
```

Menu wording depends on the installed ESP32 Arduino core. The desired result is:

```text
PSRAM size              : about 8388608 bytes / 8192 KB / 8 MB
Free PSRAM              : non-zero
Max alloc PSRAM         : non-zero
```

Notes:

```text
COM12 is the local port observed on Sample A. Choose your actual CH340 port.
If upload is unstable at 921600, retry Upload Speed = 460800 before changing other settings.
For factory flash readback/dump workflows, 460800 was more reliable than 921600.
```

## Running the test

Open:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/01_BoardInfo.ino
```

Upload the sketch, then open Serial Monitor at:

```text
115200 baud
```

## Extended report sections

The sketch now prints a larger runtime passport:

```text
[BUILD / RUNTIME]
  sketch build date/time
  ESP-IDF SDK version
  reset reason

[CHIP]
  chip model
  chip revision
  chip cores
  CPU frequency
  eFuse MAC raw value and bytes

[MEMORY]
  heap size
  free heap
  min free heap
  max alloc heap
  PSRAM size
  free PSRAM
  max alloc PSRAM
  PSRAM status and warning if not detected

[FLASH / SKETCH]
  flash chip size
  flash chip speed
  flash chip mode
  sketch size
  free sketch space
  sketch MD5

[PARTITIONS]
  running app partition
  boot app partition

[SOURCE-BACKED DISPLAY / TOUCH / SD PROFILE]
  RGB 800x480 pin map
  GT911 SDA/SCL/RST/INT and 0x5D/0x14 addresses
  microSD CS/MOSI/CLK/MISO pin map

[EXPECTED SAMPLE A BASELINE]
  expected chip, flash, PSRAM, USB bridge, display and touch status
```

## Expected output highlights

The log should include:

```text
ESP32-8048S043 Lab / 01_BoardInfo
First Arduino IDE smoke test
Author        : Alex Malachevsky
GitHub        : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
Evidence video: https://youtube.com/shorts/XVaWqrtXHE4
Board                                : ESP32S3 Dev Module
Flash Mode                           : QIO 80MHz
Flash Size                           : 16MB (128Mb)
Partition Scheme                     : 16M Flash (3MB APP/9.9MB FATFS)
Upload Mode                          : UART0 / Hardware CDC
Upload Speed                         : 921600
USB Mode                             : Hardware CDC and JTAG
```

The board information block should report approximately after the correct PSRAM mode is selected:

```text
Chip model              : ESP32-S3
Flash chip size         : 16777216 bytes / 16384 KB / 16 MB
PSRAM size              : 8388608 bytes / 8192 KB / 8 MB
PSRAM status            : DETECTED
Display                 : 800x480 RGB/DPI
GT911 touch             : SDA=19 SCL=20 RST=38 INT=18 ADDR=0x5D/0x14
microSD SPI             : CS=10 MOSI=11 CLK=12 MISO=13
```

If PSRAM still reports zero, the sketch will print:

```text
PSRAM status            : NOT DETECTED
PSRAM warning           : expected about 8 MB for N16R8-class board
PSRAM next action       : retest Arduino IDE setting OPI PSRAM / Enabled
```

Then the sketch should continue printing:

```text
ALIVE uptime=... freeHeap=... freePsram=...
```

every five seconds.

## What to paste back into the project chat

For validation, copy the full Serial Monitor output from the first boot, starting at:

```text
ESP32-8048S043 Lab / 01_BoardInfo
```

and ending after at least two `ALIVE` lines.

## PASS condition

Mark this test as full PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
chip model is ESP32-S3;
flash is reported as about 16 MB;
PSRAM is reported as about 8 MB;
running/boot partition data prints without crash;
ALIVE messages continue without resets.
```

Current Sample A status remains `PARTIAL PASS` until PSRAM is reported as about 8 MB by the Arduino runtime.

## Boundary

This test is only a BoardInfo / runtime smoke test. It does not prove:

```text
RGB display output;
GT911 touch operation;
SD card operation;
Wi-Fi/BLE operation;
backlight PWM behavior;
final BSP pinout.
```

Those are separate validation stages.
