# ESP32-8048S043 Lab / KONTAKTS Platform

Evidence-first hardware, firmware and application laboratory for the **ESP32-8048S043 / ESP32-8048S043C-I** family of ESP32-S3 4.3-inch 800x480 RGB touch modules.

The repository started as a board-identification and hardware-validation lab and has grown into a physically tested platform with display/touch, SD applications, Wi-Fi, Web control, OTA, technological UART, field RS485/Modbus, BLE and an Android client.

## Current milestone

**Current accepted platform:** KONTAKTS Platform **0.3.9**  
**Board:** Sample A  
**Status:** **PHYSICAL PASS**  
**Accepted full-image SHA-256:**

```text
20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

The physically accepted integrated path includes:

```text
800x480 RGB + GT911
SD application library + Help
Wi-Fi STA/AP + HTTP
GitHub OTA / rollback infrastructure
UART0/P1 technological service
UART1 GPIO17/18 field bus
RS485 / Modbus RTU
MA01 relay provider
Web control
BLE GATT transport
Android BLE client
```

The accepted control architecture is deliberately transport-independent:

```text
HMI -----------+
UART0/P1 ------+
Web browser ---+
BLE -----------+--> command/service layer --> providers --> hardware
Android app ---+
```

MA01 Modbus register knowledge remains in the provider/service layer. Web, BLE, Android and UART clients do not duplicate the register map.

## Major result: Wi-Fi + BLE coexistence

Platform 0.3.9 physically demonstrates **Wi-Fi and BLE operating in the same ESP32-S3 system** together with RGB/LVGL, SD, HTTP, OTA services and MA01/RS485 control.

The bring-up problems were memory/resource-allocation issues, not a fundamental Wi-Fi/BLE incompatibility. The accepted build uses explicit memory ownership:

- display/LVGL initialization before optional BLE;
- LVGL draw buffer in PSRAM;
- NimBLE host allocations in external RAM where supported;
- OTA worker stack in PSRAM;
- RGB bounce buffer kept in internal DMA-capable RAM.

This is a hardware-observed coexistence result, not a claim that every Wi-Fi/BLE workload is automatically contention-free.

## Current user paths

### Web Flasher

The current Web Flasher catalog publishes Platform 0.3.9 as:

```text
PHYSICAL PASS / CURRENT PLATFORM
```

Source:

```text
web-flasher/
```

Release image:

```text
app06-v0.3.9
app06-ota-recovery-v0.3.9-full.bin
```

Accepted full-image SHA-256:

```text
20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

### GitHub OTA

Manifest:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
```

Platform 0.3.9 OTA assets are published under release:

```text
app06-v0.3.9
```

### SD application library and Help

Current SD snapshot:

```text
app09-sd-current
kontakts-sd-library.zip
```

The SD Help system covers four roles:

```text
User
Application programmer
System programmer
Hardware
```

Canonical browser paths include:

```text
/help
/help/system?doc=user
/help/system?doc=application-programmer
/help/system?doc=system-programmer
/help/system?doc=hardware
/help/app?name=<package-folder>
```

### Technological UART

P1 / CH340C / UART0 is the technological/service transport:

```text
115200 8N1
```

Host tool:

```text
tools/KONTAKTSerial/
```

UART0 is not the user field Modbus port.

### Field RS485 / MA01

Field transport:

```text
UART1 TX GPIO17
UART1 RX GPIO18
9600 8N1
automatic-direction TTL/RS485 adapter
```

Physically validated target:

```text
Ebyte MA01-XXCX0080
slave address 16
```

Examples of the common textual service API:

```text
MA01 ADDR
MA01 READ
MA01 CONFIG
MA01 DO1 INFO
MA01 DO1 ACTION
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 TOGGLE
MA01 DO1 MODE LEVEL
MA01 DO1 MODE PULSE
MA01 DO1 PULSEMS 5000
```

### BLE

Platform 0.3.9 BLE contract:

```text
Device   KONTAKTS-8048
Service  FFF0
Command  FFF1  WRITE
Response FFF2  READ
```

The current laboratory MVP supports one BLE connection and deliberately uses explicit reads rather than notifications.

### Android App17

Native Android client:

```text
apps/17_MobileControl/
```

Current physically tested MVP:

```text
App17 v0.1.1
release tag: app17-v0.1.1
minSdk 23
Android 6.0+
physical test: Android 7.1.2 / API 25
APK SHA-256:
5DE40BC802438A35285077241DA603B0441EAD28179928FAD0E15EA0285868D0
```

Confirmed end-to-end:

```text
Android App17
 -> BLE FFF1 / FFF2
 -> Platform 0.3.9 command/service layer
 -> MA01 provider
 -> UART1 GPIO17/18
 -> RS485
 -> MA01
 -> physical relay ON/OFF
