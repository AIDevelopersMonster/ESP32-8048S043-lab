# KONTAKTS Platform concept candidate

Status: **CONCEPT CANDIDATE — requires App09 physical PASS before promotion to project canon**.

## Canonical top-level navigation

```text
SYS | SD | WIDGET
```

### SYS
Firmware-resident system/recovery surface. It must remain usable with no SD card and with no installed widget.

Responsibilities:
- emergency device status;
- firmware version / running partition / image state;
- Wi-Fi and IP diagnostics;
- heap / PSRAM diagnostics;
- SD mount state;
- GitHub OTA;
- OTA confirm / rollback;
- factory recovery;
- later: verified update manifests from approved sources.

The emergency status is deliberately firmware-resident. A richer status application may also exist as an SD widget, but it must not replace the recovery status path.

### SD
External application library and offline update source.

```text
SD/
├── widgets/
│   ├── youtube/
│   ├── status/
│   └── future-app/
└── UPDATE/
    ├── platform/
    └── project-name/
```

Responsibilities:
- mount card without automatic formatting;
- enumerate application packages;
- validate and run selected widget JSON through Widget Runtime;
- keep the last valid active widget in internal `/storage/widget.json` so boot does not depend on SD;
- enumerate verified manual firmware update packages under `UPDATE`;
- never auto-flash merely because a card was inserted.

### WIDGET
The currently active application UI. Application presentation is replaceable without reflashing when the required services already exist in the platform.

## Application/service contract

A future application package should describe *capabilities*, not hard-code one hardware assembly.

Examples of platform services:

```text
time
wifi
youtube
weather
temperature
relay
flow-meter
audio
storage
mqtt
```

Example concept:

```json
{
  "schema": 1,
  "id": "thermostat",
  "name": "Thermostat",
  "version": "1.0.0",
  "requires": {
    "minimum_platform": "0.3.0",
    "services": ["temperature", "relay"]
  },
  "configuration": {
    "temperature_source": "selectable"
  },
  "firmware": {
    "required": false
  }
}
```

The thermostat UI can then select an available temperature provider such as DS18B20, NTC, BME280 or another driver exposed by SYS, without becoming a separate firmware image for every sensor choice.

## Candidate applications

This architecture is intended to make existing and future device projects reusable as application packages where practical:

- YouTube dashboard;
- extended system/status dashboard;
- weather station;
- music station/player;
- thermostat with selectable sensors and relay outputs;
- liquid dispenser / filling controller ("наливатор") using flow-meter, valve/pump and recipe/configuration services;
- MQTT/sensor dashboards;
- other appliance/control panels.

Some applications may require new low-level firmware services. In that case the package may declare a minimum platform version or a verified firmware requirement; the package itself never writes flash.

## Firmware delivery channels

```text
GitHub Release OTA  -> normal network update
SD /UPDATE          -> normal offline/manual update
Web Flasher         -> initial install / recovery
```

All update channels must converge on the same verification policy: board/application compatibility, size, SHA-256, ESP image descriptor/version checks, inactive OTA partition, then PENDING_VERIFY / CONFIRM / ROLLBACK.

## Non-negotiable recovery invariants

1. Missing, unreadable or corrupt SD must not prevent boot.
2. Invalid SD widget must not replace the previous valid internal widget.
3. SD is never formatted automatically.
4. SD firmware update is explicit/manual only.
5. A widget/package cannot directly flash arbitrary firmware.
6. BUILD PASS is not PHYSICAL PASS.
7. This document becomes project canon only after App09 physical validation on the real ESP32-8048S043 hardware.
