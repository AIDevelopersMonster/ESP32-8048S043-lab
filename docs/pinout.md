# ESP32-8048S043 Sample A — pinout and connector evidence

This document separates **physically observed/validated**, **source-backed**, **MCU capability only**, and **revision-dependent** information for the actual project board.

## Current Sample A status

```text
RGB display runtime      PHYSICAL PASS
GT911 touchscreen        PHYSICAL PASS, internal I2C at 0x5D
microSD / TF             PHYSICAL PASS, SPI 10 MHz
P2 PCB labels            PHYSICALLY OBSERVED: IO19 / IO11 / IO12 / IO13
GPIO19/20 silk group     PHYSICALLY OBSERVED: USB
R17                      PHYSICALLY OBSERVED: DNP / not fitted
Flash                    16 MB
PSRAM                    8 MB
External I2C port        NOT YET VERIFIED
External ADC input       NOT YET VERIFIED
P1/P3/P4 full pinout     SOURCE-BACKED, continuity test still required
```

The user's actual Sample A PCB follows the P2 variant whose four printed signals are:

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
- direct physical inspection of fitted / DNP option links;
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

| Signal | GPIO / value | Sample A status |
|---|---:|---|
| SDA | 19 | PHYSICAL PASS |
| SCL | 20 | PHYSICAL PASS |
| RESET | 38 | PHYSICAL PASS |
| INT | 18 through optional R17 | **R17 physically DNP / not fitted** |
| I2C address | 0x5D | PHYSICAL PASS |

The project has proved that **GT911 uses I2C on GPIO19/20**. Current App09 creates one ESP32-S3 I2C master bus on those pins and attaches GT911 as a slave at `0x5D`; `GPIO18` is not used by the driver because `int_gpio_num = GPIO_NUM_NC`.

Physical inspection now also confirms that **R17 is not fitted on Sample A**, matching the reference capacitive-touch option. Therefore GPIO18 is not coupled to GT911 INT through R17 on this specimen. This materially strengthens GPIO18 as a likely free user GPIO, although connector continuity / digital-I/O acceptance is still required before calling it a guaranteed external pin.

## GPIO19/20: why the PCB says USB while touch uses I2C

The actual Sample A silkscreen marks the GPIO19/20 pair as **USB**. This is consistent with ESP32-S3 silicon: GPIO19 and GPIO20 are the native USB D- / D+ capable pins.

However, on the capacitive-touch configuration of this board, the same GPIOs are also the physically validated GT911 I2C bus:

```text
GPIO19 = GT911 SDA
GPIO20 = GT911 SCL
GT911  = slave 0x5D
```

So the `USB` silkscreen identifies an **alternate native function of those MCU pins / shared PCB-family routing**, not a guarantee that native USB can be used simultaneously with the active GT911 bus. In the current KONTAKTS platform, GPIO19/20 are configured as I2C, not USB D-/D+.

Do **not** connect or enable native USB on this GPIO19/20 header while the GT911 I2C path remains active. Native USB use would require a deliberate hardware/firmware mode change, including validation of the GT911 connection and I2C pull-ups on those nets.

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

## GPIO17 / GPIO18 — best current free-pin candidates

Current engineering classification:

```text
GPIO17   LIKELY FREE
GPIO18   LIKELY FREE; GT911 INT path physically open because R17 is DNP
```

Neither GPIO17 nor GPIO18 is used by the current App09 firmware for LCD, SD, Wi-Fi, GT911 I2C, touch reset, flash or PSRAM. GPIO18 had been the only significant touch-related uncertainty; physical confirmation that R17 is absent removes that coupling for Sample A.

This still does **not** yet certify either pin as an external connector contract. The next acceptance step is a continuity test followed by digital LOW/HIGH/input testing while LCD, touch, Wi-Fi and SD remain active.

## External P1/P3/P4 — current confidence

