# App09 — SD Widget Library

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Branch:** `agent/app09-sd-widget-library`  
**Status:** DESIGN / IMPLEMENTATION STARTED

## Goal

App09 extends the physically validated App08 platform with an SD-backed application library while preserving a firmware-resident recovery surface.

The canonical navigation candidate for App09 is:

```text
SYS | SD | WIDGET
```

- `SYS` — emergency status, diagnostics, OTA, rollback and recovery;
- `SD` — application library and manual offline update source;
- `WIDGET` — currently active application UI.

This navigation becomes project canon only after physical validation on the real ESP32-8048S043 hardware.

## Canonical application rule

The firmware shell must **not** contain application-specific launch buttons or application names. The platform knows only `SYS | SD | WIDGET`, the package contract and platform services/capabilities.

Every compatible application is installed by copying a package directory to SD:

```text
SD/
├── widgets/
│   ├── youtube/
│   ├── clock/
│   ├── youtube-led/
│   ├── nalivator/
│   └── thermostat/
└── UPDATE/
    ├── platform/
    └── project-name/
```

`SD` scans `/sd/widgets/*/package.json`, builds its launcher dynamically from declared `entrypoints`, and starts the selected JSON through Widget Runtime. Therefore adding another compatible application must require **no firmware rebuild and no new C button**.

A platform firmware update is justified only when an application requires a service/provider/driver or generic UI capability that the installed platform does not yet expose.

The earlier experimental build that added a dedicated `CLOCK` launcher in firmware is explicitly **non-canonical**. It proved that a second package could be carried on SD, but App09 replaces that pattern with manifest-driven discovery.

## Platform boundary

```text
KONTAKTS Platform firmware
├── SYS
│   ├── emergency status
│   ├── GitHub OTA
│   ├── CONFIRM / ROLLBACK
│   └── factory recovery
├── SD Manager
│   └── manifest-driven application catalog
├── SD Launcher
│   └── dynamic entrypoints from package.json
├── WIDGET Runtime
├── generic UI capabilities
│   └── metric_carousel
├── Wi-Fi / NVS
├── platform services/providers
└── internal /storage rescue/persistence

SD card
├── widgets/...           <- application packages
└── UPDATE/...            <- explicit/manual offline update packages
```

The application package supplies presentation and declares required capabilities. The platform supplies hardware drivers, networking, NVS secrets, service bindings, OTA, rollback and recovery.

## Demonstration package — YouTube LED Carousel

Platform `0.3.1` adds a generic `metric_carousel` object. It is not YouTube-specific: any compatible widget can cycle allowed bindings at a declared interval.

The demonstration SD package is:

```text
widgets/youtube-led/
├── package.json
└── main.json
```

Its first demo cycles every 5 seconds through:

```text
Subscribers -> Views -> Videos -> Time -> ...
```

The physical demonstration procedure is intentionally SD-only after installing platform `0.3.1`:

```text
1. boot platform without widgets/youtube-led
2. open SD and show that YouTube LED Carousel is absent
3. copy only widgets/youtube-led to the SD card
4. press MOUNT / RESCAN
5. launcher discovers YouTube LED Carousel from package.json
6. run it from SD
7. observe metrics changing automatically every 5 seconds
```

This is the acceptance proof that a new compatible application can be added by copying files to SD without adding a firmware application button.

## SYS recovery invariant

The old separate top-level `STATUS` and `OTA` concepts are folded into `SYS`.

A small emergency status remains firmware-resident and must work even if the SD card is missing, unreadable or corrupt. A richer `status` application may later be supplied as an SD widget, but it must not replace the emergency recovery path.

Minimum firmware-resident SYS information should include:

```text
firmware version
running partition
image state
Wi-Fi state / IP
SD state
heap / PSRAM
OTA state
```

## Package examples

The first SD package is `youtube` and contains three presentations already derived from the App08 physical-pass widgets:

- Dashboard — combined subscribers/views/videos + two charts;
- Views — full-width views chart;
- Subscribers — full-width subscribers chart.

The second package is `clock`, using the already proven NTP seven-segment clock widget. It exists specifically to prove that application discovery is independent of YouTube.

The third demonstration package is `youtube-led`, using the generic metric carousel capability.

The YouTube API key is **not** stored on SD. It remains in NVS.

## Runtime rule

A selected SD JSON must be read and validated by Widget Runtime. The file does not need to stay open after rendering.

