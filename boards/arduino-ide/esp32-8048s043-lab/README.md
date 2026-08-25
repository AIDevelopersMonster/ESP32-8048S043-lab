# ESP32-8048S043 Lab Arduino IDE board kit

Status: `LOCAL SKETCHBOOK HARDWARE PROFILE PASS CANDIDATE / SAMPLE A / BOARD MANAGER OPEN`.

This folder stages the Arduino IDE board-profile work for:

```text
ESP32-8048S043 Lab / ESP32-S3 N16R8 / RGB 800x480 / GT911
```

## Current validated local profile

The safe local approach is a separate Arduino sketchbook hardware platform, not a modification of the installed Espressif package:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

Working board profile:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Validated by `01_BoardInfo` on Sample A:

```text
Chip                    : ESP32-S3 rev 2
Flash                   : 16777216 bytes / 16 MB / QIO 80 MHz
PSRAM                   : 8388608 bytes / 8 MB / OPI PSRAM
Running app partition   : app0, address 0x010000, size 3145728
Upload path             : COM12 / CH340 / UART0 workflow
Runtime stability       : ALIVE lines observed with freePsram available
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
```

## Working Arduino IDE menu profile

```text
Board             : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
FQBN              : AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
Flash Size        : 16MB (128Mb)
Flash Mode        : QIO 80MHz
PSRAM             : OPI PSRAM
Partition Scheme  : 16M Flash (3MB APP/9.9MB FATFS)
Upload Mode       : UART0 / Hardware CDC
Upload Speed      : 921600, fallback 460800
Serial Monitor    : 115200 baud
```

## Obsolete local profile

The earlier first-stage local board ID can be removed from a local experimental `boards.txt`:

```text
esp32_8048s043_lab
```

Keep the working profile:

```text
esp32_8048s043_lab_n16r8
```

## Important warning about the removed installer

The previous automatic `install-windows.ps1` approach that wrote into `boards.local.txt` has been removed.

Reason:

```text
Arduino IDE 2.x did not reliably keep the original esp32:esp32:esp32s3 board visible after the generated boards.local.txt experiment.
```

Do not use `boards.local.txt` for this project until a clean reproducible package is built and validated.

## Safe fallback workflow

The generic Espressif target remains the safe fallback:

```text
Tools -> Board -> esp32 -> ESP32S3 Dev Module
Flash Size       : 16MB (128Mb)
Flash Mode       : QIO 80MHz
PSRAM            : OPI PSRAM
Partition Scheme : 16M Flash (3MB APP/9.9MB FATFS)
Upload Speed     : 921600, fallback 460800
Serial Monitor   : 115200 baud
```

## Why the idea is valid

For classic Arduino AVR / ATmega328P boards it was common to clone an existing `boards.txt` entry, rename it and change frequency, bootloader or upload properties. The same conceptual idea works here too, but the ESP32 platform is more complex.

The validated direction is:

```text
copy the installed Espressif esp32s3 board entry into a separate sketchbook hardware platform;
rename the copy to esp32_8048s043_lab_n16r8;
keep the original Espressif package untouched;
use a project variant;
use 16MB flash / OPI PSRAM / 3MB app partition;
validate 01_BoardInfo before running higher-level examples.
```

## Files intentionally kept

```text
uninstall-windows.ps1
partitions/esp32_8048s043_16m_lab.csv
variants/esp32_8048s043_lab/pins_arduino.h
```

`uninstall-windows.ps1` is kept only as a cleanup tool for earlier experiments. It is not part of the validated local sketchbook platform workflow.

## Files intentionally removed

```text
install-windows.ps1
```

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

The local board profile can be promoted from PASS candidate to project default only after:

```text
01_BoardInfo passes under the custom target;
02_DisplayRGBTest still passes under the custom target;
03_TouchGT911Test still passes under the custom target;
05_TestConsole still runs under the custom target;
the original ESP32S3 Dev Module remains visible and usable.
```

A future supported Board Manager package must be created separately and must not depend on editing the locally installed Espressif core.
