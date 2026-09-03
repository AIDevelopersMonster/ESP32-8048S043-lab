# Test 33 — RyanEwen / ESPHome + LVGL modular HMI

## Status

**THIRD-PARTY CANDIDATE / BUILD + PHYSICAL VERDICT PENDING**

Upstream repository:

```text
RyanEwen/esphome-lvgl
```

Pinned upstream commit:

```text
4d3ff33b242c6b6ff67dc76f1cfa9b1041473362
2026-01-13
Remove versions that are not needed anymore and caused compile issues
```

The repository does not currently expose a GitHub license declaration. Test 33 therefore does **not** vendor the upstream source into this lab repository. The harness clones the exact public upstream commit into a disposable local work directory.

## Why Test 33

Tests 30-32 validated three conventional firmware/UI approaches:

```text
Test 30  LovyanGFX + LVGL + EEZ
Test 31  native ESP-IDF esp_lcd + LVGL
Test 32  Arduino_GFX + LVGL + EEZ Studio
```

Test 33 is architecturally different. It uses **ESPHome as the application/code-generation layer** and composes the interface from modular YAML packages.

The original project explicitly supports the Sunton `ESP32-8048S043` and provides a ready 4.3-inch target:

```text
sunton-43-example.yaml
```

That target is only a thin composition layer:

```yaml
packages:
  common: !include common.yaml
  device: !include devices/ESP32-8048S043.yaml
  layout: !include layouts/800x480.yaml
```

This is exactly the architectural direction we wanted to inspect after Test 32: UI structure split into reusable device, layout, theme, style and widget modules rather than one monolithic generated C/C++ UI.

## Original project purpose

RyanEwen's project is a Home Assistant / ESPHome touchscreen HMI framework for inexpensive ESP32 display boards.

The 4.3-inch Sunton example combines:

```text
ESPHome
  -> device package for ESP32-8048S043
  -> common Wi-Fi / API / web / OTA services
  -> 800x480 LVGL layout
  -> reusable YAML widgets
  -> styles / themes / fonts
  -> Home Assistant actions and state display
```

The upstream README specifically describes the files under `devices/` as reusable ESPHome packages and the resolution-specific layouts as shared LVGL UI definitions.

## Modular UI structure

The project contains a real compile-time widget hierarchy such as:

```text
layouts/
  800x480.yaml
  fonts/
  styles/
  themes/
  widgets/
    header/
    footer/
    boot_screen/
    buttons/
    climate/
    printers/
    ...
```

Widgets are reused with ESPHome/YAML `!include` and variables.

Example concept:

```text
page
  -> !include widget.yaml
  -> pass uid/entity_id/text/layout variables
  -> related sensor/state definition
```

This is **not runtime widget loading from LittleFS/SD**. The YAML modules are composed at compile time and ESPHome generates the firmware. Nevertheless, it is a much more modular authoring model than a single fixed `lv_demo_widgets()` call or one generated `ui.c` tree.

## Exact Sunton ESP32-8048S043 device path

At the pinned upstream commit the device package uses:

```text
ESP32-S3
16 MB flash
Octal PSRAM 80 MHz
ESP-IDF framework
240 MHz CPU
64 KB data cache
64-byte cache line
SPIRAM instruction/RODATA support
```

Display:

```text
ESPHome display: mipi_rgb
model: RPI
resolution: 800 x 480
rotation: 90
color order: RGB
PCLK: 14 MHz
PCLK inverted: true
DE:    40
HSYNC: 39
VSYNC: 41
PCLK:  42
HSYNC: front 8 / pulse 4 / back 8
VSYNC: front 8 / pulse 4 / back 8
```

RGB pins:

```text
R: 45,48,47,21,14
G: 5,6,7,15,16,4
B: 8,3,46,9,1
```

Touch:

```text
GT911
SDA 19
SCL 20
address 0x5D
update interval 16 ms
swap_xy true
mirror_y true
```

Backlight:

```text
GPIO2 / LEDC PWM / 1220 Hz
```

## Historical ESPHome pin

The upstream commit is dated:

```text
2026-01-13 15:01 UTC
```

ESPHome `2025.12.6` was published later that same day. Therefore the newest stable ESPHome release available at the moment of the upstream commit was:

```text
ESPHome 2025.12.5
```

Test 33 pins that exact ESPHome version for the first reconstruction.

This is intentionally historical rather than using the moving current ESPHome release.

## Build harness policy

The Test 33 harness:

1. clones the exact RyanEwen upstream commit into a short disposable Windows path;
2. verifies the exact `sunton-43-example.yaml` package composition and board configuration;
3. creates an isolated Python virtual environment;
4. installs `esphome==2025.12.5`;
5. creates a temporary local `secrets.yaml` only for compilation;
6. compiles the original `sunton-43-example.yaml`;
7. optionally uploads through a specified serial port;
8. removes build artifacts and the temporary secrets file;
9. verifies that the pinned upstream source tree is unchanged.

No display, touch, layout, widget or application source is patched before the first physical verdict.

## Wi-Fi / Home Assistant note

The original `common.yaml` expects:

```yaml
wifi_ssid
wifi_password
```

The runner can use real test credentials when supplied, but they are never committed.

For a pure display/touch physical test, dummy credentials are acceptable. The firmware may remain disconnected from Wi-Fi/Home Assistant; this does not invalidate display/touch testing.

Some original dashboard buttons and values depend on Home Assistant entities. Without the author's Home Assistant environment, those application actions or states are expected to be unavailable. That is **not** a display/touch failure.

## What to observe physically

Record separately:

```text
Boot
Backlight
UI appears
Display stability at idle
Display stability during touch/redraw
Page navigation
Touch mapping
Button response
Slider/interactive control response where present
Animation smoothness
Horizontal jump
Flicker
Reset/crash
```

Also distinguish:

```text
hardware/display/touch PASS
```

from:

```text
Home Assistant entity unavailable
```

because the latter is expected outside the author's HA installation.

## Why this matters for our future projects

If Test 33 is physically good, it gives us a third production-oriented UI model:

```text
Test 31
native ESP-IDF transport
+ custom runtime/UI layer

Test 32
Arduino_GFX transport
+ EEZ Studio generated UI

Test 33
ESPHome / ESP-IDF transport
+ declarative modular YAML widgets
+ Home Assistant integration
```

Test 33 is particularly relevant to our earlier WidgetLoader idea because it demonstrates a clean separation between:

```text
device definition
layout
styles/theme
widgets
application entities/actions
```

The separation happens at build time rather than runtime, but the package boundaries are useful architectural material for a future runtime loader.

## Next fork variant

After the original RyanEwen baseline, the fork:

```text
xoquox/esphome-lvgl
```

is worth testing separately. It modifies the ESP32-8048S043 profile in 2026, including additional I2C/sensor support and historical comments around ESP-IDF/PSRAM artifacting. It should remain a separate whole-project variant rather than being mixed into Test 33.
