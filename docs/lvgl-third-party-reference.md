# LVGL third-party reference notes

Status: `OPEN / NEXT RESEARCH TRACK`.

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
