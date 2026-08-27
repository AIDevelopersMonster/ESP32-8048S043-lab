# LVGL third-party reference notes

Status: `OPEN / THIRD-PARTY AUDIT IN PROGRESS`.

This note starts the next LVGL track after the local ESP32-8048S043 LVGL examples reached a clear evidence boundary.

## Local result on ESP32-8048S043

The local examples prove that the board can run LVGL with the validated Arduino profile:

```text
RGB display initializes;
PSRAM LVGL draw buffers allocate;
GT911 initializes and reports touch;
button/slider/control intent can be routed;
static dashboard can run for a long ALIVE period.
```

But the dynamic behavior is not acceptable for user-facing applications:

```text
10_LVGL_BasicUI    : functional, but touch quality open;
11_LVGL_Dashboard  : static/manual-touch diagnostic, dynamic UX not acceptable.
```

The current local manual-touch workaround should not be polished further as a product UI path.

## WT32-SC01-PLUS reference pattern

Known-good internal reference:

```text
https://github.com/AIDevelopersMonster/WT32-SC01-PLUS-Lab/tree/main/libraries/WT32_SC01_PLUS/examples/13_LVGL_BasicUI
```

The useful pattern is architectural, not pin-compatible:

```text
Arduino application
    -> LVGL widgets/events
    -> clean BSP API
    -> display + touch + backlight drivers
```

Important properties of the WT32 example:

```text
application sketch contains no board GPIO table;
application sketch contains no direct display-controller register code;
application sketch contains no direct touch-controller register code;
LVGL flush calls board.display().drawRGB565(...);
LVGL pointer read calls board.touch().read(point);
backlight slider calls board.backlight().set(...);
BSP owns coordinate mapping and hardware-specific details.
```

This style is the preferred direction for the ESP32-8048S043 library.

## Third-party audit order

```text
1. rzeldent/platformio-espressif32-sunton      AUDITED FIRST-PASS
2. rzeldent/esp32-smartdisplay                NEXT
3. limpens/esp32-8048S043                     OPEN
4. limpens/esp32-8048S043-lvgl9               OPEN
5. pixelwave/Sunton-ESP32-8048S043            OPEN
6. clumsyCoder00/Sunton-ESP32-8048S043        PARTIAL PRIOR INSPECTION
7. wegi1/ESP32-8048S043-4INCH-LCD             OPEN
8. ffodGit/esp32-8048s043-getting-started-00  OPEN
```

Detailed first audit:

```text
docs/third-party/rzeldent-platformio-espressif32-sunton.md
```

## First audit result: rzeldent/platformio-espressif32-sunton

Useful findings:

```text
board is modeled as PlatformIO JSON definitions rather than ad-hoc sketch constants;
ESP32-8048S043 family is split into C/N/R variants;
C variant matches our capacitive GT911 specimen most closely;
N variant is display-only;
R variant uses resistive XPT2046 and is not our specimen;
profile uses 16 MB flash, QIO/OPI, PSRAM, 240 MHz, Arduino + ESP-IDF frameworks;
profile defines ST7262 parallel RGB display path;
profile uses PSRAM-backed full-frame LVGL buffer concept;
profile sets explicit DMA buffer and queue parameters;
profile uses 12.5 MHz PCLK for the 800x480 panel;
GT911 pins 19/20/38/18 and SD pins 10/11/12/13 agree with our tested paths.
```

Important caution:

```text
Upstream is GPL-3.0: reference only, no direct code/file copy.
Upstream RGB R/B channel naming differs from our current physically tested Arduino_GFX pin header.
Do not replace our pin map blindly.
The next step is to inspect rzeldent/esp32-smartdisplay to see how these macros are consumed.
```

## Third-party study goals

For ESP32-8048S043 / Sunton-style boards, study third-party projects for:

```text
RGB panel timing and buffering strategy;
Arduino_GFX vs esp_lcd RGB panel usage;
LVGL partial buffer vs direct framebuffer behavior;
GT911 coordinate mapping and interrupt/polling strategy;
recommended LVGL tick/timer handling;
how touch is filtered before reaching widgets;
whether working projects avoid frequent whole-screen invalidation;
how they organize BSP, app, UI-generated files and assets.
```

## Acceptance target for future UI work

A future user-facing LVGL example should not merely be functional. It should satisfy:

```text
idle screen has no periodic jump/tear;
touching empty areas does not trigger visible redraw artifacts;
button press redraw is limited and not irritating;
slider/drag control feels stable enough for normal use;
architecture is reusable and BSP-centered;
no direct GPIO/register code in application examples.
```

## Current decision

Freeze the current local dashboard path as evidence and diagnostic work. Continue by studying third-party and WT32-style LVGL organization, then port only the clean architectural parts into this repository.
