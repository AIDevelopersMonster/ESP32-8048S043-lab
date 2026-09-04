# Test 34 — xoquox / ESPHome + LVGL fork variant

## Status

**BUILD PASS / PHYSICAL VERDICT PENDING**

Historical build verified on 2026-09-04 against the exact pinned xoquox fork commit. Physical-board verification is the next step.

Upstream repository:

```text
xoquox/esphome-lvgl
```

Upstream URL:

https://github.com/xoquox/esphome-lvgl

This repository is a fork of:

```text
RyanEwen/esphome-lvgl
```

Test 33 already established the RyanEwen baseline as BUILD PASS + PHYSICAL PASS. Test 34 intentionally keeps the xoquox fork separate so fork-specific differences are not mixed into that known-good reference.

## Pinned upstream commit

```text
33a2a35c0c09c9b3c825e98a0a4abe41931b5708
2026-01-18 15:25:50 UTC
added bme680 support
```

At the time of this test GitHub reports no license declaration for the fork (`license: null`). Upstream source is therefore not vendored into this lab repository. The harness clones the exact public commit into a disposable work directory.

## Why Test 34

The fork is close enough to Test 33 to give us a controlled architectural comparison, but different enough to be worth testing as a whole-project variant.

Key differences visible at the pinned commit include:

```text
RyanEwen Test 33                 xoquox Test 34
-----------------------------   --------------------------------
mipi_rgb / model RPI            rpi_dpi_rgb
one I2C bus 19/20                touch bus_a 19/20
                                 sensor bus_b 17/18
GT911 on default I2C             GT911 explicitly on bus_a
no BME680 package                sensors/bme680.yaml added
current framework selection      historical anti-artifact pins remain
                                 in comments only
```

The xoquox device profile contains the historical comment:

```yaml
# these versions prevent artifacting
# version: 5.3.0
# platform_version: 6.8.1
```

Those pins are deliberately **not re-enabled** in the first Test 34 build because the pinned upstream commit itself commented them out. The purpose is to test the exact current fork state first.

## ESP32-8048S043 device profile at the pin

Target:

```text
ESP32-S3
16 MB flash
Octal PSRAM 80 MHz
ESP-IDF framework
CPU 240 MHz
64 KB data cache
64-byte cache line
```

Display:

```text
ESPHome display: rpi_dpi_rgb
resolution: 800 x 480
rotation: 90
color order: RGB
PCLK: 14 MHz
PCLK inverted: true
DE:    GPIO40
HSYNC: GPIO39
VSYNC: GPIO41
PCLK:  GPIO42
HSYNC: front 8 / pulse 4 / back 8
VSYNC: front 8 / pulse 4 / back 8
```

RGB pins:

```text
R: 45,48,47,21,14
G: 5,6,7,15,16,4
B: 8,3,46,9,1
```

Touch bus:

```text
I2C bus_a
SDA 19
SCL 20
GT911 address 0x5D
update interval 16 ms
swap_xy true
mirror_y true
```

Additional sensor bus introduced by the fork:

```text
I2C bus_b
SDA 17
SCL 18
```

The latest commit also adds `sensors/bme680.yaml` for a BME680 at address `0x77` on `bus_b`. That optional sensor package is **not included in the default physical HMI baseline**, because the laboratory board is being tested for display/touch behavior and a BME680 is not required hardware for that purpose.

## No ready 4.3-inch root example in the fork

Unlike the RyanEwen parent used in Test 33, the xoquox fork does not provide a root-level `sunton-43-example.yaml` at the pinned commit.

Test 34 therefore creates a disposable wrapper **outside the upstream git tree**:

```yaml
esphome:
  name: xoquox-43-test
  friendly_name: xoquox 4.3 Test

substitutions:
  home_page: lighting_1

packages:
  common: !include upstream/common.yaml
  device: !include upstream/devices/ESP32-8048S043.yaml
  layout: !include upstream/layouts/800x480.yaml
```

This wrapper is laboratory glue only. It does not modify or vendor the fork source.

## Historical ESPHome pin

Pinned fork commit:

```text
2026-01-18 15:25:50 UTC
```

ESPHome `2025.12.7` was published on:

```text
2026-01-17 03:49:29 UTC
```

Therefore Test 34 uses:

```text
ESPHome 2025.12.7
```

as the latest stable ESPHome release available before the fork commit.

## Build result

The exact fork state compiled successfully on 2026-09-04:

```text
============================================ [SUCCESS] Took 1264.69 seconds ============================================
INFO Successfully compiled program.

[PASS] Exact upstream source restored; generated build artifacts removed
[PASS] Isolated PlatformIO cache retained at: C:\Users\CHUWI\p34-pio-py313
```

Build verdict:

```text
ESPHome validation          PASS
Historical reconstruction  PASS
Firmware compilation       PASS
Upstream source restored   PASS
Global ~/.platformio       NOT USED
Physical-board verdict     PENDING
```

No upstream source changes were required to obtain the successful build. The commented ESP-IDF 5.3.0 / platform 6.8.1 anti-artifact pins remained commented, matching the pinned xoquox commit.

## Reused Windows reproducibility lessons from Test 33

Test 34 starts with the Test 33 build-environment fixes already applied:

```text
Python 3.11-3.13 only for this historical ESPHome line
isolated Test 34 Python venv
isolated PLATFORMIO_CORE_DIR
UV_PYTHON forced to the selected compatible Python
short PlatformIO core path to avoid Windows ESP-IDF MAX_PATH extraction failures
global ~/.platformio left untouched
```

Short PlatformIO path:

```text
C:\Users\<USER>\p34-pio-py313
```

## Test policy

This is a **whole-project third-party architecture test**, not a one-variable causal experiment.

Baseline rules:

1. exact upstream commit `33a2a35...`;
2. no edits to upstream source;
3. no re-enabling the commented ESP-IDF/platform pins;
4. no BME680 package in the default HMI test;
5. external wrapper only to compose the fork packages because the fork has no ready root example;
6. build first, physical verdict second.

The build phase is now complete. Any derived compatibility experiment, if needed after the physical verdict, must remain separate from this baseline.

## What to observe physically

After flashing:

```text
Boot
Backlight
UI appears
Display stability at idle
Display stability during touch/redraw
GT911 mapping
Page navigation
Button response
Interactive control response
Animation smoothness
Horizontal jump
Flicker/artifacting
Reset/crash
```

Home Assistant entity availability remains separate from the display/touch verdict.

## Expected architectural value

Test 34 can answer a useful practical question:

```text
Does the xoquox fork's rpi_dpi_rgb + dual-I2C ESPHome profile
remain stable on the same real ESP32-8048S043 board where the
RyanEwen mipi_rgb baseline passed?
```

Because Test 33 is already frozen as known-good, the Test 34 physical result will provide a useful whole-architecture comparison without contaminating the proven parent baseline.