The currently collected third-party board documentation gives likely mappings for P1/P3/P4, but these have not yet been continuity-tested on Sample A. They remain **SOURCE-BACKED**, not guaranteed connector contracts.

New Sample A physical observations add two useful facts:

- the GPIO19/20 pair is silk-grouped as `USB`;
- R17 is physically not fitted.

In particular:

- GPIO17/18 are now the strongest candidates for general-purpose external GPIO;
- GPIO19/20 are proven internally for GT911 I2C and are native-USB-capable pins, but no guaranteed external I2C expansion port has yet been accepted;
- ESP32-S3 datasheets list ADC functions on several GPIOs, but no exposed connector pin has yet passed a known-voltage ADC test on Sample A;
- UART1, PWM and alternative GPIO-matrix functions are MCU capabilities until the corresponding connector/net is physically validated.

## ADC boundary

**No external ADC input is currently guaranteed by this project.**

The ESP32-S3 silicon contains ADC channels, and some candidate external GPIO numbers correspond to ADC-capable pads in the Espressif datasheet. That is only an MCU-level capability. Board routing, attached peripherals, connector continuity, attenuation, calibration, source impedance and noise have not yet been validated on Sample A.

Do not document `GPIO17 = ADC input`, `GPIO18 = ADC input`, or similar as an available board feature until a controlled voltage test is passed and recorded.

## I2C boundary

**No external I2C expansion port is currently guaranteed by this project.**

What is physically proven is:

```text
ESP32-S3 GPIO19/20 <-> board-internal GT911 I2C path
GT911 responds at 0x5D
Sample A silk marks the GPIO19/20 pair as USB
```

Electrically, adding another slave to an existing I2C bus is normal. The remaining open issue is not protocol compatibility; it is proving connector continuity / loading on Sample A and then testing coexistence with touch. A successful external slave test is required before the Help Center advertises the connector as a supported external I2C expansion bus.

## Electrical boundary

All ESP32-S3 GPIO signals are 3.3 V logic. Do not apply 5 V directly to a GPIO. GPIO pins are control/signal outputs, not power outputs: relays, pumps, motors, solenoids and other substantial loads require an external driver and suitable power supply.

The exact available external current from any 3.3 V connector rail on Sample A is not yet characterized.

## Validation checklist

- [x] factory firmware preserved by double-read SHA-256 match;
- [x] own RGB display path physically validated;
- [x] GT911 detected and touchscreen physically validated at 0x5D;
- [x] SD initialized and physically validated on GPIO10/11/12/13;
- [x] Sample A P2 silk-screen sequence confirmed as IO19/IO11/IO12/IO13;
- [x] Sample A GPIO19/20 silk group observed as `USB`;
- [x] Sample A R17 physically confirmed DNP / not fitted;
- [x] App09 SD launcher physically validated;
- [x] SD-backed browser Help Center physically validated;
- [ ] continuity-test every external connector pin against the MCU/net;
- [ ] validate GPIO17/18 as digital input/output on the actual connector;
- [ ] validate UART1 externally if P3 mapping is confirmed;
- [ ] validate an external I2C slave while GT911 remains operational;
- [ ] validate ADC with known voltages and document usable range/error;
- [ ] validate P2 shared-SPI coexistence with microSD;
- [ ] measure available external 3.3 V current under LCD + Wi-Fi + SD load;
- [ ] confirm P1/P3/P4 pin order and connector pitch by measurement on Sample A.

## Boundary

For Sample A, P2 is no longer revision-ambiguous at the silkscreen level: the physical board identifies `IO19 / IO11 / IO12 / IO13`. GPIO19/20 are also physically marked as a `USB` pair, while actual runtime proves those same MCU pins are the GT911 I2C bus in the current capacitive-touch configuration. R17 is physically DNP, so GPIO18 is not connected to GT911 INT through that option link. Beyond the already tested LCD, GT911 and microSD functions, external connector capabilities remain provisional until explicitly measured on this specimen.
