# Quick Start — KONTAKTS Platform 0.4.0

This guide is the shortest supported path from a blank ESP32-8048S043 Sample A board to the current Climate Controller platform.

## 1. Supported reference hardware

Validated reference:

```text
ESP32-8048S043 / ESP32-S3
800x480 RGB display
GT911 touch
16 MB flash
8 MB PSRAM
Sample A pinout
```

Field RS485:

```text
UART1 TX GPIO17
UART1 RX GPIO18
9600 8N1
automatic-direction TTL/RS485 transceiver
```

Validated devices:

```text
EID041 temperature/humidity sensor   slave 1
Ebyte MA01-XXCX0080 relay module     slave 16 on Sample A
```

## 2. Install firmware

Preferred path: the GitHub Pages Web Flasher from the repository's public site.

Choose:

```text
KONTAKTS Platform / Climate Controller + Web
Version 0.4.0
```

The current full image is:

```text
app06-ota-recovery-v0.4.0-full.bin
SHA-256:
F2787661FD55A8A6DA1CD8131B644D3C874D89182D1CC5A7F057255A777893B0
```

For an already configured board, prefer OTA rather than rewriting the full image.

## 3. Install SD content

Download the current SD bundle from release:

```text
app09-sd-current
kontakts-sd-library.zip
```

Extract the ZIP directly into the root of a FAT32 SD card.

Expected top-level directories include:

```text
UPDATE/
widgets/
wiki/
```

Climate Controller package:

```text
widgets/climate-controller/
```

Do not flash the SD ZIP to the ESP32.

## 4. First boot

The platform boots with Climate AUTO disabled.

Open the Climate Controller from the SD application launcher and verify that the sensor reports live values.

Default limits:

```text
Temperature MIN   25.0 C
Temperature MAX   30.0 C
Temperature HYST   0.5 C

Humidity MIN      45 %RH
Humidity MAX      60 %RH
Humidity HYST      2 %RH
```

Relay allocation:

```text
DO1 HEATER
DO2 COOLER
DO3 HUMIDIFIER
DO4 DEHUMIDIFIER
```

## 5. Web control

Connect the board to Wi-Fi, then open from a phone or computer on the same trusted local network:

```text
http://<board-ip>/climate
```

The page displays temperature, humidity, controller state, relay state and DO1..DO4, and supports AUTO ON/OFF, DEFAULTS and limit editing.

AUTO ownership rule:

```text
AUTO ON  -> Climate Controller reserves MA01
AUTO OFF -> external relay control is released
```

## 6. OTA updates

The platform uses dual OTA slots with rollback.

Do not confirm a new OTA image until the claimed functions have been checked. A candidate normally progresses:

```text
NEW -> PENDING_VERIFY -> VALID
```

The repository-wide normal `latest` release is reserved for the current platform firmware so the OTA manifest remains stable.

## 7. Recovery

If the current platform is unusable:

1. use the Web Flasher or the known-good recovery image;
2. avoid erasing NVS unless recovery instructions explicitly require it;
3. for configured boards, do not use a full image as a settings-preserving OTA replacement.

## 8. Safety boundary

The current local HTTP and BLE control paths are intended for a trusted laboratory/local network.

A broken RS485 link cannot guarantee physical relay de-energization. Software can attempt FAILSAFE OFF, but only fresh readback confirms the actual relay state. Use independent hardware interlocks/watchdogs where that guarantee is required.

## 9. Developer entry points

```text
apps/06_OTARecovery/       platform core
apps/09_SDWidgetLibrary/   SD applications
apps/18_ClimateController/ climate controller
apps/17_MobileControl/     Android client
tools/                     host/service tools
web-flasher/               browser installer
```

For repository policy and release discipline see:

```text
docs/PRODUCTIZATION-PLAN.md
RELEASES.md
docs/RELEASE-ASSET-INVENTORY.md
```
