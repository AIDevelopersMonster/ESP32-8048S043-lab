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

## Arduino IDE setup used for Sample A

Install the ESP32 board package:

```text
Boards Manager -> esp32 by Espressif Systems
```

The following table mirrors the Arduino IDE Tools menu used for Sample A.

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
| PSRAM | QSPI PSRAM |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |
| Zigbee Mode | Disabled |
| Serial Monitor | 115200 baud |

Notes:

```text
COM12 is the local port observed on Sample A. Choose your actual CH340 port.
If upload is unstable at 921600, retry Upload Speed = 460800 before changing other settings.
For factory flash readback/dump workflows, 460800 was more reliable than 921600.
```

Menu names differ between ESP32 Arduino core versions. If your IDE does not show exactly the same wording, preserve the intent:

```text
ESP32-S3 target
16 MB flash
QSPI PSRAM enabled
external UART upload through CH340C / UART0 Hardware CDC
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
Board                                : ESP32S3 Dev Module
Flash Mode                           : QIO 80MHz
Flash Size                           : 16MB (128Mb)
Partition Scheme                     : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM                                : QSPI PSRAM
Upload Mode                          : UART0 / Hardware CDC
Upload Speed                         : 921600
USB Mode                             : Hardware CDC and JTAG
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
