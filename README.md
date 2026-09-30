# ESP32-8048S043 / KONTAKTS Platform

KONTAKTS is a physically validated ESP32-S3 platform for the **ESP32-8048S043** 4.3-inch 800x480 RGB touch board. The current release combines display/touch, SD applications, Wi-Fi, local Web control, OTA/rollback, technological UART, field RS485/Modbus, BLE, Android control and the Climate Controller application.

## Current stable release

**KONTAKTS Platform 0.4.0**  
**Reference board:** ESP32-8048S043 Sample A  
**Status:** **PHYSICAL + WEB PASS**

Current Climate Controller path:

```text
EID041 temperature/humidity sensor
        |
        v
UART1 GPIO17/18 -> RS485 / Modbus RTU
        |
        v
ESP32-S3 climate service
        |
        v
Ebyte MA01-XXCX0080
        |
        +--> DO1 HEATER
        +--> DO2 COOLER
        +--> DO3 HUMIDIFIER
        +--> DO4 DEHUMIDIFIER
```

User interfaces share one command/service architecture:

```text
SD HMI --------+
Web browser ---+
UART0/P1 ------+
BLE -----------+--> command/service layer --> providers --> hardware
Android -------+
```

Device register knowledge stays in providers/services and is not duplicated by each UI.

## Install

Start here:

**[Quick Start](docs/QUICKSTART.md)**

Preferred first installation: the public **Web Flasher** generated from `web-flasher/`.

Current full image:

```text
app06-ota-recovery-v0.4.0-full.bin
SHA-256:
F2787661FD55A8A6DA1CD8131B644D3C874D89182D1CC5A7F057255A777893B0
```

Current OTA image:

```text
app06-ota.bin
SHA-256:
F5A494AB887BA010C130C5AFB29F1478FE42BB4D8384B5F136A414842DD554B6
```

Release tag:

```text
app06-v0.4.0
```

For an already configured board, prefer OTA over rewriting the complete flash image.

## SD applications

Current mutable SD distribution:

```text
app09-sd-current
kontakts-sd-library.zip
```

Extract the ZIP directly into the root of a FAT32 SD card.

The Climate Controller package is also available separately as:

```text
widget-climate-controller.zip
```

The verified on-card path is:

```text
widgets/climate-controller/
```

## Climate Controller

Default limits:

```text
Temperature MIN   25.0 C
Temperature MAX   30.0 C
Temperature HYST   0.5 C

Humidity MIN      45 %RH
Humidity MAX      60 %RH
Humidity HYST      2 %RH
```

AUTO ownership:

```text
AUTO ON  -> Climate Controller reserves MA01
AUTO OFF -> external relay control is released
```

The controller rejects malformed Modbus data, retains only confirmed state, and uses a 60-second stale-data grace interval for transient sensor/relay read failures. After sensor grace expiry it attempts all-off and enters FAILSAFE.

A broken physical RS485 path cannot guarantee relay de-energization; only fresh readback confirms the actual relay state.

## Web control

On the same trusted local network:

```text
http://<board-ip>/climate
```

The responsive page shows live temperature, humidity, controller mode, sensor/relay state, DO1..DO4 and all six climate limits. It supports AUTO ON/OFF, DEFAULTS and limit editing through the same climate service used by the SD HMI and other transports.

## Field RS485 reference

```text
UART1 TX GPIO17
UART1 RX GPIO18
9600 8N1
automatic-direction TTL/RS485 transceiver
```

Validated devices:

```text
EID041 sensor              slave 1
Ebyte MA01-XXCX0080        slave 16 on Sample A
```

P1/CH340C/UART0 remains the technological/service interface and is not the field Modbus port.

## OTA / rollback

The platform uses dual OTA slots and a confirm-or-rollback model:

```text
NEW -> PENDING_VERIFY -> VALID
```

The 0.4.0 candidate was written to the inactive slot, flash digest verified, exercised through the physical Climate Controller and Web UI, then confirmed VALID.

## Android BLE client

Native Android client:

```text
apps/17_MobileControl/
release: app17-v0.1.1
minSdk: 23
```

The tested end-to-end path is:

```text
Android -> BLE -> command/service -> MA01 provider -> UART1/RS485 -> relay
```

## Repository map

```text
apps/06_OTARecovery/       canonical platform core
apps/09_SDWidgetLibrary/   SD applications and Help
apps/17_MobileControl/     Android BLE client
apps/18_ClimateController/ climate controller
boards/                    Arduino IDE board profile
libraries/ESP32_8048S043/  Arduino library line
hardware/                  board research and schematics
evidence/                  physical acceptance evidence
tools/                     desktop/service tools
web-flasher/               browser installer
docs/                      manuals, release and engineering documentation
```

Older numbered applications are retained as development history where they still help explain the platform. They are not separate current products.

## Documentation

- [Quick Start](docs/QUICKSTART.md)
- [Productization programme](docs/PRODUCTIZATION-PLAN.md)
- [Release protocol](RELEASES.md)
- [Release asset inventory](docs/RELEASE-ASSET-INVENTORY.md)
- [Security](SECURITY.md)
- [Roadmap](ROADMAP.md)
- [Branch hygiene](docs/BRANCH-HYGIENE.md)
- [Climate Controller](apps/18_ClimateController/README.md)

## Evidence policy

1. Identify hardware before flashing.
2. Separate source-backed claims from physical PASS.
3. Name the specimen and firmware for physical evidence.
4. Do not promote a candidate without an observation, log, photo or video.
5. Keep current user documentation free of stale experimental status.
6. Preserve important research history through Git history/evidence/archive tags rather than an ever-growing active branch list.

## Current boundaries

Platform 0.4.0 is a physically validated laboratory/product baseline for the documented Sample A hardware.

Not claimed:

- industrial or functional-safety certification;
- production public-Internet security;
- universal compatibility with every ESP32-8048S043 revision;
- guaranteed physical relay OFF when the RS485/relay path itself is unavailable.

See [SECURITY.md](SECURITY.md) for deployment boundaries.

## License

Original code and text are released under the repository license. Third-party firmware, photos, schematics and source material retain their own licenses and are not redistributed unless permitted.
