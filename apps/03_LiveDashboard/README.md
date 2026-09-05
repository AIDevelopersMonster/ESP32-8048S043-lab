# App 03 - Live System Dashboard

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Programmer:** Sol  
**Engineer:** Alex Malachevsky

## Purpose

App 03 is the first instrument-style HMI in the project. It keeps the physically validated App 01/App 02 hardware runtime unchanged and replaces the control laboratory with a real live telemetry dashboard.

## Real telemetry in v0.1.0

- ESP32-S3 internal temperature via ESP-IDF temperature sensor driver
- free internal heap
- free PSRAM
- uptime
- 60-sample live temperature chart

No simulated sensor values are used in v0.1.0.

## UI

Main screen:
- large temperature arc and numeric value
- internal heap bar
- PSRAM bar
- uptime
- live temperature history chart
- DETAILS navigation button

Details screen:
- exact current telemetry values
- BACK navigation button

## Touch ownership

The Test 36 rule remains mandatory:

> one semantic control = one intentional touch owner

Telemetry cards, labels, bars, arcs and chart are presentation-only. Only the navigation buttons are clickable.

## Frozen hardware/runtime baseline

Inherited from physically validated App 01/App 02:
- ESP32-S3
- 800x480 RGB565
- PCLK 16 MHz
- one PSRAM RGB framebuffer
- RGB bounce buffer: 10 lines
- LVGL partial draw buffer: 60 lines in internal RAM
- GT911 touch: SDA 19, SCL 20, RST 38
- modern ESP-IDF I2C master API
- dedicated UI task: 16 KB stack, priority 9, core 1
- ESP-IDF 5.5.x
- LVGL 9.3.0

## Physical test boundary

App 03 remains **BUILD / PHYSICAL PENDING** until tested on the real ESP32-8048S043 board.

Physical PASS requires:
1. stable 800x480 display;
2. temperature value changes plausibly over time;
3. heap/PSRAM values are visible and stable;
4. live chart advances once per second;
5. DETAILS and BACK touch navigation works reliably;
6. no flicker, reset, hang, touch loss or stack overflow during observation.

After physical PASS, App 03 can be added to the KONTAKTS Web Flasher catalog.
