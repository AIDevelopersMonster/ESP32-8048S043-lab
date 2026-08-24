# ESP32-8048S043 Lab Arduino IDE board kit

Status: `EXPERIMENTAL LOCAL PROFILE / CLONED FROM ESP32S3 DEV MODULE / PHYSICAL VALIDATION OPEN`.

This folder stages a local Arduino IDE board profile:

```text
ESP32-8048S043 Lab / ESP32-S3 N16R8 / RGB 800x480 / GT911
```

The practical strategy is simple:

```text
copy the installed Espressif `esp32s3` board entry;
rename the copy to `esp32_8048s043_lab`;
keep the original `esp32s3` entry untouched;
write the new entry to `boards.local.txt`;
copy the project variant and partition CSV into the installed Arduino-ESP32 core.
```

This keeps the original Espressif `boards.txt` unchanged and gives Arduino IDE a second selectable board profile.

## Install on Windows

Close Arduino IDE completely before running the installer.

From the repository root:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\ESP32-8048S043-lab
powershell -ExecutionPolicy Bypass -File .\boards\arduino-ide\esp32-8048s043-lab\install-windows.ps1
```

The installer searches the installed Espressif Arduino-ESP32 core under:

```text
%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\<version>
```

It then:

```text
reads the original boards.txt;
extracts the `esp32s3` board block;
clones it as `esp32_8048s043_lab`;
changes the visible board name;
sets build.variant=esp32_8048s043_lab;
sets build.board=ESP32_8048S043_LAB;
adds ESP32_8048S043 compile macros;
adds a project partition menu option;
copies variants/esp32_8048s043_lab/pins_arduino.h;
copies partitions/esp32_8048s043_16m_lab.csv;
writes the generated board block to boards.local.txt.
```

The original `boards.txt` is not modified.

## Manual core path

If multiple ESP32 core versions are installed and the automatic selection picks the wrong one, pass the core path explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File .\boards\arduino-ide\esp32-8048s043-lab\install-windows.ps1 -CorePath "$env:LOCALAPPDATA\Arduino15\packages\esp32\hardware\esp32\3.3.0"
```

Replace `3.3.0` with the version actually installed on the machine.

## Arduino IDE selection

Restart Arduino IDE completely after installation.

Select the board:

```text
Tools -> Board -> esp32 -> ESP32-8048S043 Lab (ESP32-S3 N16R8 RGB800x480 GT911)
```

Recommended Tools menu values for first validation:

```text
Flash Size       : 16MB (128Mb)
Flash Mode       : QIO 80MHz
PSRAM            : OPI PSRAM
Partition Scheme : ESP32-8048S043 Lab 16M (3MB APP/9.9MB SPIFFS)
Upload Speed     : 921600, fallback 460800
Serial Monitor   : 115200 baud
```

First validation example:

```text
File -> Examples -> ESP32_8048S043 -> 01_BoardInfo
```

Expected result before promotion:

```text
custom board appears in Arduino IDE;
sketch compiles;
upload works;
BoardInfo still reports ESP32-S3;
Flash reports about 16 MB;
PSRAM reports about 8 MB;
ALIVE lines continue.
```

## Uninstall on Windows

Close Arduino IDE completely.

```powershell
cd C:\Users\CHUWI\Documents\GitHub\ESP32-8048S043-lab
powershell -ExecutionPolicy Bypass -File .\boards\arduino-ide\esp32-8048s043-lab\uninstall-windows.ps1
```

The uninstaller:

```text
backs up boards.local.txt;
removes only the marked ESP32-8048S043 Lab block;
removes the copied esp32_8048s043_lab variant;
removes the copied esp32_8048s043_16m_lab partition CSV.
```

It does not modify the original `boards.txt`.

## Included files

```text
install-windows.ps1
uninstall-windows.ps1
partitions/esp32_8048s043_16m_lab.csv
variants/esp32_8048s043_lab/pins_arduino.h
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

This profile can be promoted from experimental local profile to validated project profile only after:

```text
Arduino IDE shows the custom board target;
01_BoardInfo compiles and uploads with that target;
PSRAM is reported as 8 MB;
02_DisplayRGBTest still passes;
03_TouchGT911Test still passes;
05_TestConsole still runs.
```

A future supported Board Manager package must be created separately and must not depend on editing the locally installed Espressif core.
