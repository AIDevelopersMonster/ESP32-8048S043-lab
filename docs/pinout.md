# ESP32-8048S043 Sample A — pinout and connector evidence

This document separates **physically observed/validated**, **source-backed**, **MCU capability only**, and **revision-dependent** information for the actual project board.

## Current Sample A status

```text
RGB display runtime      PHYSICAL PASS
GT911 touchscreen        PHYSICAL PASS, internal I2C at 0x5D
microSD / TF             PHYSICAL PASS, SPI 10 MHz
P2 PCB labels            PHYSICALLY OBSERVED: IO19 / IO11 / IO12 / IO13
P3 PCB labels            PHYSICALLY OBSERVED: IO17 / IO18 / IO19 / IO20
P4 PCB labels            PHYSICALLY OBSERVED: GND / 3.3V / IO17 / IO18
GPIO19/20 silk group     PHYSICALLY OBSERVED: USB
GPIO17/18 silk group     PHYSICALLY OBSERVED: UART1
R17                      PHYSICALLY OBSERVED: DNP / not fitted; GPIO18 net
P3 IO17 <-> P4 IO17      CONTINUITY PASS
P3 IO18 <-> P4 IO18      CONTINUITY PASS
P3 IO19 <-> P2 IO19      CONTINUITY PASS
P3 IO19 <-> R4           CONTINUITY PASS
P3 IO20 <-> R3           CONTINUITY PASS
GPIO17 <-> ESP module pin 10 from dot   CONTINUITY PASS
R7 <-> CH340C pin 2      CONTINUITY PASS
R6 <-> CH340C pin 3      CONTINUITY PASS
Flash                    16 MB
PSRAM                    8 MB
External I2C pins        PHYSICALLY EXPOSED on P3 IO19/IO20; coexistence test pending
External ADC input       NOT YET VERIFIED
```

## Evidence classes

- **PHYSICAL PASS** — exercised on the actual Sample A hardware.
- **CONTINUITY PASS** — electrical continuity verified on Sample A with a meter.
- **PHYSICALLY OBSERVED** — visible silkscreen or directly observed hardware, but not necessarily functionally exercised.
- **SOURCE-BACKED** — supported by external board documentation/schematic material for this family, but not yet confirmed pin-by-pin on Sample A.
- **MCU CAPABILITY ONLY** — ESP32-S3 itself supports the function on the GPIO according to the chip datasheet. This does **not** prove that the function is safe, usable, or electrically clean on this board connector.

A function is promoted to a supported platform interface only after both board-level connectivity and the relevant functional coexistence test pass.

## Evidence sources

Primary project evidence:

- project macro photographs and visible PCB silkscreen;
- direct physical inspection of fitted / DNP option links;
- direct continuity measurements on Sample A;
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
| SDA | 19 | PHYSICAL PASS; P3 IO19 continuity to P2 IO19 and R4 |
| SCL | 20 | PHYSICAL PASS; P3 IO20 continuity to R3 |
| RESET | 38 | PHYSICAL PASS |
| INT option | 18 through R17 | R17 physically DNP / open on Sample A; current driver does not use INT |
| I2C address | 0x5D | PHYSICAL PASS |

The project has proved that **GT911 uses I2C on GPIO19/20**. Current App09 creates one ESP32-S3 I2C master bus on those pins and attaches GT911 as a slave at `0x5D`; current touch configuration uses no interrupt GPIO (`GPIO_NUM_NC`).

### Sample A GPIO17 / GPIO18 continuity correction

Direct re-check on Sample A establishes:

```text
P3 IO17 <-> P4 IO17 <-> ESP32-S3-WROOM-1 physical pin 10 counted from the module pin-1 dot
P3 IO18 <-> P4 IO18 <-> R17-side net
R17 = DNP / not fitted
```

Therefore **R17 belongs to the GPIO18 path, not GPIO17**. The previous project note that placed R17 on GPIO17 was a measurement transcription error and is superseded by this correction.

No external pull-up/pull-down component for GPIO17 has yet been identified by continuity inspection. Record this as **not found**, not as proof that no such bias network exists anywhere on the board.

