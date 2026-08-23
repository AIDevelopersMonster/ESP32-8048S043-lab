# 01_BoardInfo

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

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

It does not initialize the RGB display, GT911 touch, SD card or LVGL. It only confirms that the board can be flashed and that the ESP32-S3 runtime reports the expected chip, flash and PSRAM configuration.

## Why this test comes first

Run this example before display or touch tests because it verifies the basic programming path and memory profile.

Expected board family:

```text
ESP32-S3
16 MB flash
8 MB PSRAM
CH340C USB-UART bridge on the capacitive-touch Jingcai/TinyTronics variant
```

## Arduino IDE setup

Install the ESP32 board package:

```text
Boards Manager -> esp32 by Espressif Systems
```

Recommended first settings:

| Arduino IDE menu | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Port | CH340 / USB-SERIAL port of the board |
| Upload Speed | 460800 first; 921600 only if stable |
| CPU Frequency | 240MHz (WiFi) |
| Flash Size | 16MB / 128Mb |
| Flash Mode | DIO recommended first |
| Partition Scheme | Any 16MB-compatible scheme for this smoke test |
| PSRAM | OPI PSRAM / Enabled |
| USB CDC On Boot | Disabled when using CH340C USB-UART |
| Upload Mode | UART0 / Hardware CDC, depending on menu wording |
| Core Debug Level | None |
| Serial Monitor | 115200 baud |

Menu names differ between ESP32 Arduino core versions. If your IDE does not show exactly the same wording, preserve the intent:

```text
ESP32-S3 target
16 MB flash
8 MB / OPI PSRAM enabled
external UART upload through CH340C
serial monitor at 115200
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

## Expected output

The log should include:

```text
ESP32-8048S043 Lab / 01_BoardInfo
First Arduino IDE smoke test
Author        : Alex Malachevsky
GitHub        : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
Evidence video: https://youtube.com/shorts/XVaWqrtXHE4
Board         : ESP32S3 Dev Module
Flash Size    : 16MB / 128Mb
PSRAM         : OPI PSRAM / Enabled
```

The board information block should report approximately:

```text
Chip model      : ESP32-S3
Flash size      : 16777216 bytes
PSRAM size      : 8388608 bytes
```

Then the sketch should continue printing:

```text
ALIVE uptime=... freeHeap=... freePsram=...
```

every five seconds.

## PASS condition

Mark this test as PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
chip model is ESP32-S3;
flash is reported as about 16 MB;
PSRAM is reported as about 8 MB;
ALIVE messages continue without resets.
```

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
