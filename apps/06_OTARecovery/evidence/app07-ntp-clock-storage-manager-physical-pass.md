# App07 NTP Clock + Storage Manager Physical Pass

Date: 2026-09-08
Branch: `agent/app07-widget-runtime`
Board: ESP32-8048S043 / ESP32-S3 / 800x480 / GT911

## Result

PHYSICAL PASS for the NTP seven-segment clock widget installation/runtime and browser Storage Manager visibility/protection behavior.

The user confirmed on real hardware that the clock works and supplied the browser state below.

## Filesystem Widget evidence

- Installed: yes
- ID: `clock.ntp-sevenseg`
- Name: `NTP Seven Segment Clock`
- Version: `1.0.0`
- File: `1331 bytes`
- Generation: `3`
- Status: `INSTALL PASS`
- SPIFFS: `2761 / 5775761 bytes used`

This physically confirms that the clock widget passed validation, replaced `/storage/widget.json`, was published by Widget Runtime, and operated on the target board.

## Storage Manager evidence

The browser Storage Manager reported:

- `platform.cfg` — 121 bytes — user — downloadable/deletable
- `ui-settings-screen.cfg` — 121 bytes — user — downloadable/deletable
- `widget.json` — 1331 bytes — `system / protected`

This physically confirms that the generic Storage Manager can enumerate `/storage` while classifying the active widget file as a protected system file. Widget installation remains routed through the validated Widget section rather than generic file replacement.

## Functional clock confirmation

User report: `Отлично все работает`.

The tested clock application includes:

- seven-segment time display;
- NTP synchronization support;
- calendar/date presentation;
- weekday/year presentation;
- manual time synchronization action;
- filesystem installation as an external widget rather than firmware-resident application code.

## Video evidence

YouTube Short recorded on physical hardware:

https://youtube.com/shorts/m0YZcbebFiU

## Gate status

- Widget JSON validation/install: PASS
- Clock widget runtime on physical display: PASS
- Storage enumeration: PASS
- Protected `widget.json` classification: PASS
- Browser-managed external application path: PASS

This evidence does not by itself certify every App07 regression gate (OTA rollback/recovery, every generic file operation, or all failure-path tests); it closes the clock-widget + storage-manager physical functionality demonstrated above.
