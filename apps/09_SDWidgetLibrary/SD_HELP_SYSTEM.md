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

The platform firmware provides a safe read-only HTTP file-serving capability. Documentation content lives on SD and can be updated without reflashing the ESP32.

## Browser routes

The first implementation intentionally uses four exact handlers so it stays inside the current ESP-IDF HTTP handler budget without enabling wildcard routing:

```text
http://<board-ip>/help
http://<board-ip>/help/system?doc=user
http://<board-ip>/help/system?doc=application-programmer
http://<board-ip>/help/system?doc=system-programmer
http://<board-ip>/help/system?doc=hardware
http://<board-ip>/help/app?name=<package-folder>
http://<board-ip>/help/style.css
```

`/help` is generated dynamically. It always shows the system documentation cards and then discovers application documentation declared by SD package manifests.

## SD layout

```text
SD:/
├── wiki/
│   ├── index.html
│   ├── assets/style.css
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
        └── html/index.html
```

`wiki/index.html` is also useful when the SD card is opened directly on a PC. The normal network landing page is the dynamic `/help` catalog.

## Package contract

A package advertises its browser documentation with:

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

## Browser security boundary

Application HTML is user-controlled SD content served on the same HTTP origin as platform control APIs. Help responses therefore apply a restrictive Content Security Policy: scripts, forms, frames, objects and network connections are disabled. Help HTML is documentation, not an executable application surface.

## Invariant

The board display remains `SYS | SD | WIDGET`. Browser documentation is a separate platform capability. Adding or editing HTML must not require a firmware rebuild once platform 0.3.3 or later is installed.
