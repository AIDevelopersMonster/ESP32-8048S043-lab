# ESP32-8048S043 Lab Arduino IDE board kit

Status: `DESIGN NOTES / LOCAL BOARD PROFILE EXPERIMENT / AUTOMATIC INSTALLER REMOVED`.

This folder stages a future Arduino IDE board profile:

```text
ESP32-8048S043 Lab / ESP32-S3 N16R8 / RGB 800x480 / GT911
```

## Important warning

The previous automatic `install-windows.ps1` approach has been removed.

Reason:

```text
Arduino IDE 2.x did not reliably keep the original esp32:esp32:esp32s3 board visible after the generated boards.local.txt experiment.
```

Current safe workflow:

```text
Tools -> Board -> esp32 -> ESP32S3 Dev Module
Flash Size       : 16MB (128Mb)
Flash Mode       : QIO 80MHz
PSRAM            : OPI PSRAM
Partition Scheme : 16M Flash (3MB APP/9.9MB FATFS)
Upload Speed     : 921600, fallback 460800
Serial Monitor   : 115200 baud
```

## Why the idea is still valid

For classic Arduino AVR / ATmega328P boards it was common to clone an existing `boards.txt` entry, rename it and change frequency, bootloader or upload properties. The same conceptual idea can work here too:

```text
copy the installed Espressif esp32s3 board entry;
rename the copy to esp32_8048s043_lab;
keep the original esp32s3 entry untouched;
add the project variant;
add the project partition CSV;
add ESP32_8048S043 compile macros.
```

The problem is not the concept. The problem is doing it automatically against a complex installed ESP32 core without a proven version-specific patch.

## Files intentionally kept

```text
uninstall-windows.ps1
partitions/esp32_8048s043_16m_lab.csv
variants/esp32_8048s043_lab/pins_arduino.h
```

`uninstall-windows.ps1` is kept only as a cleanup tool for earlier experiments. It is not part of a supported installation workflow.

## Files intentionally removed

```text
install-windows.ps1
```

## Future safe options

### Option A: manual lab patch

Create a documented patch for a specific Arduino-ESP32 core version:

```text
backup boards.txt;
copy the esp32s3 block inside boards.txt;
rename the copied block;
copy the variant folder;
copy the partition CSV;
validate 01_BoardInfo first.
```

This is close to the old AVR/328P workflow, but must be version-controlled and backed up.

### Option B: separate Arduino hardware platform

Create a proper sketchbook hardware package:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32/
  boards.txt
  platform.txt
  variants/
  tools/
  partitions/
```

This is closer to "drop a board folder and restart IDE", but it requires a complete and compatible ESP32 platform definition.

### Option C: Board Manager package

Create a real Boards Manager package index later:

```text
package_aidevelopersmonster_esp32_index.json
```

This is the cleanest distribution path, but should come only after the board profile is validated locally.

## Boundary

The variant file is allowed to describe standard Arduino-level aliases such as:

```text
SDA / SCL;
SS / MOSI / MISO / SCK;
LED_BUILTIN if needed later;
project macros visible to sketches/libraries.
```

The complex RGB display bus, GT911 registers and backlight behavior remain in the `ESP32_8048S043` library.

## Promotion rule

A custom board profile can be promoted only after:

```text
Arduino IDE shows the custom board target;
01_BoardInfo compiles and uploads with that target;
PSRAM is reported as 8 MB;
02_DisplayRGBTest still passes;
03_TouchGT911Test still passes;
05_TestConsole still runs;
the original ESP32S3 Dev Module remains visible and usable.
```
