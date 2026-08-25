# 08_SDCardTest

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

This example validates the source-backed microSD / TF SPI pin map on ESP32-8048S043 boards.

It is intentionally read-only. It does not write, format, delete or rename files.

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
```

`08_SDCardTest` validates the next hardware subsystem before SD-backed logs, file upload, Widget Runtime storage or offline asset loading.

## Pin map under test

```text
CS   : GPIO10
MOSI : GPIO11
CLK  : GPIO12
MISO : GPIO13
```

These pins come from:

```text
libraries/ESP32_8048S043/src/ESP32_8048S043_Pins.h
```

## What it checks

```text
custom board profile remains alive after upload;
SD SPI pins are configured from ESP32_8048S043_Pins.h;
SD card can be mounted through Arduino SD.h;
card type and size can be read;
filesystem total/used/free can be reported;
root directory can be listed;
optional first-file read-only HEX preview can be printed;
ALIVE lines continue after the test.
```

## What it does not check

```text
filesystem write safety;
formatting;
long-duration SD stress;
concurrent SD + RGB/LVGL rendering;
SD-backed Web upload;
Widget Runtime storage;
serving files from SD over HTTP.
```

## Arduino IDE setup

Use the same profile that passed `01_BoardInfo`:

```text
Board             : ESP32-8048S043 Lab N16R8 FIXED or ESP32S3 Dev Module fallback
Flash Size        : 16MB (128Mb)
Flash Mode        : QIO 80MHz
Partition Scheme  : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM             : OPI PSRAM
Serial Monitor    : 115200 baud
```

## Running the test

Open:

```text
File -> Examples -> ESP32_8048S043 -> 08_SDCardTest
```

Insert a microSD card before boot or before reset. FAT/FAT32/exFAT support depends on the Arduino-ESP32 SD stack and card formatting.

## Expected serial output

A good run should look like:

```text
ESP32-8048S043 Lab / 08_SDCardTest
Read-only microSD / TF SPI validation
Pin map: CS=10 MOSI=11 CLK=12 MISO=13
Mode   : read-only, no write/format/delete/rename operations

[MOUNT] Trying SD.begin(... freq=10000000 Hz)
[PASS] SD mounted

[CARD]
Card type          : SDHC/SDXC
Mounted frequency  : ... Hz
Card size          : ...
Filesystem total   : ...
Filesystem used    : ...
Filesystem free    : ...

[LIST] Directory: /
  FILE ...
  DIR  ...

SD CARD READ-ONLY PHYSICAL PASS CANDIDATE
Mount + metadata + root listing completed. No writes performed.
```

The sketch retries several SPI frequencies:

```text
10 MHz -> 4 MHz -> 1 MHz -> 400 kHz
```

The first frequency that mounts successfully is recorded.

## PASS boundary

```text
READ-ONLY SD PASS CANDIDATE:
  card mounts;
  card type and size are reported;
  filesystem total/used/free are reported;
  root directory opens and lists;
  no write operation is performed;
  no reboot, brownout or crash during observation.
```

## Failure interpretation

If mounting fails:

```text
check card insertion;
try another microSD card;
try a simple FAT32-formatted card;
confirm the board revision really uses CS=10 MOSI=11 CLK=12 MISO=13;
re-run 01_BoardInfo to confirm the board profile is still sane.
```

If metadata is visible but listing fails:

```text
card electrical path is likely alive;
filesystem format or root directory handling needs separate investigation.
```

## Boundary

Do not promote this test to PHYSICAL PASS until a named specimen log or video exists.