```text
/sd/widgets/<package>/<entrypoint>.json
      |
      | read + validate
      v
Widget Runtime
      |
      | persist active copy
      v
/storage/widget.json
```

This gives an important failure property: removing the SD card after a successful selection does not destroy the currently active widget.

## Missing/dead SD rule

SD is optional application storage. Failure to mount SD must not block:

- boot;
- SYS / recovery;
- Wi-Fi provisioning;
- the currently persisted `/storage/widget.json`.

No automatic formatting of the user's SD card is permitted.

## Pins

Use the already physically tested SD SPI mapping for this board family:

```text
CS   = GPIO10
MOSI = GPIO11
CLK  = GPIO12
MISO = GPIO13
```

Initial App09 frequency is `10 MHz`, matching the prior physical read-only SD test.

## Physical evidence

### 1. First SD integration

The following short video records the hardware stage where the SD card was first added to the ESP32-8048S043 project and tested on the real board:

- **Video:** [ESP32-8048S043 — first SD integration](https://youtube.com/shorts/TMS2s1jirdw)
- **Scope:** physical SD integration baseline; this video documents the stage before the later App09 SD application-library work and should not be treated as a full App09 acceptance test.

### 2. SD application-library demonstration

The following short records the next physical milestone: the board is already working with the SD application library and the SD launcher is being demonstrated on real hardware.

- **Video:** [ESP32-8048S043 — working with the SD application library](https://youtube.com/shorts/FdH1dvEePZg)
- **Scope:** demonstrates the App09 stage where the SD card is mounted and application entries from the SD library are presented for selection/launch. This is the bridge between the basic SD hardware test above and the upcoming SD-only `youtube-led` package demonstration.

## Application/service contract

Applications depend on platform **services/capabilities**, not on one hard-coded sensor assembly.

Examples:

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

A thermostat package, for example, can require `temperature + relay` while allowing the temperature provider to be selected from available implementations such as DS18B20, NTC or BME280.

The package should not need a separate firmware image for each supported sensor if the installed platform already exposes the matching provider.

This model is intended for:

- YouTube dashboards;
- clocks;
- weather station;
- music station/player;
- thermostat with selectable sensors and outputs;
- liquid dispenser / filling controller ("наливатор") using flow-meter, valve/pump and recipe services;
- MQTT and generic sensor dashboards.

## Package-to-firmware contract

Some future applications may require capabilities that the installed platform does not contain. The package format therefore supports an **optional firmware requirement**.

The intended user flow is:

```text
copy project package to SD
      |
      v
MOUNT / RESCAN
      |
      v
read package.json + entrypoints
      |
      v
check package requirements
      |
      +-- compatible platform -> show in launcher -> RUN
      |
      +-- capability missing
              |
              v
         offer verified platform update
              |
              +-- GitHub manifest
              |
              +-- SD /UPDATE package
              |
              v
         normal PENDING_VERIFY / CONFIRM / ROLLBACK
              |
              v
         return to project package
```

The widget/package itself never writes flash directly.

## Firmware delivery channels

```text
GitHub Release OTA  -> normal network update
SD /UPDATE          -> normal offline/manual update
Web Flasher         -> initial install / recovery
```

All firmware paths must converge on the same verification policy: board/application compatibility, size, SHA-256, ESP image descriptor/version checks, inactive OTA slot, then `PENDING_VERIFY / CONFIRM / ROLLBACK`.

## Acceptance gates

App09 is not PHYSICAL PASS until real hardware confirms at least:

```text
[ ] top navigation is SYS | SD | WIDGET
[ ] SYS remains usable without SD
[ ] SD mounts without formatting
[ ] missing SD leaves platform usable
[ ] launcher is generated from /widgets/*/package.json
[ ] no application-specific launcher buttons exist in firmware
[ ] YouTube package entrypoints appear dynamically
[ ] Clock package appears dynamically without adding a firmware button
[ ] youtube-led appears after copy + RESCAN without reflashing
[ ] metric carousel changes value every 5 seconds
[ ] arbitrary new compatible package appears after copy + RESCAN without reflashing
[ ] selected widget can be run
[ ] selected widget persists internally
[ ] SD can be removed after selection without killing active widget
[ ] reboot without SD restores persisted active widget
[ ] SD /UPDATE package is discovered
[ ] SD update requires explicit user action
[ ] bad SHA / wrong board update is rejected
[ ] successful SD update enters normal PENDING_VERIFY flow
[ ] rollback remains functional
[ ] secrets never appear on SD
```