```

Permanent APK:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/download/app17-v0.1.1/kontakts-mobile-app17-v0.1.1-debug.apk

Video:

https://youtube.com/shorts/FxDnALva3xM

## Application line

The repository keeps incremental application/laboratory stages rather than hiding earlier development.

| App | Purpose | Current role |
|---|---|---|
| 01 | Six Card Serial Deck | physical-pass laboratory firmware |
| 02 | Mixed Widgets | closed physical-pass LVGL lab |
| 03 | Live Dashboard | closed physical-pass telemetry lab |
| 04 | Storage Config | storage/config development stage |
| 05 | Network Provisioning | physical-pass legacy network lab |
| 06 | OTA Recovery / platform core | current platform foundation |
| 09 | SD Widget Library | current SD apps + Help distribution |
| 10 | BLE Slider Sync | earlier BLE laboratory stage |
| 11 | USB Serial Terminal | serial UI stage |
| 14 | Modbus Controller | generic field-bus/service development record |
| 15 | Web Control | physical MVP pass |
| 16 | BLE Control | physical pass / Platform 0.3.9 |
| 17 | Mobile BLE Control | physical MVP pass / Android |

Some historical stage numbers are intentionally absent from `main/apps` because the useful implementation was folded into the platform line rather than preserved as a separate top-level application.

## Hardware baseline — Sample A

Current core hardware evidence:

```text
ESP32-S3                     PASS
Flash                        16 MB
PSRAM                        8 MB
RGB display                  800x480 PHYSICAL PASS
GT911                        0x5D / PHYSICAL PASS
SD SPI                       GPIO10/11/12/13 PHYSICAL PASS
P1                           +5V + UART0/CH340C service path
P4                           GND / 3.3V / GPIO17 / GPIO18
GPIO17/18                    UART1 field path PHYSICAL PASS
Q1                           CJ3401 P-channel reverse-polarity protection
```

Board operating current observed around:

```text
~0.64 A @ 5 V
~3.17 W
```

An ordinary PC USB 2.0 port is not treated as the recommended normal power source for the integrated platform.

## Factory baseline

Sample A factory dump was read twice over the full 16 MB flash and matched:

```text
SHA-256
3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
```

The proprietary factory binary is intentionally not committed. Metadata, hashes and reviewed analysis are preserved instead.

## Evidence policy

This repository remains evidence-first:

1. **Identify before flashing.**
2. **Separate source-backed claims from physical PASS.**
3. **Name the specimen and firmware for physical evidence.**
4. **Keep failed experiments as history when useful, but do not leave stale status in the current README.**
5. **Do not promote a candidate to PASS without a hardware observation, log, photo or video.**
6. **Do not duplicate hardware/protocol knowledge across transports.**

## Repository map

```text
apps/                       incremental firmware/platform/mobile stages
boards/                     Arduino IDE board-profile work
config/                     machine-readable board profiles
docs/                       hardware/software/manuals/research
evidence/                   named physical evidence
hardware/                   schematic/BOM/photo research
libraries/ESP32_8048S043/   Arduino BSP line
tools/                      host tools, flasher, analysis
web-flasher/                ESP Web Tools catalog/site
.github/workflows/          CI, releases and deployment
```

Important starting points:

- `apps/06_OTARecovery/README.md` — current platform/OTA foundation.
- `apps/09_SDWidgetLibrary/README.md` — SD application architecture.
- `apps/14_ModbusController/README.md` — field-bus development record.
- `apps/15_WebControl/README.md` — browser control.
- `apps/16_BLEControl/README.md` — BLE transport and 0.3.9 acceptance.
- `apps/17_MobileControl/README.md` — Android BLE client.
- `tools/KONTAKTSerial/README.md` — technological UART tool.
- `docs/HARDWARE-ACCEPTANCE-START.md` — board acceptance workflow.
- `hardware/SCHEMATIC_BOM_RESEARCH.md` — reconstructed hardware evidence.
- `web-flasher/firmware-list.json` — current public firmware/application catalog.

## Current boundaries / next work

The large **transport/platform proof stage is complete**. The next work should be refinement rather than another duplicated control stack:

```text
App17 UI/state parsing
BLE reconnect behaviour
optional notifications/security design
Web/Modbus polling cleanup
long-duration coexistence tests
generic external-device/provider expansion
documentation/release hygiene
```

Not yet claimed:

- production BLE security;
- long-duration multi-client/multi-device operation;
- universal external I2C/ADC use on every board revision;
- guaranteed GPIO current capability beyond the documented board evidence;
- production certification.

## License

Original code and text are released under the repository license. Third-party firmware, photos, schematics and source material retain their own licenses and are not redistributed unless permitted.
