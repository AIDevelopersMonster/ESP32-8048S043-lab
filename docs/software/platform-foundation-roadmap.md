# KONTAKTS Platform Foundation Roadmap

Status: ACTIVE

The platform is intentionally being completed before the project layer is expanded to concrete sensors, actuators and end-use controllers.

## Foundation completion sequence

1. Close App 03 as the first real live instrument dashboard.
2. Add persistent settings and configuration storage.
3. Add filesystem-backed configuration/assets with partial loading instead of compiling every future resource into the firmware image.
4. Add Wi-Fi station mode for joining an existing router/network.
5. Add Wi-Fi access-point fallback / first-run setup mode.
6. Add a local web setup surface for network and device configuration.
7. Add OTA application update with recovery-safe behavior.
8. Extend the KONTAKTS Firmware Catalog so a factory/web-flashed device can transition to OTA-managed updates.
9. Define the SD-card content model for large project-specific resources, profiles, histories and optional UI/data packages.
10. Only after the above foundation is stable, expand the `projects/` layer with concrete physical equipment such as thermostats, incubator controllers and greenhouse controllers.

## Architectural rule

Apps validate reusable platform capabilities. Projects solve a complete physical task.

Future project code should consume reusable services rather than directly own board-specific display, touch, storage, network and OTA plumbing.

## Planned platform services

```text
KONTAKTS Core
├── board_runtime
│   ├── display
│   ├── touch
│   ├── backlight
│   └── board identity
├── ui_runtime
│   ├── LVGL lifecycle
│   ├── navigation
│   └── reusable widgets
├── storage
│   ├── NVS settings
│   ├── internal filesystem
│   └── SD content provider
├── network
│   ├── Wi-Fi STA
│   ├── Wi-Fi AP fallback
│   └── web setup
├── update
│   ├── OTA download
│   ├── version policy
│   └── rollback/recovery hooks
└── project services
    ├── sensors
    ├── actuators
    ├── automation/control logic
    └── project-specific UI/content
```

## Storage direction

The firmware should not assume that all future project data belongs in the application binary.

The intended hierarchy is:

- NVS: small durable settings, flags, selected profile and network state;
- internal filesystem: compact UI/configuration resources required without an SD card;
- SD card: large optional project packages, logs, profiles, historical data, media, model descriptions and future dynamically loaded resources.

Partial loading from the filesystem is therefore a platform requirement, not a project-specific optimization.

## Completion boundary

The foundation phase is complete when a device can be installed from the browser, retain settings, load external configuration/resources, join a router, expose an AP fallback/setup path, update itself over OTA, and recover safely without rebuilding the application for every configuration change.
