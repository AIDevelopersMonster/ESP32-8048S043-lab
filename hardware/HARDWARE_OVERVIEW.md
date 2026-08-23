# ESP32-8048S043 — photographed hardware overview

This document records hardware evidence obtained from the physically inspected ESP32-8048S043 board. The goal is to separate facts visible on the PCB from assumptions derived from similar boards.

## Identification

| Item | Observation | Status |
|---|---|---|
| PCB marking | `ESP32-8048S043` | **PHOTO VERIFIED** |
| MCU module | Espressif `ESP32-S3-WROOM-1` | **PHOTO VERIFIED** |
| Display interface | 40-position FPC, silkscreen `LCD1` | **PHOTO VERIFIED** |
| Storage | microSD/TF socket, silkscreen `TF1 V1.2` | **PHOTO VERIFIED** |
| USB connector | USB-C, silkscreen `USB1` | **PHOTO VERIFIED** |

## Exposed connectors

The following labels are directly readable on the photographed PCB.

| Connector | PCB label / GPIO | Interpretation | Confidence |
|---|---|---|---|
| P2 | `SPI`, `IO19 IO11 IO12 IO13` | Four GPIOs exposed and explicitly grouped as SPI | **PHOTO VERIFIED** |
| P3 | `UART1`, `USB`, `IO17 IO18 IO19 IO20` | Four exposed GPIOs; PCB groups them with UART1/USB functions | **PHOTO VERIFIED** |
| P4 | `GND 3.3V IO17 IO18` | Ground, 3.3 V and two GPIOs | **PHOTO VERIFIED** |
| P1 | `5V TXD RXD GND` | 5 V UART/service connector | **PHOTO VERIFIED** |

Note: the table records silkscreen labels, not yet a complete electrical validation of every alternate ESP32-S3 peripheral function.

## Visible board circuitry

The photographs provide useful evidence for the following functional blocks:

- ESP32-S3-WROOM-1 module and PCB antenna area.
- LCD FPC interface (`LCD1`).
- microSD/TF socket.
- USB-C connector (`USB1`).
- IC references `U1` through `U6` visible in the photographed areas.
- power/regulator circuitry around U3/U4/U5/U6.
- transistor references including `Q1` and `T1`/`T2`.
- diode references including `D1`/`D2`.
- solder jumpers `JP1` and `JP2`.
- test pads `S1`, `S2`, `S3`.

## Component identification policy

Component identity is **not** inferred solely from package shape or from schematics of visually similar ESP32-8048S043 variants. Until the top marking, surrounding circuit, PCB routing, datasheet pinout, or an electrical measurement provides adequate evidence, a device remains `UNCONFIRMED`.

| Ref. | Visible evidence | Identification | Status |
|---|---|---|---|
| ESP module | Shield marking readable | ESP32-S3-WROOM-1 | **PHOTO VERIFIED** |
| U1 | TSSOP/SSOP device near TF/LCD routing; top marking partly visible | To be decoded | **UNCONFIRMED** |
| U2 | Multi-pin IC near USB/power area | To be decoded | **UNCONFIRMED** |
| U3 | Power-package IC, marking partly visible | To be decoded | **UNCONFIRMED** |
| U4 | Power-package IC | To be decoded | **UNCONFIRMED** |
| U5 | Power-package IC, marking partly visible | To be decoded | **UNCONFIRMED** |
| U6 | Small regulator/power IC near L1 | To be decoded | **UNCONFIRMED** |

## Why the photographs matter

These photographs are primary evidence for this repository. They establish the actual PCB variant under test and reduce the risk of silently applying pinouts or schematics from another ESP32-8048S043 revision.

The next hardware-analysis steps are:

1. preserve the original photographs under `hardware/images/`;
2. decode readable IC top markings from macro photographs;
3. correlate each candidate component with package, routing and passive components;
4. verify exposed GPIOs and power rails electrically;
5. promote only verified findings into the BSP/pinout documentation.

## Evidence status vocabulary

- **PHOTO VERIFIED** — directly readable/observable in a physical-board photograph.
- **ELECTRICALLY VERIFIED** — confirmed by measurement or working hardware test.
- **SOFTWARE VERIFIED** — confirmed by a reproducible firmware test on the physical board.
- **UNCONFIRMED** — plausible or partially visible, but evidence is insufficient for a definitive claim.
