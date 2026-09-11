# ESP32-8048S043 Sample A — pinout and connector evidence

This document separates **physically observed/validated**, **source-backed**, and **revision-dependent** mappings for the actual project board.

## Current Sample A status

```text
RGB display runtime      PHYSICAL PASS
GT911 touchscreen        PHYSICAL PASS, I2C 0x5D
microSD / TF             PHYSICAL PASS, SPI 10 MHz
P2 PCB labels            PHYSICALLY OBSERVED: IO19 / IO11 / IO12 / IO13
Flash                    16 MB
PSRAM                    8 MB
```

The user's actual Sample A PCB therefore follows the P2 variant whose four printed signals are:

```text
P2 / SPI
pin 1 = IO19
pin 2 = IO11
pin 3 = IO12
pin 4 = IO13
```

This overrides the earlier ambiguity for **Sample A**. Some external documents for older ESP32-8048S043 revisions show GPIO18 on P2 pin 1; that remains relevant only as a warning for other board revisions.

## Evidence sources

Primary project evidence:

- project macro photographs and visible PCB silkscreen;
- factory firmware dump and partition analysis;
- physical RGB display tests;
- physical GT911 touch tests;
- physical microSD tests;
- App09 SD application-library and browser Help Center acceptance tests.

Source-backed reconstruction:

- `hardware/SCHEMATIC_BOM_RESEARCH.md`;
- JCZN1688 / Jingcai `ESP32-8048S043` support archive lead;
- TinyTronics Jingcai ESP32-8048S043C-I documentation package lead;
- same-layout annotated board references.

## 800x480 RGB panel mapping

Status: `SOURCE-BACKED / OWN RUNTIME PHYSICAL PASS`.

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

## GT911 capacitive touch mapping

Status: `PHYSICAL PASS`.

| Signal | GPIO / value |
|---|---:|
| SDA | 19 |
| SCL | 20 |
| RESET | 38 |
| INT | 18, optional / link-dependent |
| I2C address on Sample A | 0x5D |

GPIO19/20 are therefore not electrically free even though they are exposed on the connector area: they form the active GT911 I2C bus and may only be shared with compatible I2C devices.

## microSD / TF1 mapping

Status: `PHYSICAL PASS at 10 MHz`.

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| CLK | 12 |
| MISO | 13 |

GPIO11/12/13 exposed on P2 are the same shared SPI lines used by microSD.

## External connector P2 — Sample A

Status: `PHYSICALLY OBSERVED PCB LABELS + SD FUNCTION CROSS-CHECK`.

The actual user's board is marked:

| P2 pin | PCB marking | Current platform use | Expansion note |
|---:|---|---|---|
| 1 | IO19 | GT911 SDA | Shared I2C line; not a free SPI CS in the current platform |
| 2 | IO11 | SD MOSI | Shared SPI MOSI |
| 3 | IO12 | SD CLK | Shared SPI clock |
| 4 | IO13 | SD MISO | Shared SPI MISO |

So for this Sample A the exact sequence is:

```text
IO19 / IO11 / IO12 / IO13
```

Do not substitute GPIO18 for P2.1 on this specimen.

## Practical expansion implications

- GPIO17 and GPIO18 remain the preferred general-purpose external signals when available on P3/P4.
- GPIO19/20 form the live GT911 I2C bus and can be shared only as I2C with non-conflicting device addresses.
- GPIO11/12/13 form the live SD SPI bus. A second SPI peripheral may share them if it has a separate CS and correctly tri-states MISO when deselected.
- GPIO10 remains the SD card CS in the current platform.
- GPIO43/44 are used for the UART0/CH340 console path and should not be treated as preferred expansion pins.
- LCD RGB, sync, PCLK and backlight pins are platform-reserved.
- OPI PSRAM pins on the N16R8 configuration are not available as ordinary expansion GPIO.

## Electrical boundary

All ESP32-S3 GPIO signals are 3.3 V logic. Do not apply 5 V directly to a GPIO. GPIO pins are control/signal outputs, not power outputs: relays, pumps, motors, solenoids and other substantial loads require an external driver and suitable power supply.

## Validation checklist

- [x] factory firmware preserved by double-read SHA-256 match;
- [x] own RGB display path physically validated;
- [x] GT911 detected and touchscreen physically validated at 0x5D;
- [x] SD initialized and physically validated on GPIO10/11/12/13;
- [x] Sample A P2 silk-screen sequence confirmed as IO19/IO11/IO12/IO13;
- [x] App09 SD launcher physically validated;
- [x] SD-backed browser Help Center physically validated;
- [ ] continuity-test every external connector pin against the MCU/net before declaring a full connector-level electrical certificate;
- [ ] measure available external 3.3 V current on the actual Sample A regulator under LCD + Wi-Fi + SD load;
- [ ] confirm P1/P3/P4 pin order and connector pitch by measurement on Sample A.

## Boundary

For Sample A, P2 is no longer revision-ambiguous: the physical board itself identifies `IO19 / IO11 / IO12 / IO13`. Other ESP32-8048S043 revisions may differ, so published third-party pinouts must not be applied blindly.