With R17 open, GPIO18 is not connected through that option link to the downstream INT path. GPIO17 has direct continuity to the ESP module pin corresponding to its net and is not currently tied to any identified option resistor.

## GPIO19/20: P3 exposes the active touch I2C nets

The actual Sample A silkscreen labels the P3 lower pair `IO19 / IO20` as **USB**, consistent with the ESP32-S3 native USB D- / D+ capability. Runtime and continuity evidence show that, in this capacitive-touch assembly, those same physical nets are the GT911 I2C bus:

```text
P3 IO19 -> P2 IO19 -> R4 -> GPIO19 / active GT911 SDA net
P3 IO20 -> R3              -> GPIO20 / active GT911 SCL net
GT911 = slave 0x5D
```

Thus P3 physically exposes the already-running system I2C nets. The remaining acceptance step is a **functional coexistence test with an external I2C slave while touch remains operational**.

The `USB` silk identifies the alternate native MCU function. Native USB cannot be used simultaneously on these pins while the GT911 I2C wiring and pull-ups remain active.

## microSD / TF1 mapping

Status: `PHYSICAL PASS at 10 MHz`.

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| CLK | 12 |
| MISO | 13 |

GPIO11/12/13 are the active SD SPI signals.

## External connector P2 — Sample A

Status: `PHYSICALLY OBSERVED PCB LABELS + SD NET CORRELATION`.

The actual board is marked:

| P2 pin | PCB marking | Current platform use | External-interface status |
|---:|---|---|---|
| 1 | IO19 | GT911 SDA | continuity-confirmed shared net with P3 IO19; not usable as independent SPI CS in current touch configuration |
| 2 | IO11 | SD MOSI | SPI data net; external coexistence not yet tested |
| 3 | IO12 | SD CLK | SPI clock net; external coexistence not yet tested |
| 4 | IO13 | SD MISO | SPI data net; external coexistence not yet tested |

So for this Sample A:

```text
P2 = IO19 / IO11 / IO12 / IO13
```

Because IO19 is the active GT911 SDA net, P2 is a mixed-function/legacy header on this capacitive configuration rather than a self-contained four-wire SPI connector.

## P3 — Sample A

Silkscreen, top-to-bottom in the project photograph:

```text
IO17   UART1
IO18   UART1
IO19   USB
IO20   USB
```

Continuity measurements:

```text
P3 IO17 <-> P4 IO17 <-> ESP module physical pin 10 from pin-1 dot
P3 IO18 <-> P4 IO18 <-> R17 net
P3 IO19 <-> P2 IO19 <-> R4
P3 IO20 <-> R3
```

Current KONTAKTS interpretation:

- `IO17/IO18`: general-purpose candidates; also UART1-capable at MCU level / silk intent;
- `IO18`: optional R17 branch is open because R17 is DNP;
- `IO17`: no external pull-up/pull-down component has yet been located;
- `IO19/IO20`: active system I2C bus for GT911, physically exposed on P3;
- `USB` is an alternate ESP32-S3 function and is not simultaneously available with the current touch wiring.

## P4 — Sample A

Silkscreen, top-to-bottom in the project photograph:

```text
GND
3.3V
IO17
IO18
```

Continuity already proves:

```text
P4 IO17 <-> P3 IO17
P4 IO18 <-> P3 IO18
```

The GND and 3.3V rails remain to be recorded as voltage/continuity acceptance measurements if not already done.

## UART0 / CH340C service path

The board silk at the service connector shows:

```text
5V / TXD0 / RXD0 / GND
```

Sample A continuity measurements prove:

```text
R7 <-> CH340C pin 2
R6 <-> CH340C pin 3
```

This is consistent with the CH340C UART service circuitry. The exact header-to-resistor path should still be recorded explicitly if a complete connector certification table is desired; do not infer signal direction only from resistor reference designators.

## GPIO17 / GPIO18 — best current free-pin candidates

Current engineering classification:

```text
GPIO17   LIKELY FREE in current firmware; P3/P4 and ESP module continuity confirmed; no external bias resistor found yet
GPIO18   LIKELY FREE in current firmware; P3/P4 continuity confirmed; R17 option link is DNP/open
```

