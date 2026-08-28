# Third-party audit: clumsyCoder00/Sunton-ESP32-8048S043

Status: `AUDITED FIRST-PASS / EXTERNAL FIRMWARE TEST CANDIDATE / GPL-2.0 REFERENCE ONLY`.

Upstream:

```text
https://github.com/clumsyCoder00/Sunton-ESP32-8048S043
```

Repository metadata observed through GitHub:

```text
Owner          : clumsyCoder00
Repository     : Sunton-ESP32-8048S043
Default branch : main
Visibility     : public
Archived       : false
License        : GPL-2.0
Description    : A minimal configuration for this board.
```

## Scope of this audit

This audit continues the ESP32-8048S043 LVGL third-party reference track after:

```text
rzeldent/platformio-espressif32-sunton
rzeldent/esp32-smartdisplay
limpens/esp32-8048S043
limpens/esp32-8048S043-lvgl9
pixelwave/Sunton-ESP32-8048S043
```

This repository is especially useful because it is a more recent PlatformIO Arduino project for the same board family, built around:

```text
Arduino_GFX
LVGL 9.1.0
EEZ Studio / eez-framework
GT911 capacitive touch
```

It is not a clean BSP. It is an external application firmware candidate to test separately.

## Upstream stated purpose

The upstream README describes the project as a minimal configuration for an ESP32-S3 4.3 inch IPS 800x480 smart display with the IPS capacitive touch option.

The README states that the setup is intended to demonstrate a minimal fully functional configuration using Visual Studio Code, PlatformIO IDE, C++ and Serial Monitor.

## Dependencies declared upstream

The README lists the firmware dependencies as:

```text
Arduino GFX 1.4.7
eez-framework 0.0.1
GT911 1.0.2
LVGL 9.1.0
```

It also states that `lv_conf.h` is included with project-selected options and should live next to the LVGL library directory.

## EEZ Studio / UI generator

The README says the UI is built with EEZ Studio and that LVGL version 9.0 should be selected in EEZ Studio settings when generating compatible code.

The repository includes:

```text
Sunton-ESP32-8048S043/Sunton-ESP32-8048S043.eez-project
```

The EEZ project file records:

```text
projectType   : lvgl
lvglVersion   : 9.0
displayWidth  : 800
displayHeight : 480
colorFormat   : BGR
flowSupport   : true
```

The generated `ui.h` contains local compatibility defines:

```text
EEZ_FOR_LVGL
LV_LVGL_H_INCLUDE_SIMPLE
```

## Project layout

Top-level repository layout:

```text
LICENSE
README.md
Sunton-ESP32-8048S043/
screen 1.JPG
screen 2.JPG
```

Nested PlatformIO project:

```text
Sunton-ESP32-8048S043/README.md
Sunton-ESP32-8048S043/Sunton-ESP32-8048S043.eez-project
Sunton-ESP32-8048S043/include/
Sunton-ESP32-8048S043/lib/
Sunton-ESP32-8048S043/platformio.ini
Sunton-ESP32-8048S043/src/
Sunton-ESP32-8048S043/test/
```

Source layout:

```text
src/main.cpp
src/touch.h
src/ui/actions.h
src/ui/flow_def.c
src/ui/flow_def.h
src/ui/fonts.h
src/ui/images.c
src/ui/images.h
src/ui/screens.c
src/ui/screens.h
src/ui/styles.c
src/ui/styles.h
src/ui/ui.c
src/ui/ui.h
```

Important packaging note:

```text
The nested lib/ folder contains lv_conf.h and a PlatformIO README, but not the full dependency libraries.
External libraries still need to be installed or copied for a local build.
```

## PlatformIO environment

The nested `platformio.ini` defines:

```text
[env:ESP32S3-8048S043]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.arduino.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_opi
build_flags = -DBOARD_HAS_PSRAM
board_upload.flash_size = 16MB
```

This matches our board in the important high-level profile assumptions:

```text
ESP32-S3;
240 MHz CPU;
80 MHz flash;
16 MB flash;
QIO flash + OPI PSRAM profile;
PSRAM enabled.
```

It remains a generic ESP32-S3 DevKitC PlatformIO profile, not a reusable ESP32-8048S043 board definition.

## Display path

The firmware uses Arduino_GFX, not `esp_lcd` directly.

Relevant display architecture:

