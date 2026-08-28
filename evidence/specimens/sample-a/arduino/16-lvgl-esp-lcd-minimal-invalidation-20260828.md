# ESP32-8048S043 / Sample A / Arduino 16_LVGL_EspLcdMinimalInvalidation

Status: `FUNCTIONAL PASS CANDIDATE / IDLE STABLE / HARD-TAP JITTER OPEN`.

Date: 2026-08-28

Specimen:

```text
Sample A
ESP32-8048S043 / ESP32-S3 / 800x480 RGB panel / GT911 family
```

Example:

```text
libraries/ESP32_8048S043/examples/16_LVGL_EspLcdMinimalInvalidation
```

Firmware ID tested:

```text
16LVGL-MINV1-240828A
```

## Purpose

This test followed the 15th combined LVGL display/touch test:

```text
15_LVGL_EspLcdBasicUI
FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE
```

The goal was to remove almost all intentional UI invalidation and determine whether the previous visual defects were caused by periodic label updates, visible button-state redraws, or a more fundamental RGB/LVGL synchronization issue.

## Runtime identity

Observed runtime profile:

```text
Firmware ID          : 16LVGL-MINV1-240828A
ESP-IDF SDK          : v5.5.5
ARDUINO_BOARD        : ESP32_8048S043_LAB
ARDUINO_VARIANT      : esp32_8048s043_lab
LVGL version         : 8.3.11
LV_COLOR_DEPTH       : 16
Flash                : 16777216 bytes
PSRAM                : 8388608 bytes
Free PSRAM at boot   : 8384788 bytes
Free heap at boot    : 291816 bytes
PCLK                 : 12500000 Hz
Framebuffer in PSRAM : true
Double framebuffer   : true
Font mode            : LV_FONT_DEFAULT only
Periodic UI update   : disabled
Visible press style  : transparent click target
```

## Init result

All init stages passed:

```text
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=1
[PASS] lvBuf1 allocated in PSRAM: 128000 bytes
[PASS] lvBuf2 allocated in PSRAM: 128000 bytes
[PASS] LVGL display driver registered
[PASS] LVGL GT911 pointer driver registered
[PASS] LVGL minimal-invalidation UI objects created
[PASS] Backlight ON after LVGL first draw
```

## Physical observation

Operator report:

```text
Idle for 15 seconds: no self-redraw every 1-3 seconds.
Gentle phone-like touch / careful touch: no visible jitter.
Hard tapping on the target: screen jitter can appear.
Click counter increases after release.
White border remains stable.
HITBOX pressed/clicked events are printed.
```

The uploaded serial log confirms the functional path and failure counters:

```text
clicked=38
pressed=38
reports=715
releases=38
flush=82
readFail=0
pointFail=0
uptime=85s
heap=285924
psram=8388608
freePsram=6589028
```

## Interpretation

This is a significant improvement over test 15:

```text
The previous periodic redraw defect disappeared when periodic UI invalidation was removed.
The quiet LVGL scene can remain visually stable while idle.
The GT911-to-LVGL event path remains functional.
Clicks and releases are correctly delivered.
The white border remains stable.
```

The remaining open issue is narrower:

```text
hard physical tapping can still provoke visible jitter;
gentle phone-like touch is acceptable in this first report.
```

This suggests that the immediate rejected behavior in test 15 was not purely a display pin-map, color-order, or GT911 detection problem. It was strongly related to unnecessary LVGL invalidation and/or touch-time redraw behavior.

## Decision

Promote this test to:

```text
FUNCTIONAL PASS CANDIDATE / IDLE STABLE / HARD-TAP JITTER OPEN
```

Do not yet promote it to a polished user-facing HMI template.

Recommended next isolation step:

```text
17_LVGL_EspLcdManualHitbox
```

Candidate purpose:

```text
Do not register GT911 as a LVGL pointer driver.
Read GT911 manually in the sketch.
Do manual hit testing against the central target.
Do not invalidate anything on press.
Update only a small counter label after release.
```

This would separate:

```text
LVGL pointer-state invalidation during press
from
manual touch handling with click-only LVGL update.
```

Alternative lower-level path remains:

```text
16B_DisplayEspLcdVsyncProbe
```

if even click-only manual updates still produce unacceptable artifacts.

## PASS boundary

A positive result here means:

```text
LVGL 8 over native esp_lcd can run a quiet interactive screen with GT911 BSP touch and click-only invalidation on Sample A, with stable idle display and correct click delivery.
```

It does not prove:

```text
hard-tap robustness;
slider/drag stability;
fast animation;
full dashboard UI;
Widget Runtime;
Web setup or OTA;
LVGL 9 migration.
```
