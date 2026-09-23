# ESP32-8048S043 Sample A — pinout and connector evidence

This is the hardware contract for the actual project board **Sample A**. It separates physically tested functions, continuity measurements, visible silk, and MCU-only capabilities.

## Current Sample A status

```text
RGB display runtime      PHYSICAL PASS
GT911 touchscreen        PHYSICAL PASS, I2C slave 0x5D
microSD / TF             PHYSICAL PASS, SPI 10 MHz
P2 silk                  IO19 / IO11 / IO12 / IO13
P3 silk                  IO17 / IO18 / IO19 / IO20
P4 silk                  GND / 3.3V / IO17 / IO18
P3 IO17/18 silk group    UART1
P3 IO19/20 silk group    USB
R17                      DNP / not fitted; on GPIO18 option path
GPIO17                   ESP32-S3-WROOM-1 module pin 10 from pin-1 dot
GPIO18                   ESP32-S3-WROOM-1 module pin 11 from pin-1 dot
Flash                    16 MB
PSRAM                    8 MB
P1 +5V input             PHYSICAL PASS; board powers from P1
P1 Q1 protection         CJ3401 P-MOSF, marking R1; reverse-polarity protection
```

## Practical external resource map

For the current capacitive-touch configuration the useful external resources are:

```text
ALWAYS PRESENT

GPIO19 = system I2C SDA  -> GT911 resident slave 0x5D
GPIO20 = system I2C SCL  -> GT911 resident slave 0x5D
GPIO17 = primary user GPIO candidate
GPIO18 = primary user GPIO candidate; R17 option link is DNP/open

IF microSD IS NOT USED

GPIO11 = additional user GPIO candidate / SPI MOSI
GPIO12 = additional user GPIO candidate / SPI SCK
GPIO13 = additional user GPIO candidate / SPI MISO
```

Therefore the practical exposed resource count is:

```text
with SD in use:
    system I2C on GPIO19/20
    + 2 user GPIOs: 17, 18

without SD use:
    system I2C on GPIO19/20
    + 5 user GPIOs: 17, 18, 11, 12, 13
```

**GPIO19 does not become free when the SD card is removed.** It remains the GT911 SDA line. GPIO20 likewise remains GT911 SCL.

The three lines GPIO11/12/13 become candidates for other use only when the SD interface is not initialized/owned by the platform. Removing the card alone is not the software contract for reusing those pins.

## Evidence classes

- **PHYSICAL PASS** — function exercised on Sample A.
- **CONTINUITY PASS** — electrical continuity measured on Sample A.
- **PHYSICALLY OBSERVED** — silk/component state directly observed.
- **SOURCE-BACKED** — supported by matching external board documentation but not yet physically accepted.
- **MCU CAPABILITY ONLY** — ESP32-S3 silicon capability; not automatically a board-level interface.

## RGB LCD

Status: `PHYSICAL PASS` with the project firmware.

| Signal | GPIO |
|---|---:|
| DE | 40 |
| VSYNC | 41 |
| HSYNC | 39 |
| PCLK | 42 |
| Backlight | 2 |
| R0..R4 | 45, 48, 47, 21, 14 |
| G0..G5 | 5, 6, 7, 15, 16, 4 |
| B0..B4 | 8, 3, 46, 9, 1 |

## GT911 capacitive touch / system I2C

| Signal | GPIO | Sample A evidence |
|---|---:|---|
| SDA | 19 | PHYSICAL PASS; P3 IO19 <-> P2 IO19 <-> R4 continuity |
| SCL | 20 | PHYSICAL PASS; P3 IO20 <-> R3 continuity |
| RESET | 38 | PHYSICAL PASS |
| optional INT | 18 through R17 option | R17 is DNP/open; current driver uses no INT |
| address | 0x5D | PHYSICAL PASS |

Current App09 creates one ESP32-S3 I2C master bus on GPIO19/20 and attaches GT911 as a slave at `0x5D`.

P3 therefore physically exposes the already-running touch I2C nets:

```text
P3 IO19 -> GPIO19 -> GT911 SDA
P3 IO20 -> GPIO20 -> GT911 SCL
```

Adding another compatible I2C **slave** to this bus is architecturally normal. The remaining acceptance step is a coexistence test with an external slave while touch remains operational.

The PCB silk calls GPIO19/20 `USB` because these ESP32-S3 pins also support native USB D-/D+. In the current capacitive-touch configuration they are used as I2C and are **not free USB pins**.

## GPIO17 / GPIO18

Direct Sample A continuity:

```text
P3 IO17 <-> P4 IO17 <-> ESP32-S3-WROOM-1 module pin 10
P3 IO18 <-> P4 IO18 <-> ESP32-S3-WROOM-1 module pin 11
P3/P4 IO18 <-> R17-side net
R17 = DNP / not fitted
```

This agrees with the ESP32-S3-WROOM-1 module pinout:

```text
module pin 10 = GPIO17
module pin 11 = GPIO18
```

No external pull-up/pull-down component for GPIO17 has yet been located. Record that as **not found**, not as proof that no bias network exists anywhere on the PCB.

Current engineering classification:

```text
GPIO17  PRIMARY USER GPIO CANDIDATE
GPIO18  PRIMARY USER GPIO CANDIDATE; optional R17 branch open
```

Neither GPIO17 nor GPIO18 is used by the current platform for LCD, SD, GT911 SDA/SCL/reset, flash, or PSRAM. Final digital HIGH/LOW functional acceptance is still pending.

## microSD / SPI

