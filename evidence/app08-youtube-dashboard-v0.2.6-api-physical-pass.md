# App08 YouTube Dashboard v0.2.6 — API physical pass

Date: 2026-09-09
Branch: `agent/app08-youtube-dashboard`
Firmware: `0.2.6`
Scope: physical validation of the App08 network-data service and `/youtube` web dashboard. This is **not** yet a physical pass for the final TFT YouTube widget/chart UI.

## Board evidence

ESP32-8048S043 / ESP32-S3 booted the 0.2.6 App08 candidate and remained stable through UI, Wi-Fi provisioning and YouTube API use.

Observed runtime evidence from the board:

- PSRAM detected and passed memory test.
- SPIFFS `/storage` mounted.
- Existing external NTP seven-segment widget autoloaded and rendered.
- App08 YouTube history buffer allocated in PSRAM: `11712` bytes.
- LVGL draw buffer allocated successfully: `51200` bytes / 32 lines internal RAM.
- Platform shell reached `PLATFORM:READY`.
- Wi-Fi provisioning connected to the home WLAN and obtained STA address `192.168.1.71`.
- NTP synchronized successfully.
- Opening `/youtube` no longer caused the previous HTTP-task reboot after moving the HTML page buffer off the HTTP task stack.

## YouTube API result

The local dashboard reported:

- Channel ID: `UCplLC3QnAagQq2hw1G3RLvQ`
- Configured: `yes`
- State: `READY`
- Message: `YouTube statistics updated`
- History: `1 days`
- Subscribers: `13400`
- Views: `9138697`
- Videos: `6376`
- Selected period: `7D`
- Views delta: `0`
- Subscribers delta: `0`

The zero deltas are expected on the first stored daily sample.

## Video evidence

Published physical demonstration:

- https://youtube.com/shorts/cjgx2RB0l_A

The video documents the App08 YouTube statistics/API and local dashboard stage. It does not by itself close the still-open final TFT widget/chart validation gate.

## Security note

The API key was entered through the ESP32 local `/youtube` page and stored in NVS. The key is not recorded in this evidence file, source tree or widget JSON.

## Verdict

`APP08 YOUTUBE SERVICE + LOCAL WEB DASHBOARD: PHYSICAL PASS`

Validated:

1. stable App08 boot;
2. Wi-Fi provisioning and STA operation;
3. NTP availability;
4. `/youtube` page stability;
5. API key persistence path;
6. successful YouTube Data API statistics retrieval;
7. local first-day history storage.

Still open before App08 can be called complete:

- wire `youtube.*` bindings into `widget_runtime.c`;
- implement/render `chart` objects in the runtime/UI;
- add 7D/30D/90D/ALL controls to the TFT widget;
- physically validate the final on-screen YouTube dashboard and graph;
- only then consider full App08 physical pass/release promotion.
