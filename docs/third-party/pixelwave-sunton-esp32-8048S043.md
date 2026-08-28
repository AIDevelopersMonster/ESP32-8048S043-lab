# Third-party audit: pixelwave/Sunton-ESP32-8048S043

Status: `AUDITED FIRST-PASS / REFERENCE ONLY`.

Upstream:

```text
https://github.com/pixelwave/Sunton-ESP32-8048S043
```

Repository metadata observed through GitHub:

```text
Owner          : pixelwave
Repository     : Sunton-ESP32-8048S043
Default branch : main
Visibility     : public
Archived       : false
Root files     : README.md, include, lib, platformio.ini, src, test
```

## Scope of this audit

This audit continues the ESP32-8048S043 LVGL third-party reference track.

Previous first-pass audits in this track:

```text
rzeldent/platformio-espressif32-sunton
rzeldent/esp32-smartdisplay
limpens/esp32-8048S043
limpens/esp32-8048S043-lvgl9
```

This repository is different from the strongest previous references. It is not an `esp_lcd` / ESP-IDF style board BSP. It is an Arduino_GFX + LVGL + SquareLine UI PlatformIO project.

## Upstream stated purpose

The upstream README describes the project as a first test with:

```text
Sunton ESP32-8048S043
SquareLine UI
LVGL
Arduino GFX
```

It says the project should display a simple UI with two screens that can navigate forward and back.

## Project layout

Top-level layout:

```text
README.md
include/
lib/
platformio.ini
src/
test/
```

Relevant source layout:

```text
src/main.cpp
src/touch.h
src/ui.c
src/ui.h
src/ui_events.h
src/ui_helpers.c
src/ui_helpers.h
src/screens/ui_Screen1.c
src/screens/ui_Screen2.c
```

The `lib/` folder vendors third-party libraries inside the repository:

```text
lib/GFX_Library_for_Arduino
lib/Gt911-arduino-main
lib/Lvgl
```

This is convenient for a one-off PlatformIO demo, but it is not a packaging pattern to copy into this lab repository.

## PlatformIO environment

The upstream PlatformIO configuration uses:

```text
[env:esp32s3box]
platform = espressif32
board = esp32s3box
framework = arduino
monitor_speed = 115200
```

Important observation:

```text
The project uses generic esp32s3box as the PlatformIO board, not a dedicated ESP32-8048S043 board definition.
```

For our lab this means the repository is useful as an application sketch reference, not as a board-profile reference.

## Display path

The upstream application uses Arduino_GFX:

```text
#include <Arduino_GFX_Library.h>
```

Display bus:

```text
Arduino_ESP32RGBPanel
```

Panel wrapper:

```text
Arduino_RPi_DPI_RGBPanel
```

Observed RGB pin map:

```text
DE     40
VSYNC  41
HSYNC  39
PCLK   42
R0..R4 45, 48, 47, 21, 14
G0..G5 5, 6, 7, 15, 16, 4
B0..B4 8, 3, 46, 9, 1
```

This matches the pin map already validated in this lab.

Observed panel timing:

```text
width              : 800
height             : 480
hsync polarity     : 0
hsync front porch  : 8
hsync pulse width  : 4
hsync back porch   : 8
vsync polarity     : 0
vsync front porch  : 8
vsync pulse width  : 4
vsync back porch   : 8
pclk active neg    : 1
prefer speed       : 14 MHz
auto_flush         : true
```

Timing lesson:

```text
This repository adds a third observed working-ish timing candidate: 14 MHz with 8/4/8 porches.
Earlier references gave 12.5 MHz and 18 MHz.
```

Do not change our tested timing solely from this reference. Treat 14 MHz as a future experiment point only.

## LVGL path

The upstream code uses LVGL 8 style APIs:

```text
lv_disp_draw_buf_t
lv_disp_drv_t
lv_indev_drv_t
lv_timer_handler()
```

The SquareLine-generated UI declares:

```text
SquareLine Studio version : 1.3.3
LVGL version              : 8.3.6
```

The generated `ui.c` enforces:

```text
LV_COLOR_DEPTH == 16
LV_COLOR_16_SWAP == 0
```

Display flush path:

```text
gfx->draw16bitRGBBitmap(...)
lv_disp_flush_ready(disp)
```

or, if LV_COLOR_16_SWAP is enabled:

```text
gfx->draw16bitBeRGBBitmap(...)
```

The upstream comment explicitly says that for parallel screens the color swap setting should not be enabled.

## LVGL draw buffer

The upstream LVGL draw buffer is allocated as:

```text
screenWidth * screenHeight / 4
```

On an 800x480 display this is:

```text
800 * 480 / 4 = 96000 pixels
```

At 16-bit color this is about:

```text
192000 bytes
```

The allocation flags are:

```text
MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
```

Important caution for our board:

```text
This is internal-RAM-oriented, not PSRAM-oriented.
Our local working tests allocate LVGL draw buffers in PSRAM.
Do not copy this buffer allocation policy blindly.
```

## Touch path

The upstream touch layer is a generic selector for FT6X36, GT911 and XPT2046, with GT911 enabled.

Observed GT911 settings:

```text
TOUCH_GT911_SCL       20
TOUCH_GT911_SDA       19
TOUCH_GT911_INT       -1
TOUCH_GT911_RST       38
TOUCH_GT911_ROTATION  ROTATION_NORMAL
TOUCH_MAP_X1          480
TOUCH_MAP_X2          0
TOUCH_MAP_Y1          272
TOUCH_MAP_Y2          0
```

This confirms again:

```text
GT911 raw coordinate space is treated as 480 x 272.
The X axis is reversed by mapping 480 -> 0.
The Y axis is reversed by mapping 272 -> 0.
The INT pin is not used for GT911 in this project.
Polling is used.
```

The LVGL input callback calls `touch_has_signal()`, then `touch_touched()` or `touch_released()`. For the GT911 branch, `touch_has_signal()` always returns true, and `touch_touched()` performs `ts.read()` and maps the first point into display coordinates.

Important direct lesson:

```text
Polling GT911 without depending on INT is consistent with our working local direction.
```

Important caution:

```text
The upstream release logic for the GT911 branch returns true in touch_released().
This is a simple demo pattern, not a robust input-state model for our BSP.
```

## SquareLine UI structure

The UI is generated by SquareLine Studio and consists of two screens.

Screen 1:

```text
black background;
central 100x50 button;
label "Go";
label "pixelWAVE STUDIOS";
button event changes to Screen 2.
```

Screen 2:

```text
central 100x50 button;
label "Back";
button event changes back to Screen 1.
```

Screen transitions use:

```text
LV_SCR_LOAD_ANIM_FADE_ON
500 ms duration
```

Important caution for our current stage:

```text
The project uses screen fade animation and full screen switching.
Our current local evidence shows that unnecessary invalidation and dynamic redraw are exactly where visual defects appear.
Do not port this animation pattern yet.
```

## Good ideas to keep

Useful architectural ideas:

```text
A generated UI can be kept separate from board bring-up code.
The application demonstrates a two-screen SquareLine-style workflow.
The UI files are separated into ui.c/ui.h/helpers/screens.
Touch mapping is isolated in touch.h rather than scattered through generated UI files.
The first screen is simple and can be a future test shape once redraw stability is solved.
```

Useful hardware confirmations:

```text
RGB pin map agrees with our validated map.
GT911 pins agree with our validated map.
GT911 raw mapping around 480x272 appears again.
14 MHz / 8-4-8 is another timing candidate.
```

## Things not to copy

Do not copy these parts into this lab repository:

```text
vendored GFX/LVGL/GT911 libraries under lib/;
generic esp32s3box board selection as a board-profile solution;
internal-RAM-only LVGL buffer allocation for this 800x480 board;
full-screen fade transition as an early UI test;
GT911 release model as-is;
application-level direct GPIO table as the final architecture;
SquareLine generated files verbatim.
```

## Comparison with our current local evidence

Our current local evidence after tests 13-16 shows:

```text
13_LVGL_EspLcdStatic       : static LVGL over esp_lcd works;
14_GT911_NormalizedTouch   : GT911 BSP normalized 9 zones;
15_LVGL_EspLcdBasicUI      : functional but dynamic redraw not acceptable;
16_LVGL_EspLcdMinimalInvalidation : idle stable, click-only path better, hard-tap jitter open.
```

Pixelwave is useful because it demonstrates that a two-screen SquareLine-generated LVGL interface is plausible on this board family, but it also uses the exact kind of screen animation and Arduino_GFX transport that we are currently trying not to depend on.

## Direct decision for this project

Use pixelwave as:

```text
SquareLine workflow reference;
UI folder organization reference;
pin-map cross-check;
GT911 raw-range confirmation;
14 MHz timing candidate.
```

Do not use it as:

```text
final display-driver reference;
final touch-driver reference;
final PlatformIO board profile;
final buffering strategy;
proof that dynamic UI quality is solved.
```

## Recommended next local experiment

After test 16, the next local test should continue isolating redraw cause:

```text
17_LVGL_EspLcdManualHitbox
```

Purpose:

```text
Do not register GT911 as LVGL pointer driver.
Read GT911 manually in the sketch.
Do manual hit testing against a central target.
Do not invalidate anything on press.
Update only a small counter label after release.
```

This will separate LVGL pointer-state invalidation from manual touch handling.

## Next third-party audit target

Continue in order with:

```text
clumsyCoder00/Sunton-ESP32-8048S043
```

It was partially inspected before and should now be turned into a commit-safe audit note.
