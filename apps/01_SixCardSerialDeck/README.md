# App 01 — Six-Card Serial Deck

## Status

**ORIGINAL APPLICATION / PHYSICAL PASS / PROFILE SWITCHING PASS / STACK STABLE**

This is the first own application in the ESP32-8048S043 lab. It is not a fork of the `halyssonJr/lvgl-demo-esp32s3` Stream Deck UI and does not reuse that project's UI source or image assets.

The application intentionally keeps only the useful concept discovered during external-project study: a six-card command panel.

## Repository and physical demonstration

Repository:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

App path:

```text
apps/01_SixCardSerialDeck/
```

Full physical demonstration of the current corrected firmware:

https://youtube.com/shorts/0I5JL6jt8e0

The video demonstrates the complete App 01 idea on the physical ESP32-8048S043 board:

- corrected top-right PROFILE control;
- HOME / MEDIA / SYSTEM profile switching;
- visible reassignment of all six cards on the 800x480 display;
- GT911 touch operation;
- matching `PROFILE:` and `CARD:` output in Serial/COM;
- the profile-driven design where visible label/icon and stable command are separate.

The video is the physical demonstration of the current normal App 01 firmware after the PROFILE presentation fix. The earlier version with the small PROFILE-label clipping issue was shown separately on YouTube and is not the video registered here as the current App 01 evidence.

## Physical result

The application now passes on the physical ESP32-8048S043 board.

Confirmed:

```text
ESP32-S3 boot                  PASS
16 MB flash                    PASS
8 MB Octal PSRAM               PASS
native RGB display             PASS
GT911 identification           PASS
GT911 Config Version 65        PASS
HOME profile                   PASS
MEDIA profile                  PASS
SYSTEM profile                 PASS
profile switching              PASS
card touch                     PASS
serial CARD commands           PASS
visible UI                     PASS
reboot/stack overflow          FIXED
long-running stack stability   PASS
```

Observed command examples:

```text
CARD:POWER
CARD:MEDIA
CARD:GAME
CARD:SETTINGS
CARD:WORK
CARD:SOCIAL
```

The dedicated UI task remained stable during the physical run. Reported stack high-water values settled at approximately:

```text
11956 bytes
11924 bytes
```

with a configured task stack of 16384 bytes. This leaves a large operating margin for the current application.

## First physical boot issue and fix

The first flashed build reached all initialization stages successfully but repeatedly rebooted with:

```text
***ERROR*** A stack overflow in task main has been detected.
```

The display, PSRAM, GT911 and UI had already initialized correctly. The fault was architectural: the infinite `lv_timer_handler()` loop had been left inside ESP-IDF's system `main` task.

The LVGL runtime was moved into a dedicated pinned FreeRTOS task:

```text
task name    app01_ui
stack        16384 bytes
priority     9
core         1
```

`app_main()` now only creates the UI task and returns.

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

Display baseline:

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

Press the profile selector in the top-right corner to cycle through the profiles.

Profile changes are reported as:

```text
PROFILE:HOME
PROFILE:MEDIA
PROFILE:SYSTEM
```

## Profile selector UX history

An earlier physically successful build used a smaller 150 x 40 top-right profile button. Functionally it worked correctly, but the control was not visually obvious enough and part of the PROFILE label could be clipped.

The current firmware uses the corrected presentation:

```text
old size     150 x 40
new size     200 x 48
position     moved left
border       added
shadow       added
background   made more distinct
label        PROFILE: <NAME> >
```

This was a presentation-only refinement; profile logic and touch behavior were not changed. The registered current demonstration video shows the corrected version.

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

## Independence

The application is a clean lab implementation built from board knowledge and generic LVGL/ESP-IDF APIs. It does not include the third-party Stream Deck project's images or generated UI source.

The original external project that inspired the six-card study is:

https://github.com/halyssonJr/lvgl-demo-esp32s3

Our lab repository:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