Neither is used by the current App09 firmware for LCD, SD, GT911 SDA/SCL/reset, flash or PSRAM. GPIO18 has an optional hardware branch through R17, but R17 is physically not fitted on Sample A.

Next acceptance step: drive/read each pin digitally while LCD, touch, Wi-Fi and SD remain operational.

## ADC boundary

**No external ADC input is currently guaranteed by this project.**

The ESP32-S3 silicon contains ADC channels, and GPIO17/18 are interesting candidates at the MCU level. Board routing is now much better established, but a controlled known-voltage ADC test is still required before advertising either connector pin as an analog input.

## I2C boundary

Sample A evidence:

```text
GT911 runtime on GPIO19/20             PHYSICAL PASS
P3 IO19 -> P2 IO19 -> R4              CONTINUITY PASS
P3 IO20 -> R3                          CONTINUITY PASS
external I2C slave coexistence         NOT YET TESTED
```

Accordingly, **P3 IO19/IO20 are physically exposed active I2C nets**, but the project will not yet label them a fully supported expansion bus until an external slave is operated successfully alongside GT911.

## Electrical boundary

All ESP32-S3 GPIO signals are 3.3 V logic. Do not apply 5 V directly to a GPIO. GPIO pins are control/signal outputs, not power outputs: relays, pumps, motors, solenoids and other substantial loads require an external driver and suitable power supply.

The exact available external current from any 3.3 V connector rail on Sample A is not yet characterized.

## Validation checklist

- [x] factory firmware preserved by double-read SHA-256 match;
- [x] own RGB display path physically validated;
- [x] GT911 detected and touchscreen physically validated at 0x5D;
- [x] SD initialized and physically validated on GPIO10/11/12/13;
- [x] Sample A P2 silk-screen sequence confirmed as IO19/IO11/IO12/IO13;
- [x] Sample A P3 silk-screen sequence confirmed as IO17/IO18/IO19/IO20;
- [x] Sample A P4 silk-screen sequence confirmed as GND/3.3V/IO17/IO18;
- [x] Sample A P3 IO17 <-> P4 IO17 continuity;
- [x] Sample A P3 IO18 <-> P4 IO18 continuity;
- [x] Sample A GPIO17 <-> ESP module physical pin 10 from pin-1 dot continuity;
- [x] Sample A GPIO18 <-> R17 net continuity;
- [x] Sample A R17 physically confirmed DNP / not fitted;
- [x] Sample A P3 IO19 <-> P2 IO19 continuity;
- [x] Sample A P3 IO19 <-> R4 continuity;
- [x] Sample A P3 IO20 <-> R3 continuity;
- [x] Sample A R7 <-> CH340C pin 2 continuity;
- [x] Sample A R6 <-> CH340C pin 3 continuity;
- [x] App09 SD launcher physically validated;
- [x] SD-backed browser Help Center physically validated;
- [ ] identify the far side/function of R17 if needed;
- [ ] locate/verify any external bias network for GPIO17; none found so far;
- [ ] record P4 GND/3.3V rail continuity/voltage;
- [ ] record complete service-header-to-R6/R7 continuity if desired;
- [ ] validate GPIO17/18 as digital input/output;
- [ ] validate an external I2C slave while GT911 remains operational;
- [ ] validate ADC with known voltages and document usable range/error;
- [ ] validate P2 shared-SPI coexistence with microSD if ever needed;
- [ ] measure available external 3.3 V current under LCD + Wi-Fi + SD load.

## Boundary

For Sample A, the connector silk and several critical nets are directly confirmed by continuity measurements. P3 IO19/IO20 physically expose the active GPIO19/20 GT911 I2C bus; P3/P4 share GPIO17/18; P2 IO19 is the same net as P3 IO19. **R17 is confirmed on GPIO18, not GPIO17, and is physically DNP/open. GPIO17 has direct continuity to ESP32-S3-WROOM-1 physical pin 10 counted from the pin-1 dot; no external pull-up/pull-down for GPIO17 has yet been located.** Functional external-I2C, digital-GPIO and ADC tests remain separate acceptance steps.
