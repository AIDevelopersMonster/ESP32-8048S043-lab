# App 02 — Mixed Widgets

Status: **PHYSICAL PASS (v0.1.0) / v0.1.1 COSMETIC FIX CANDIDATE**

App 02 keeps the physically validated App 01 hardware/runtime baseline unchanged and varies only the LVGL UI layer.

## Physical evidence

- **v0.1.0 PHYSICAL PASS** on the ESP32-8048S043 sample board.
- Display stable, touch correct, COMMAND/SWITCH/SLIDER/ARC/PROGRESS/NAVIGATION all worked correctly.
- Slider and arc ergonomics were reported as excellent.
- Known cosmetic issue in v0.1.0: the top-right `BACK TO CONTROLS` button was slightly too narrow for its label.
- Video evidence: https://youtube.com/shorts/LwmW8UwDED0

The v0.1.1 candidate changes only the `BACK TO CONTROLS` button geometry from 200×48 at x=576 to 236×48 at x=540. No hardware, touch, widget, display, timing, memory, or callback behavior is changed.

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

- display stable / no flicker — PASS
- touch alignment correct — PASS
- COMMAND responds across the full card — PASS
- SWITCH toggles without parent-card interception — PASS
- SLIDER drag is continuous and precise — PASS
- ARC drag is continuous and precise — PASS
- PROGRESS follows SLIDER live — PASS
- NAVIGATION opens status page and BACK returns — PASS
- Serial output matches the manipulated widget — PASS
- UI task/runtime remained stable during the recorded test — PASS

Public Web Flasher publication should use the corrected v0.1.1 candidate only after the cosmetic button fix is physically confirmed.
