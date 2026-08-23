# 02_DisplayRGBTest

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

## Purpose

`02_DisplayRGBTest` is the second test from the `ESP32_8048S043` Arduino library.

It runs after `01_BoardInfo` and checks our own minimal RGB display path on the physical ESP32-8048S043 / ESP32-8048S043C-I board.

The goal is not to run LVGL yet. The goal is to verify that the board can show simple graphics using our own Arduino sketch, the source-backed RGB GPIO map and `Arduino_GFX_Library`.

## What this test checks

This test validates:

```text
RGB panel initialization through Arduino_GFX_Library
800x480 display size
Backlight GPIO 2 basic ON control
RGB control pins: DE / VSYNC / HSYNC / PCLK
RGB data pins: R0..R4, G0..G5, B0..B4
Basic color order: red / green / blue / white / black
Landscape orientation
Corner marker placement
Color bars
Stripe/data-line sanity pattern
Basic repeated drawing stability
```

Source-backed pin map used by this test:

```text
LCD DE        40
LCD VSYNC     41
LCD HSYNC     39
LCD PCLK      42
Backlight PWM 2
RGB R0..R4    45, 48, 47, 21, 14
RGB G0..G5    5, 6, 7, 15, 16, 4
RGB B0..B4    8, 3, 46, 9, 1
```

## What this test does not check

This test does not validate:

```text
GT911 touch
SD card
Wi-Fi / BLE
LVGL integration
backlight PWM dimming quality
display tearing under LVGL load
final BSP status
```

Those are separate validation stages.

## Required dependency

Install this Arduino library before compiling:

```text
Arduino_GFX_Library by moononournation
```

Arduino IDE path:

```text
Tools / Manage Libraries -> search Arduino_GFX_Library -> Install
```

## Arduino IDE settings

Use the same base settings that passed `01_BoardInfo` on Sample A:

| Arduino IDE menu | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB CDC On Boot | Disabled |
| USB Mode | Hardware CDC and JTAG |
| Serial Monitor | 115200 baud |

Local port depends on the computer. On Sample A it was observed as `COM12`.

## How to run

Open:

```text
File -> Examples -> ESP32_8048S043 -> 02_DisplayRGBTest
```

Then upload the sketch and open Serial Monitor at:

```text
115200 baud
```

Expected Serial Monitor start:

```text
================================================================
 ESP32-8048S043 Lab / 02_DisplayRGBTest
 Minimal RGB panel + backlight validation
================================================================
Author : Alex Malachevsky
GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
Purpose: validate own Arduino_GFX RGB path after 01_BoardInfo PASS
----------------------------------------------------------------
[PIN MAP]
LCD 800x480
DE=40 VSYNC=41 HSYNC=39 PCLK=42 BL=2
R0..R4=45,48,47,21,14
G0..G5=5,6,7,15,16,4
B0..B4=8,3,46,9,1
Display begin: OK
Test sequence started.
```

## Expected screen sequence

The screen should repeatedly show:

```text
1. Full-screen RED
2. Full-screen GREEN
3. Full-screen BLUE
4. Full-screen WHITE
5. Full-screen BLACK
6. Orientation frame
7. RGB color bars
8. Stripe/data-line pattern
```

The orientation frame should show:

```text
top-left     red
top-right    green
bottom-left  blue
bottom-right white
```

## How to evaluate the result

### PASS candidate

Mark this test as `PHYSICAL PASS` only when all of the following are true on a named specimen:

```text
Display begin reports OK in Serial Monitor.
The display lights up.
The image fills the full 800x480 panel area.
Red, green and blue full-screen tests look like the expected colors.
White and black screens look correct.
The screen is landscape, not rotated or mirrored.
The corner markers appear in the correct positions:
  TL red / TR green / BL blue / BR white.
The color bars are stable and recognizable.
The stripe pattern is stable, not random noise.
The test loops without crashes or repeated resets.
```

### FAIL / investigate

Investigate before marking PASS if any of the following happens:

```text
Display begin reports FAIL.
Backlight stays off.
The panel is white/black only with no changing graphics.
Colors are obviously swapped, for example red appears blue.
The image is rotated, mirrored or shifted.
The image does not fill the 800x480 area.
There are random pixels, heavy noise or unstable stripes.
The board repeatedly resets.
```

## Evidence to capture

For repository evidence, capture at least:

```text
Serial Monitor output from startup through one full cycle.
Photo or video of the full-screen colors.
Photo or video of the orientation frame.
Photo or video of the color bars or stripe pattern.
```

After visual confirmation, add a short evidence note under:

```text
evidence/specimens/sample-a/arduino/
```

## Boundary

A PASS here means:

```text
Our own minimal Arduino_GFX RGB/backlight display test works on Sample A.
```

It does not mean:

```text
Touch PASS
SD PASS
LVGL PASS
Full BSP PASS
```

The next stages remain separate.
