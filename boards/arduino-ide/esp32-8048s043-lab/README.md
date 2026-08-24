# ESP32-8048S043 Lab Arduino IDE board kit

Status: `EXPERIMENTAL SKELETON / NOT A SUPPORTED BOARD PACKAGE YET`.

This folder is the staging area for a future custom Arduino board profile:

```text
ESP32-8048S043 Lab / ESP32-S3 N16R8 / RGB 800x480 / GT911
```

It is intentionally not promoted as a supported Boards Manager package yet.

## Current supported workflow

Until this kit is physically validated, use the generic Espressif target:

```text
Board         : ESP32S3 Dev Module
Flash Size    : 16MB (128Mb)
Flash Mode    : QIO 80MHz
PSRAM         : OPI PSRAM
Partition     : 16M Flash (3MB APP/9.9MB FATFS)
Upload        : UART0 / CH340 workflow
Serial Monitor: 115200 baud
```

## What this kit will eventually provide

```text
custom board name;
16 MB flash defaults;
8 MB OPI PSRAM defaults;
project partition CSV;
compile-time ESP32_8048S043 macros;
variant-level standard bus aliases;
clean separation from the ESP32_8048S043 runtime library.
```

## Included files

```text
partitions/esp32_8048s043_16m_lab.csv
variants/esp32_8048s043_lab/pins_arduino.h
```

## Boundary

The variant file is allowed to describe standard Arduino-level aliases such as:

```text
SDA / SCL;
SS / MOSI / MISO / SCK;
LED_BUILTIN if needed later.
```

The complex RGB display bus, GT911 registers and backlight behavior remain in the `ESP32_8048S043` library.

## Not yet included

```text
full boards.txt entry;
platform.txt override;
package index JSON;
installer;
compile/upload evidence for the custom board target.
```

## Promotion rule

This kit can be promoted from skeleton to usable experimental profile only after:

```text
Arduino IDE shows the custom board target;
01_BoardInfo compiles and uploads with that target;
PSRAM is reported as 8 MB;
02_DisplayRGBTest still passes;
03_TouchGT911Test still passes;
05_TestConsole still runs.
```
