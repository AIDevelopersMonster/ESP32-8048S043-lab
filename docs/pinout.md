# ESP32-8048S043 Sample A — pinout and connector evidence

This document separates **physically observed/validated**, **source-backed**, **MCU capability only**, and **revision-dependent** information for the actual project board.

## Current Sample A status

```text
RGB display runtime      PHYSICAL PASS
GT911 touchscreen        PHYSICAL PASS, internal I2C at 0x5D
microSD / TF             PHYSICAL PASS, SPI 10 MHz
P2 PCB labels            PHYSICALLY OBSERVED: IO19 / IO11 / IO12 / IO13
Flash                    16 MB
PSRAM                    8 MB
External I2C port        NOT YET VERIFIED
External ADC input       NOT YET VERIFIED
P1/P3/P4 full pinout     SOURCE-BACKED, continuity test still required
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

## Evidence classes

- **PHYSICAL PASS** — exercised on the actual Sample A hardware.
- **PHYSICALLY OBSERVED** — visible silkscreen or directly observed hardware, but not necessarily electrically continuity-tested.
- **SOURCE-BACKED** — supported by external board documentation/schematic material for this family, but not yet confirmed pin-by-pin on Sample A.
- **MCU CAPABILITY ONLY** — ESP32-S3 itself supports the function on the GPIO according to the chip datasheet. This does **not** prove that the function is safe, usable, or electrically clean on this board connector.

A function must not be advertised in Help Center as an available external interface until it passes the relevant board-level physical test.

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

Status: `PHYSICAL PASS FOR BOARD-INTERNAL TOUCH BUS`.

| Signal | GPIO / value |
|---|---:|
| SDA | 19 |
| SCL | 20 |
| RESET | 38 |
| INT | 18, optional / link-dependent |
| I2C address on Sample A | 0x5D |

The project has therefore proved that **the GT911 itself uses I2C internally on GPIO19/20**. It has **not** yet proved that P3 or any other connector is a supported external I2C expansion port. External sharing of GPIO19/20 with another I2C device remains an experiment until continuity, electrical loading and coexistence are physically tested.

## microSD / TF1 mapping

Status: `PHYSICAL PASS at 10 MHz`.

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| CLK | 12 |
| MISO | 13 |

GPIO11/12/13 are the active SD SPI signals. Their presence on P2 is visually/source backed, but use of P2 as an external shared-SPI expansion connector still needs a physical coexistence test with the SD card.

## External connector P2 — Sample A

Status: `PHYSICALLY OBSERVED PCB LABELS + SD NET CORRELATION`.

The actual user's board is marked:

| P2 pin | PCB marking | Current platform use | External-interface status |
|---:|---|---|---|
| 1 | IO19 | GT911 SDA | NOT VERIFIED as external I2C/GPIO |
| 2 | IO11 | SD MOSI | NOT VERIFIED as external shared SPI pin |
| 3 | IO12 | SD CLK | NOT VERIFIED as external shared SPI pin |
| 4 | IO13 | SD MISO | NOT VERIFIED as external shared SPI pin |

So for this Sample A the exact visible sequence is:

```text
IO19 / IO11 / IO12 / IO13
```

Do not substitute GPIO18 for P2.1 on this specimen.

## External P1/P3/P4 — current confidence

The currently collected third-party board documentation gives likely mappings for P1/P3/P4, but these have not yet been continuity-tested on Sample A. They remain **SOURCE-BACKED**, not guaranteed connector contracts.

In particular:

- GPIO17/18 may be attractive general-purpose candidates, but external access and conflicts must be physically confirmed first;
- GPIO19/20 are proven internally for GT911 I2C, but no guaranteed external I2C port has been accepted;
- ESP32-S3 datasheets list ADC functions on several GPIOs, but no exposed connector pin has yet passed a known-voltage ADC test on Sample A;
- UART1, PWM and alternative GPIO-matrix functions are MCU capabilities until the corresponding connector/net is physically validated.

## ADC boundary

**No external ADC input is currently guaranteed by this project.**

The ESP32-S3 silicon contains ADC channels, and some candidate external GPIO numbers correspond to ADC-capable pads in the Espressif datasheet. That is only an MCU-level capability. Board routing, attached peripherals, connector continuity, attenuation, calibration, source impedance and noise have not yet been validated on Sample A.

Do not document `GPIO17 = ADC input`, `GPIO18 = ADC input`, or similar as an available board feature until a controlled voltage test is passed and recorded.

## I2C boundary

**No external I2C expansion port is currently guaranteed by this project.**

What is physically proven is only:

```text
ESP32-S3 GPIO19/20 <-> board-internal GT911 I2C path
GT911 responds at 0x5D
```

Whether an external device can safely and reliably share those nets through P2/P3 is still open. A successful external I2C scanner/device coexistence test is required before the Help Center can advertise an I2C expansion connector.

## Electrical boundary

All ESP32-S3 GPIO signals are 3.3 V logic. Do not apply 5 V directly to a GPIO. GPIO pins are control/signal outputs, not power outputs: relays, pumps, motors, solenoids and other substantial loads require an external driver and suitable power supply.

The exact available external current from any 3.3 V connector rail on Sample A is not yet characterized.

## Validation checklist

- [x] factory firmware preserved by double-read SHA-256 match;
- [x] own RGB display path physically validated;
- [x] GT911 detected and touchscreen physically validated at 0x5D;
- [x] SD initialized and physically validated on GPIO10/11/12/13;
- [x] Sample A P2 silk-screen sequence confirmed as IO19/IO11/IO12/IO13;
- [x] App09 SD launcher physically validated;
- [x] SD-backed browser Help Center physically validated;
- [ ] continuity-test every external connector pin against the MCU/net;
- [ ] validate GPIO17/18 as digital input/output on the actual connector;
- [ ] validate UART1 externally if P3 mapping is confirmed;
- [ ] validate an external I2C device while GT911 remains operational;
- [ ] validate ADC with known voltages and document usable range/error;
- [ ] validate P2 shared-SPI coexistence with microSD;
- [ ] measure available external 3.3 V current under LCD + Wi-Fi + SD load;
- [ ] confirm P1/P3/P4 pin order and connector pitch by measurement on Sample A.

## Boundary

For Sample A, P2 is no longer revision-ambiguous at the silkscreen level: the physical board identifies `IO19 / IO11 / IO12 / IO13`. Beyond the already tested LCD, GT911 and microSD functions, external connector capabilities remain provisional until explicitly measured on this specimen.
