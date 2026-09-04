# Test 33 — RyanEwen / ESPHome + LVGL modular HMI

## Status

**BUILD PASS / PHYSICAL PASS / CLOSED / KNOWN-GOOD THIRD-PARTY REFERENCE**

Test 33 was completed on 2026-09-04 on the real ESP32-8048S043 4.3-inch board.

Final verdict:

```text
Build                    PASS
Boot                     PASS
Backlight                PASS
Display/UI               PASS
GT911 touch              PASS
Navigation/controls      PASS
Visible redraw flicker   NOT OBSERVED
Horizontal jump          NOT OBSERVED
Reset/crash              NOT OBSERVED
Overall physical result  PASS
State                     CLOSED / KNOWN-GOOD REFERENCE
```

User physical verdict: **everything works excellently**.

## Links

Original third-party source:

https://github.com/RyanEwen/esphome-lvgl

Our ESP32-8048S043 laboratory repository:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

Test 33 documentation in our repository:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/tree/agent/test33-thirdparty-ryanewen-esphome-lvgl/third_party_tests/33_ESPHome_LVGL_RyanEwen

Physical-board video evidence:

https://youtube.com/shorts/PH_WeBeZZqg

## Upstream baseline

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

The repository did not expose a GitHub license declaration at the time of this test. Test 33 therefore does **not** vendor the upstream source into this lab repository. The harness clones the exact public upstream commit into a disposable local work directory.

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

That target is a thin composition layer:

```yaml
packages:
  common: !include common.yaml
  device: !include devices/ESP32-8048S043.yaml
  layout: !include layouts/800x480.yaml
```

This architecture is relevant to the lab because the UI is split into reusable device, layout, theme, style and widget modules rather than one monolithic generated C/C++ UI.

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

The upstream README describes the files under `devices/` as reusable ESPHome packages and the resolution-specific layouts as shared LVGL UI definitions.

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

Conceptually:

```text
page
  -> !include widget.yaml
  -> pass uid/entity_id/text/layout variables
  -> related sensor/state definition
```

This is **not runtime widget loading from LittleFS/SD**. The YAML modules are composed at compile time and ESPHome generates the firmware. Nevertheless, it is substantially more modular than a fixed `lv_demo_widgets()` call or a single generated `ui.c` tree.

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

## Historical ESPHome reconstruction

The upstream commit is dated:

```text
2026-01-13 15:01 UTC
```

ESPHome `2025.12.6` was published later that same day. Therefore the newest stable ESPHome release available at the time of the upstream commit was:

```text
ESPHome 2025.12.5
```

Test 33 pins that exact ESPHome version for the reconstruction rather than using a moving current release.

## What we had to solve to reproduce the build

The upstream application source itself was left unchanged. The work was entirely in the Windows build harness and reproducibility environment.

### 1. Python compatibility

The test machine initially had only:

```text
Python 3.14.6
```

ESPHome 2025.12.5 requires:

```text
Python >= 3.11 and < 3.14
```

We installed/selected Python 3.13 side-by-side and modified the Test 33 runner so it automatically selects only a compatible interpreter.

### 2. PlatformIO ESP-IDF child environment

Even after ESPHome itself ran under Python 3.13, PlatformIO initially created its ESP-IDF environment under the user's global PlatformIO installation and picked Python 3.14 again. That caused native Python dependency failures involving `pydantic-core`, Rust/MSVC and a missing `idf_component_manager`.

The runner was changed to:

```text
use a Test-33-only PLATFORMIO_CORE_DIR
force child processes to the selected Python 3.13
set UV_PYTHON to the Test 33 venv Python
leave the user's global ~/.platformio untouched
```

### 3. Windows path-length failure

The isolated ESP-IDF package then failed while unpacking a deeply nested OpenThread/mbedTLS test file because the first isolated PlatformIO path was too long for the Windows extraction path.

The PlatformIO core path was shortened from a long Test 33 work path to:

```text
C:\Users\CHUWI\p33-pio-py313
```

That removed the extraction failure without modifying the upstream RyanEwen source.

## Build result

The historical reconstruction reached a clean successful build on 2026-09-04:

```text
============================================ [SUCCESS] Took 839.35 seconds ============================================
INFO Successfully compiled program.

[PASS] Exact upstream source restored; ESPHome build artifacts removed
[PASS] Isolated PlatformIO cache retained at: C:\Users\CHUWI\p33-pio-py313
```

This means the exact pinned RyanEwen application/package source was successfully built in our controlled historical environment.

## Physical-board verification

The compiled firmware was uploaded to the real ESP32-8048S043 board and tested interactively.

Observed result:

```text
Boot                  PASS
Backlight             PASS
LVGL interface        PASS
Display stability     PASS
GT911 touch           PASS
Navigation            PASS
Interactive controls  PASS
Responsiveness        PASS
```

No visible redraw flicker, horizontal jump, reset or crash was reported during the physical test.

The Home Assistant-specific entity state is intentionally not part of the low-level display/touch verdict because the original project expects the author's external HA environment.

Video evidence from the physical test:

https://youtube.com/shorts/PH_WeBeZZqg

## Build harness policy

The Test 33 harness:

1. clones the exact RyanEwen upstream commit into a disposable Windows work directory;
2. verifies the exact `sunton-43-example.yaml` package composition and board configuration;
3. selects Python 3.11-3.13 and rejects unsupported Python 3.14 for this historical ESPHome version;
4. creates an isolated Python virtual environment;
5. installs `esphome==2025.12.5`;
6. creates a temporary local `secrets.yaml` only for compilation;
7. uses an isolated, short-path PlatformIO core for the ESP-IDF dependency layer;
8. compiles the original `sunton-43-example.yaml`;
9. optionally uploads through a specified serial port;
10. removes generated build artifacts and the temporary secrets file;
11. verifies that the pinned upstream source tree is unchanged.

No display, touch, layout, widget or application source was patched to obtain the PASS result.

## Wi-Fi / Home Assistant note

The original `common.yaml` expects:

```yaml
wifi_ssid
wifi_password
```

The runner can use real test credentials when supplied, but they are never committed.

For a pure display/touch physical test, dummy credentials are acceptable. Some dashboard buttons and values depend on Home Assistant entities; unavailable external entities are therefore not classified as display/touch failures.

## Architectural result

Test 33 adds another proven UI architecture to the laboratory matrix:

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

Test 33 is especially relevant to the earlier WidgetLoader direction because it demonstrates a clean separation between:

```text
device definition
layout
styles/theme
widgets
application entities/actions
```

The separation happens at build time rather than runtime, but the package boundaries are useful architectural material for a future runtime loader.

## Final Test 33 conclusion

```text
RyanEwen/esphome-lvgl
ESP32-8048S043
ESPHome 2025.12.5 historical reconstruction
ESP-IDF / MIPI RGB / LVGL / GT911

BUILD PASS
PHYSICAL PASS
DISPLAY STABLE
TOUCH PASS
NO VISIBLE FLICKER REPORTED
NO HORIZONTAL JUMP REPORTED
NO RESET/CRASH REPORTED
CLOSED / KNOWN-GOOD THIRD-PARTY REFERENCE
```

## Next fork variant

The fork:

```text
xoquox/esphome-lvgl
```

remains worth testing separately. It modifies the ESP32-8048S043 profile in 2026, including additional I2C/sensor support and historical comments around ESP-IDF/PSRAM artifacting. It should remain a separate whole-project variant rather than being mixed into Test 33.
