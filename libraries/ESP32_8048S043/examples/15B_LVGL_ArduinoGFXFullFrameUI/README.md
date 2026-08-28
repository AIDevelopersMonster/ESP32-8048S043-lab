# 15B_LVGL_ArduinoGFXFullFrameUI

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Purpose:

```text
A/B comparison against 15_LVGL_EspLcdBasicUI using the redraw mechanism that
behaved well in a physically tested third-party firmware on the same board family.
```

Architecture:

```text
LVGL 8
  -> one full-screen 800x480 RGB565 direct buffer in PSRAM
  -> Arduino_GFX
  -> full 800x480 frame push every loop
  -> ESP32_8048S043_Touch BSP as LVGL pointer input
```

Important provenance boundary:

```text
The mechanism is independently reimplemented.
No third-party application source or generated EEZ UI code is copied here.
```

What changed compared with test 15:

```text
15: native esp_lcd + partial LVGL flushes to esp_lcd_panel_draw_bitmap()
15B: Arduino_GFX + LVGL direct_mode + complete framebuffer pushed every loop
```

What remains intentionally stressful:

```text
normal visible LVGL button pressed state;
Pressed and Clicks counters;
touch/status label updates;
5-second status label update;
stable white border;
GT911 through the existing BSP.
```

This is deliberate. Test 16 already showed that removing invalidation makes the
screen stable. Test 15B asks a different question:

```text
Can a visually active LVGL 8 UI behave well if we keep the same kind of UI activity
but change only the redraw transport to the full-frame Arduino_GFX mechanism?
```

Firmware ID:

```text
15B-LVGL-GFXFULL1-240828A
```

Dependencies:

```text
Arduino_GFX_Library
LVGL 8.x
ESP32_8048S043 BSP library
```

Expected build profile:

```text
ESP32-8048S043 Lab N16R8 FIXED
16 MB flash
OPI PSRAM
LV_COLOR_DEPTH 16
```

Expected Serial highlights:

```text
[PASS] gfx->begin()
[PASS] ESP32_8048S043_Touch::begin() ...
[PASS] full LVGL framebuffer allocated in PSRAM: 768000 bytes ...
[PASS] LVGL full-frame direct display driver registered
[PASS] LVGL GT911 BSP pointer driver registered
[PASS] LVGL comparison UI objects created
[PASS] Backlight ON after first full-frame transfer
[READY] Compare against test 15 ...
```

Physical validation checklist:

```text
1. Idle for 15-30 seconds.
2. Is there periodic self-redraw/flicker?
3. Press the button gently.
4. Hard-tap the button.
5. Does the white border remain stable?
6. Do Pressed and Clicks counters work?
7. Does the status update every 5 seconds cause visible disturbance?
8. Do readFail and pointFail stay at 0?
9. Compare subjectively with:
   - 15_LVGL_EspLcdBasicUI
   - 16_LVGL_EspLcdMinimalInvalidation
   - external clumsyCoder00 firmware
```

Interpretation matrix:

```text
15B visually good, including status/button redraw:
    full-frame Arduino_GFX redraw mechanism is a strong practical candidate.

15B idle good but hard-tap jitter remains:
    touch/button interaction still contributes, but full-frame path reduces transport artifacts.

15B worse or unstable:
    do not adopt full-frame redraw blindly; continue native esp_lcd sync investigation.
```

PASS boundary:

```text
Do not mark PASS until physical Sample A evidence is recorded.
```
