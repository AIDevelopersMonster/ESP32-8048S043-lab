# App09 — SD Widget Library

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Branch:** `agent/app09-sd-widget-library`  
**Status:** DESIGN / IMPLEMENTATION STARTED

## Goal

App09 extends the physically validated App08 platform with an SD-backed application library while preserving the existing firmware-resident recovery surface.

The first scope is deliberately narrow:

```text
SD
└── widgets
    └── youtube
        ├── package.json
        ├── dashboard.json
        ├── views.json
        └── subscribers.json
```

All YouTube display variants remain in **one YouTube subfolder**. App09 does not yet spread individual views across unrelated SD directories.

## Platform boundary

```text
KONTAKTS Platform firmware
├── STATUS
├── OTA
├── WIDGET
├── Wi-Fi / NVS
├── YouTube service
├── internal /storage rescue/persistence
└── SD Manager            <- App09

SD card
└── widgets/youtube       <- application package
```

The application package supplies presentation. The platform supplies hardware drivers, networking, NVS secrets, service bindings, OTA, rollback and recovery.

## First package

The first SD package is `youtube` and contains three presentations already derived from the App08 physical-pass widgets:

- Dashboard — combined subscribers/views/videos + two charts;
- Views — full-width views chart;
- Subscribers — full-width subscribers chart.

The YouTube API key is **not** stored on SD. It remains in NVS.

## Runtime rule

A selected SD JSON must be read and validated by Widget Runtime. The file should not need to stay open after rendering.

For the first implementation it is acceptable for `RUN` to install the selected validated SD widget into the existing internal active slot:

```text
/sd/widgets/youtube/views.json
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
- STATUS;
- OTA/recovery;
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

App09 must reproduce this mapping in ESP-IDF before any write-capable SD feature is accepted.

## Future package-to-firmware contract

Some future applications may require firmware capabilities that the installed platform does not contain: for example a new hardware driver, protocol stack or firmware-resident service.

The package format is therefore intentionally designed to grow an **optional firmware requirement**:

```text
project package
├── presentation/resources on SD
└── optional firmware requirement
       ├── minimum platform/version
       └── verified OTA manifest URL
```

The intended user flow is:

```text
select project on SD
      |
      v
check package requirements
      |
      +-- compatible platform -> RUN
      |
      +-- firmware capability missing
              |
              v
         offer verified OTA update
              |
              v
         normal PENDING_VERIFY / CONFIRM / ROLLBACK contract
              |
              v
         return to project package
```

This must reuse the platform's verified OTA machinery rather than let an SD package directly flash arbitrary binary data.

For the current YouTube package:

```text
firmware.required = false
minimum platform  = 0.2.8
```

because the required `youtube.*` bindings and chart renderer are already present in the App08 platform.

## Acceptance gates

App09 is not PHYSICAL PASS until real hardware confirms at least:

```text
[ ] SD mounts without formatting
[ ] missing SD leaves platform usable
[ ] /widgets/youtube is enumerated
[ ] Dashboard can be selected and run
[ ] Views can be selected and run
[ ] Subscribers can be selected and run
[ ] selected widget persists internally
[ ] SD can be removed after selection without killing active widget
[ ] reboot without SD restores persisted active widget
[ ] STATUS / OTA / WIDGET remain stable
[ ] YouTube API key never appears on SD
```