Status: `PHYSICAL PASS at 10 MHz`.

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| SCK | 12 |
| MISO | 13 |

With SD active, GPIO11/12/13 belong to the SD SPI bus.

If the platform is deliberately run **without SD support**, GPIO11/12/13 are externally available on P2 and may be repurposed as three independent GPIOs or as another SPI use, subject to a functional test.

GPIO10 is the SD CS but is not part of the convenient P2 signal set, so it is not counted in the practical external GPIO total.

## P2 — mixed-function header on Sample A

Visible order:

```text
IO19
IO11
IO12
IO13
```

| P2 signal | Current use |
|---|---|
| IO19 | GT911/system I2C SDA; same net as P3 IO19 |
| IO11 | SD MOSI while SD is active |
| IO12 | SD SCK while SD is active |
| IO13 | SD MISO while SD is active |

Because IO19 is already GT911 SDA, P2 is **not a self-contained four-wire SPI expansion connector** on this capacitive Sample A. Its 11/12/13 lines are still useful when SD is not used.

## P3 — main signal expansion header

Visible order:

```text
IO17   UART1 silk group
IO18   UART1 silk group
IO19   USB silk group
IO20   USB silk group
```

Practical KONTAKTS interpretation:

```text
IO17 = user GPIO candidate
IO18 = user GPIO candidate
IO19 = system I2C SDA
IO20 = system I2C SCL
```

The UART1 and USB labels describe intended/alternate peripheral functions of the ESP32-S3 pins. They do not override the current platform allocation.

## P4 — GPIO/power header

Visible order:

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

GND/3.3V rail voltage acceptance can be recorded separately.

## Service UART0 / CH340C connector — technological interface

The separate service connector is silked:

```text
5V / TXD0 / RXD0 / GND
```

Sample A continuity measurements include:

```text
R7 <-> CH340C pin 2
R6 <-> CH340C pin 3
```

This connector is classified as a **technological/service programming and debug interface**, shared with the onboard CH340C path. Its TXD0/RXD0 signals are **not counted as free application GPIOs** and should not be presented as normal user expansion ports.

Do not attach another active UART transmitter casually to a line already driven through the CH340C path; treat this header as service/debug infrastructure.

### P1 +5 V power input and Q1 protection

On Sample A, P1 is also a validated board-power input:

```text
P1 +5V ---- D  Q1 CJ3401  S ---- +5V_SYS -> U3/U4
              |
              G
              |
             GND
```

Q1 is a **CJ3401 P-channel MOSFET** in SOT-23, top marking `R1`. The drain is on the P1 +5 V side, the gate is tied to ground, and the source feeds the board's internal +5 V rail used by U3/U4.

Function: **reverse-polarity protection of the board when powered through P1**. At correct polarity the MOSFET turns on and has only a small channel voltage drop. At reversed polarity it stays off and blocks the reverse feed. This circuit does **not** provide over-voltage protection, so P1 remains a nominal +5 V input.

## ADC boundary

No external ADC input is yet guaranteed. GPIO17/18 are ADC-capable at MCU level, but a controlled known-voltage measurement is required before advertising them as analog inputs.

## Validation checklist

- [x] RGB display physically validated;
- [x] GT911 touch physically validated at 0x5D;
- [x] microSD physically validated on GPIO10/11/12/13;
- [x] P2 silk confirmed IO19/IO11/IO12/IO13;
- [x] P3 silk confirmed IO17/IO18/IO19/IO20;
- [x] P4 silk confirmed GND/3.3V/IO17/IO18;
- [x] P3 IO17 <-> P4 IO17 continuity;
- [x] P3 IO18 <-> P4 IO18 continuity;
- [x] GPIO17 <-> WROOM-1 module pin 10 continuity;
- [x] GPIO18 <-> WROOM-1 module pin 11 continuity;
- [x] GPIO18 <-> R17 option path continuity;
- [x] R17 physically DNP/open;
- [x] P3 IO19 <-> P2 IO19 <-> R4 continuity;
- [x] P3 IO20 <-> R3 continuity;
- [x] R7 <-> CH340C pin 2 continuity;
- [x] R6 <-> CH340C pin 3 continuity;
- [x] board powered successfully from P1 +5V/GND;
- [x] Q1 identified as CJ3401 P-MOSF, marking R1;
- [x] Q1 topology traced: D -> P1 +5V, G -> GND, S -> internal +5V rail feeding U3/U4;
- [ ] validate GPIO17 digital input/output;
- [ ] validate GPIO18 digital input/output;
- [ ] validate GPIO11/12/13 as general GPIO with SD disabled if needed;
- [ ] validate an external I2C slave while GT911 remains operational;
- [ ] validate ADC with known voltages;
- [ ] record P4 3.3V rail voltage/current capability;
- [ ] validate shared SPI with SD only if a real project requires it.

## Canonical practical conclusion

```text
SYSTEM I2C
  GPIO19 = SDA, GT911 resident slave
  GPIO20 = SCL, GT911 resident slave

PRIMARY USER GPIO
  GPIO17
  GPIO18

ADDITIONAL USER GPIO WHEN SD IS DISABLED
  GPIO11
  GPIO12
  GPIO13

TECHNOLOGICAL / SERVICE ONLY
  TXD0 / RXD0 through CH340C service path
```

So Sample A gives **I2C + two primary user GPIOs**, and **three more candidate GPIOs when microSD is deliberately not used**. GPIO19/20 remain occupied by the touch I2C bus regardless of SD presence.