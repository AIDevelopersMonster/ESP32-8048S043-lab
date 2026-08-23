# Pinout working document

This file separates **reported**, **source-backed** and **physically verified** pin mappings.

Current status for Sample A:

```text
RGB display runtime      PASS with factory LVGL Widgets Demo
Touchscreen visual check PASS with factory LVGL Widgets Demo
Exact touch IC identity  SOURCE-BACKED GT911, dedicated I2C scan still open
Exact pin map            SOURCE-BACKED, not yet fully continuity-verified
```

## Evidence sources

Primary project evidence:

- factory firmware dump and partition analysis;
- factory serial boot log;
- factory LVGL Widgets Demo display PASS;
- factory touchscreen visual PASS video;
- project macro photographs.

Source-backed reconstruction:

- `hardware/SCHEMATIC_BOM_RESEARCH.md`;
- JCZN1688 / Jingcai `ESP32-8048S043` support archive lead;
- TinyTronics Jingcai ESP32-8048S043C-I documentation package lead;
- same-layout annotated 2022-10-18 board reference.

## Source-backed 800x480 RGB panel mapping

Status: `SOURCE-BACKED / FACTORY RUNTIME DISPLAY PASS`.

The mapping below is backed by the recovered board documentation and is consistent with the factory LVGL display runtime PASS. It is not yet promoted to a full BSP pinout PASS until reproduced by our own minimal RGB example.

| Signal | GPIO |
|---|---:|
| DE | 40 |
| VSYNC | 41 |
| HSYNC | 39 |
| PCLK / DCLK | 42 |
| Backlight PWM | 2 |
| R0 | 45 |
| R1 | 48 |
| R2 | 47 |
| R3 | 21 |
| R4 | 14 |
| G0 | 5 |
| G1 | 6 |
| G2 | 7 |
| G3 | 15 |
| G4 | 16 |
| G5 | 4 |
| B0 | 8 |
| B1 | 3 |
| B2 | 46 |
| B3 | 9 |
| B4 | 1 |

Compact form:

```text
DE 40, VSYNC 41, HSYNC 39, PCLK 42, BL 2
R0..R4 = 45, 48, 47, 21, 14
G0..G5 = 5, 6, 7, 15, 16, 4
B0..B4 = 8, 3, 46, 9, 1
```

## Source-backed GT911 capacitive touch mapping

Status: `SOURCE-BACKED / FACTORY TOUCHSCREEN VISUAL PASS / CONTROLLER SCAN OPEN`.

The factory demo responds to touch in the visual check, and the recovered documentation identifies GT911 for the capacitive-touch version. A dedicated I2C scanner and coordinate-target test are still required before marking the controller and coordinate transform as fully verified.

| Signal | GPIO / value |
|---|---:|
| SDA | 19 |
| SCL | 20 |
| RESET | 38 |
| INT | 18, optional / link-dependent |
| I2C address | 0x5D or 0x14 depending on reset/address strap sequence |

## Source-backed microSD / TF1 mapping

Status: `SOURCE-BACKED / NOT YET PHYSICALLY TESTED`.

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| CLK | 12 |
| MISO | 13 |

## Board-level interface notes

The TinyTronics/Jingcai documentation path identifies the capacitive-touch board as using:

```text
ESP32-S3
16 MB flash
8 MB PSRAM
CH340C USB-UART bridge
GT911 capacitive-touch controller
```

The PCB family may contain both the XPT2046 resistive-touch footprint/device and the GT911 capacitive-touch interface. For the current capacitive panel, do not assume XPT2046 is active.

## Validation checklist

- [x] factory firmware preserved by double-read SHA-256 match;
- [x] factory LVGL Widgets Demo serial boot confirmed;
- [x] factory LVGL Widgets Demo display visible on 800x480 panel;
- [x] touchscreen visually responds in factory demo video;
- [ ] minimal RGB color-bar example built and physically validated;
- [ ] backlight PWM GPIO 2 confirmed by dedicated brightness test;
- [ ] RGB data order confirmed by own color test;
- [ ] PCLK polarity confirmed by own timing test;
- [ ] GT911 detected by I2C scan at 0x5D or 0x14;
- [ ] GT911 reset/address strap behavior documented;
- [ ] touch coordinates match rendered targets;
- [ ] orientation transform documented;
- [ ] SD card initialized and read/write tested on GPIO 10/11/12/13.

## Boundary

This document is now stronger than a vendor pinout copy, but still not the final BSP validation certificate. The next promotion step is to run our own minimal examples against this map and attach serial/video/photo evidence for each subsystem.
