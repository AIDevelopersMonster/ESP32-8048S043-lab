# App 01 — Six-Card Serial Deck

## Status

**ORIGINAL APPLICATION / PHYSICAL BOOT CONFIRMED / UI-TASK STACK FIX APPLIED / FULL PHYSICAL VALIDATION PENDING**

This is the first own application in the ESP32-8048S043 lab. It is not a fork of the `halyssonJr/lvgl-demo-esp32s3` Stream Deck UI and does not reuse that project's UI source or image assets.

The application intentionally keeps only the useful concept discovered during external-project study: a six-card command panel.

## First physical boot result

The first flashed build reached all of the following successfully on the physical board:

```text
ESP32-S3 boot                  PASS
16 MB flash                    PASS
8 MB Octal PSRAM               PASS
native RGB display init        PASS
GT911 identification           PASS
GT911 Config Version 65        PASS
HOME profile construction      PASS
six-card screen visible        PASS
```

Observed serial sequence:

```text
APP01: App 01 - Six-Card Serial Deck
APP01: Free INTERNAL heap before display: 312987
APP01: Free PSRAM before display: 7840024
GT911: TouchPad_ID:0x39,0x31,0x31
GT911: TouchPad_Config_Version:65
PROFILE:HOME
APP01: Ready. Tap cards; press PROFILE to reassign all six slots.
```

Immediately after this, the first build repeatedly rebooted with:

```text
***ERROR*** A stack overflow in task main has been detected.
```

This was not a display, PSRAM or GT911 failure. The UI was already visible and the touch controller had initialized correctly. The fault was architectural: the infinite `lv_timer_handler()` loop had been left inside ESP-IDF's system `main` task, whose stack was insufficient for the LVGL runtime path.

### Fix

The LVGL runtime now executes in a dedicated pinned FreeRTOS task:

```text
task name    app01_ui
stack        16384 bytes
priority     9
core         1
```

`app_main()` now only creates that task and returns. The UI task reports its stack high-water mark after initialization and every 10 seconds so stack usage remains observable during development.

## Architecture

```text
ESP32-S3
  -> native ESP-IDF RGB esp_lcd
  -> RGB565 / 800x480
  -> one PSRAM RGB framebuffer
  -> 10-line RGB bounce buffer
  -> LVGL 9.3 partial rendering
  -> 60-line INTERNAL LVGL draw buffer
  -> GT911 over modern i2c_master
  -> dedicated app01_ui FreeRTOS task
  -> six universal card slots
```

Display baseline uses the physically proven board-family configuration:

```text
PCLK       16 MHz
HSYNC      pulse 4 / back 8 / front 8
VSYNC      pulse 4 / back 8 / front 8
pclk neg   true
BL         GPIO2
HSYNC      GPIO39
VSYNC      GPIO41
DE         GPIO40
PCLK       GPIO42
```

Touch:

```text
GT911
SDA GPIO19
SCL GPIO20
RST GPIO38
INT disabled / polling
modern ESP-IDF i2c_master
400 kHz
```

## Key UI rule learned from Test 36

Each semantic control has exactly one intentional touch target.

The card itself is an `lv_button`. All decorative children are explicitly non-clickable:

```c
lv_obj_remove_flag(child, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
```

This avoids the failure mode found in the third-party Stream Deck demo where a decorative child object intercepted touches intended for the parent button.

## Universal slots

A card does not derive its behavior from its visible icon or label.

Each slot has a data model:

```text
visual
label
command
color
```

The event callback receives the slot model and emits a stable command:

```text
CARD:<COMMAND>
```

The visible UI can therefore change independently from the serial protocol.

## Profiles

App 01 v1 contains three profiles.

### HOME

```text
Power      -> CARD:POWER
Media      -> CARD:MEDIA
Game       -> CARD:GAME
Social     -> CARD:SOCIAL
Work       -> CARD:WORK
Settings   -> CARD:SETTINGS
```

### MEDIA

```text
Play       -> CARD:PLAY
Pause      -> CARD:PAUSE
Previous   -> CARD:PREVIOUS
Next       -> CARD:NEXT
Vol -      -> CARD:VOL_DOWN
Vol +      -> CARD:VOL_UP
```

### SYSTEM

```text
Home       -> CARD:HOME
Wi-Fi      -> CARD:WIFI
Bluetooth  -> CARD:BLUETOOTH
USB        -> CARD:USB
SD Card    -> CARD:SD_CARD
Settings   -> CARD:SETTINGS
```

Press the `PROFILE` button in the top-right corner to cycle through the profiles.

Profile changes are also reported:

```text
PROFILE:HOME
PROFILE:MEDIA
PROFILE:SYSTEM
```

## Visual-slot design

Version 1 uses LVGL built-in symbols. The visual slot is intentionally separated from card semantics so later versions can replace symbols with:

- original bitmap/SVG-derived assets converted for LVGL;
- animated images;
- status LEDs;
- arcs/bars/gauges;
- small read-only status widgets.

For ordinary command cards, those children must remain non-clickable. A truly interactive slider/switch card should be implemented as a separate card type with explicit touch ownership.

## Build

Requirements:

- ESP-IDF 5.5.x
- ESP32-S3 target
- 16 MB flash
- 8 MB Octal PSRAM

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\apps\01_SixCardSerialDeck\run-app01.ps1
```

Flash, for example COM7:

```powershell
powershell -ExecutionPolicy Bypass -File .\apps\01_SixCardSerialDeck\run-app01.ps1 -Flash -Port COM7
```

## Physical acceptance

Minimum acceptance run after the UI-task fix:

```text
1. leave screen running for at least 20 seconds
   -> no reboot
   -> observe UI task stack high-water logs

2. HOME profile:
   each card x10
   then one circular pass across all six

3. switch to MEDIA:
   verify all six labels/symbols change
   press each once

4. switch to SYSTEM:
   verify all six labels/symbols change
   press each once
```

Expected:

- no reboot or stack-overflow message;
- no visible flicker;
- no missed ordinary taps;
- pressed feedback visible immediately;
- one serial `CARD:` line per completed click;
- profile change does not alter touch reliability.

## Independence

The application is a clean lab implementation built from board knowledge and generic LVGL/ESP-IDF APIs. It does not include the third-party Stream Deck project's images or generated UI source.

The original external project that inspired the six-card study is:

https://github.com/halyssonJr/lvgl-demo-esp32s3

Our lab repository:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