```text
Arduino_ESP32RGBPanel
Arduino_RGB_Display
800 x 480
rotation 0
auto_flush true
backlight GPIO2
```

RGB pin map:

```text
DE     40
VSYNC  41
HSYNC  39
PCLK   42
R0..R4 45, 48, 47, 21, 14
G0..G5 5, 6, 7, 15, 16, 4
B0..B4 8, 3, 46, 9, 1
```

Timing fields in the `Arduino_ESP32RGBPanel` constructor:

```text
hsync polarity     : 0
hsync front porch  : 8
hsync pulse width  : 4
hsync back porch   : 8
vsync polarity     : 0
vsync front porch  : 8
vsync pulse width  : 4
vsync back porch   : 8
```

Direct lesson:

```text
The pin map and 8/4/8 porch family agree with our validated board knowledge and with other third-party references.
```

Important limitation:

```text
The sketch does not expose a clear explicit pixel clock value like the pixelwave 14 MHz or limpens 18 MHz examples.
```

## LVGL mode and redraw strategy

The firmware is LVGL 9 style:

```text
lv_display_t
lv_display_create()
lv_display_set_flush_cb()
lv_display_set_buffers()
lv_indev_t
lv_indev_create()
lv_indev_set_read_cb()
lv_tick_set_cb()
lv_task_handler()
```

It defines:

```text
#define DIRECT_MODE
// #define RGB_PANEL
```

The code comment says:

```text
RGB_PANEL // doesn't render well with this defined
```

Buffer strategy:

```text
DIRECT_MODE enabled  -> bufSize = screenWidth * screenHeight
RGB_PANEL undefined  -> buffer allocated with heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
                        then fallback to heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT)
LVGL render mode     -> LV_DISPLAY_RENDER_MODE_DIRECT
```

For 800x480 RGB565, this requests approximately:

```text
800 * 480 * 2 = 768000 bytes
```

Loop redraw strategy:

```text
lv_task_handler();
ui_tick();
gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
delay(5);
```

This is the most important technical risk:

```text
The current firmware sends a full 800x480 frame to Arduino_GFX every loop in DIRECT_MODE when RGB_PANEL is not defined.
This may be visually useful for their setup, but it is exactly the kind of high redraw pressure we must evaluate carefully on Sample A.
```

## LVGL configuration

The nested `lib/lv_conf.h` is configured for LVGL 9.1.0 and uses:

```text
LV_COLOR_DEPTH 16
LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
LV_MEM_SIZE 64 KB
LV_DEF_REFR_PERIOD 33 ms
LV_USE_OS LV_OS_NONE
```

This is useful as a reference but should not be mixed into our Arduino library path or committed into our BSP.

## Touch path

The firmware uses the same generic `touch.h` style as other Arduino_GFX Sunton examples, with GT911 enabled.

GT911 settings:

```text
TOUCH_GT911_SCL       20
TOUCH_GT911_SDA       19
TOUCH_GT911_INT       0
TOUCH_GT911_RST       38
TOUCH_GT911_ROTATION  ROTATION_NORMAL
TOUCH_MAP_X1          480
TOUCH_MAP_X2          0
TOUCH_MAP_Y1          272
TOUCH_MAP_Y2          0
```

Confirmed useful facts:

```text
SDA/SCL/RST match our board;
GT911 raw coordinate mapping is again treated as 480 x 272;
X and Y are mapped in reverse direction: 480 -> 0, 272 -> 0;
touch polling is used because touch_has_signal() returns true for GT911.
```

Important caution:

```text
TOUCH_GT911_INT is set to 0, while our tested pin map records GT911 INT as GPIO18 optional.
If touch or boot behavior is abnormal, the first local safety patch should be TOUCH_GT911_INT -1 or 18.
```

The release model is also simple:

```text
touch_released() returns true for GT911.
```

This is acceptable for a quick external firmware test, but it is not a robust BSP input-state model.

## UI structure

The generated UI is minimal:

```text
main screen       : full 800x480 object, label "Hello, world!", button at x=556 y=215 size 100x50
secondary screen  : full 800x480 object, button at x=162 y=215 size 100x50, label "!Main"
```

Button events are attached to `LV_EVENT_ALL`, but screen navigation happens on:

```text
LV_EVENT_RELEASED
```

The EEZ flow runtime is used:

```text
eez_flow_init(...)
eez_flow_tick()
flowPropagateValue(...)
```

This makes the firmware a good candidate to test a generated EEZ/LVGL9 workflow, not a minimal low-level display driver example.

## License boundary

The upstream repository is GPL-2.0.

Local project policy:

```text
Do not copy source files, generated UI files, lv_conf.h, or vendored dependency code into this repository.
Use it as a reference and as an external firmware test only.
If any code is later ported, reimplement independently and record a license decision first.
```

## External firmware test plan

The user's factory firmware is already preserved in this lab project, so it is reasonable to test this external firmware as a controlled experiment. Still, keep it outside this repository.

Recommended local folder:

```text
C:\Users\CHUWI\Documents\GitHub\third-party\clumsyCoder00-Sunton-ESP32-8048S043
```

Recommended commands:

```powershell
mkdir "$HOME\Documents\GitHub\third-party" -Force
cd "$HOME\Documents\GitHub\third-party"

git clone https://github.com/clumsyCoder00/Sunton-ESP32-8048S043.git clumsyCoder00-Sunton-ESP32-8048S043
cd "$HOME\Documents\GitHub\third-party\clumsyCoder00-Sunton-ESP32-8048S043\Sunton-ESP32-8048S043"

pio run -e ESP32S3-8048S043
```

If dependencies are missing, install or copy the exact upstream-declared libraries into the PlatformIO project environment:

```text
Arduino GFX 1.4.7
eez-framework 0.0.1
GT911 1.0.2
LVGL 9.1.0
```

Then build again:

```powershell
pio run -e ESP32S3-8048S043
```

Upload only after a successful build:

```powershell
pio run -e ESP32S3-8048S043 -t upload
pio device monitor -b 115200
```

Expected serial identity from upstream code:

```text
Arduino_GFX LVGL_Arduino example v9
Hello Arduino! V<major>.<minor>.<patch>
Init Display
TFT_BL
Setup done
```

What to observe physically:

```text
Does the screen light without boot loop?
Does the UI appear?
Does touch move between the EEZ-generated screens?
Is the screen stable while idle?
Does the full-frame redraw every loop cause visible flicker/tearing?
Is touch coordinate direction correct?
Does hard tapping reproduce the same jitter boundary seen in our tests?
```

If the sketch builds but the screen is blank or unstable:

```text
record Serial output;
record whether backlight is on;
record whether touch reacts;
do not erase all flash repeatedly;
restore by uploading our known-good Arduino test if needed.
```

If touch does not work:

```text
first local patch candidate: in src/touch.h change TOUCH_GT911_INT from 0 to -1 or 18;
do not commit this patch into our repository;
record the patch as a local experiment.
```

## Comparison with our current local evidence

Our current local path after tests 13-16:

```text
13_LVGL_EspLcdStatic                 : static LVGL over native esp_lcd works;
14_GT911_NormalizedTouch             : GT911 BSP normalized 9 zones;
15_LVGL_EspLcdBasicUI                : functional but dynamic redraw not acceptable;
16_LVGL_EspLcdMinimalInvalidation    : idle stable, click-only path better, hard-tap jitter open.
```

clumsyCoder00 differs sharply:

```text
LVGL 9 instead of LVGL 8;
Arduino_GFX instead of native esp_lcd;
EEZ flow runtime instead of hand-written minimal UI;
DIRECT_MODE full-frame buffer;
full-screen draw16bitRGBBitmap() every loop;
GT911 INT configured as GPIO0;
external dependencies not fully vendored.
```

Therefore this firmware is valuable as a black-box external comparison, but it is not yet a safe template for our BSP.

## Direct decision for this project

Use clumsyCoder00 as:

```text
external firmware comparison candidate;
LVGL9 + EEZ Studio workflow reference;
pin-map and GT911 raw-range confirmation;
full-frame Arduino_GFX DIRECT_MODE stress comparison;
PlatformIO profile comparison for 16 MB / qio_opi / PSRAM.
```

Do not use it as:

```text
final display driver architecture;
final touch driver model;
license-compatible source for copying;
proof that dynamic UI quality is solved;
replacement for our BSP-centered path.
```

## Recommended next third-party target

After this audit and optional firmware test, continue with:

```text
wegi1/ESP32-8048S043-4INCH-LCD
```
