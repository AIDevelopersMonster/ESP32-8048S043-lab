# App09 SD Web Help System

## Purpose

App09 documentation is split into two independent layers and is intended for a phone or desktop browser, not the 800x480 LVGL display.

```text
SYSTEM HELP                          APPLICATION HELP
/sd/wiki/system/...                 /sd/widgets/<package>/html/...

User guide                          package-specific help
Application programmer guide       controls / screens / setup
System programmer guide            package notes / troubleshooting
Hardware reference
```

The platform firmware provides only a safe HTTP file-serving capability. Documentation content lives on SD and can be updated without reflashing the ESP32.

## Browser routes

```text
http://<board-ip>/help
http://<board-ip>/help/system/
http://<board-ip>/help/system/user/
http://<board-ip>/help/system/application-programmer/
http://<board-ip>/help/system/system-programmer/
http://<board-ip>/help/system/hardware/
http://<board-ip>/help/apps/<package>/
```

## SD layout

```text
SD:/
├── wiki/
│   ├── index.html
│   ├── assets/
│   │   └── style.css
│   └── system/
│       ├── index.html
│       ├── user/index.html
│       ├── application-programmer/index.html
│       ├── system-programmer/index.html
│       └── hardware/index.html
└── widgets/
    └── <package>/
        ├── package.json
        ├── *.json
        └── html/
            └── index.html
```

## Package contract

A package may advertise its browser documentation with:

```json
"documentation": {
  "entry": "html/index.html"
}
```

The documentation entry is not an LVGL entrypoint and must never appear as a button on the board display.

## System documentation roles

### User guide

Initial setup, flashing, Wi-Fi, SD application installation, launching applications, OTA/recovery and troubleshooting.

### Application programmer guide

`package.json`, JSON widget schema, entrypoints, bindings, generic capabilities, service dependencies, SD-only application rules and examples.

### System programmer guide

ESP-IDF platform architecture, SYS/SD/WIDGET boundary, boot/recovery, partitioning, OTA state machine, HTTP APIs, SD manager, Widget Runtime, services/providers and rules for adding new platform capabilities.

### Hardware reference

Named board/specimen identity, connectors, power, verified GPIO map, LCD RGB bus, GT911 touch, SD SPI, USB/UART, flash/PSRAM and links to authoritative datasheets/reference manuals.

## Invariant

The board display remains `SYS | SD | WIDGET`. Browser documentation is a separate platform capability. Adding or editing HTML must not require a firmware rebuild.
