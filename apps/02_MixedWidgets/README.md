# App 02 — Mixed Widgets

Status: **BUILD CANDIDATE / PHYSICAL TEST PENDING**

App 02 keeps the physically validated App 01 hardware/runtime baseline unchanged and varies only the LVGL UI layer.

## Widgets under test

1. **COMMAND** — full-card button; the entire card is one touch target.
2. **SWITCH** — non-clickable card container; only the `lv_switch` is interactive.
3. **SLIDER** — large horizontal slider with enlarged knob/click area.
4. **ARC** — rotary value control with enlarged knob/click area.
5. **PROGRESS / STATUS** — display-only bar driven live by the slider.
6. **NAVIGATION** — full-card button opening a second status screen; BACK returns to controls.

## Touch ownership rule

One semantic control has one intentional touch owner. Decorative labels and containers are explicitly non-clickable. This is the retained fix from the Test36 Stream Deck hit-test investigation.

## Frozen hardware baseline

- ESP32-S3
- 800x480 RGB565
- PCLK: 16 MHz
- RGB bounce buffer: 10 lines
- one PSRAM framebuffer
- LVGL partial draw buffer: 60 lines in INTERNAL RAM
- GT911: SDA 19, SCL 20, RST 38
- ESP-IDF 5.5.x
- LVGL 9.3.0

## Expected Serial evidence

```text
WIDGET:COMMAND:RUN:1
WIDGET:SWITCH:OFF
WIDGET:SWITCH:ON
WIDGET:SLIDER:72
WIDGET:ARC:48
WIDGET:NAV:STATUS
WIDGET:NAV:CONTROLS
```

## Physical PASS checklist

- display stable / no flicker
- touch alignment correct
- COMMAND responds across the full card
- SWITCH toggles without parent-card interception
- SLIDER drag is continuous and precise
- ARC drag is continuous and precise
- PROGRESS follows SLIDER live
- NAVIGATION opens status page and BACK returns
- Serial output matches the manipulated widget
- UI task stack remains stable

Do not add App 02 to the public Web Flasher until this checklist is physically validated.
