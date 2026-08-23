# Sample A factory app identity summary

Status: `STATIC LEAD ONLY`.

This note summarizes the strongest non-secret string evidence from the local factory image analysis. It does not publish the raw `strings.txt` file or the factory `.bin` dump.

## Source evidence

Generated local report:

```text
evidence/specimens/sample-a/factory-firmware/analysis/keyword-summary.md
```

Related partition-level evidence:

```text
evidence/specimens/sample-a/factory-firmware/analysis/partition-report/partition-report.md
```

## High-confidence application identity leads

| Offset | String |
|---:|---|
| 0x00010120 | `LVGL Widgets Demo` |
| 0x000103B5 | `LVGL v8` |
| 0x0002A164 | `esp_lcd_new_rgb_panel(_panel_config, &_panel_handle)` |
| 0x0002A199 | `<WINDOWS_PATH>\Arduino_GFX-master\src\databus\Arduino_ESP32RGBPanel.cpp` |
| 0x0002A201 | `esp_lcd_panel_reset(_panel_handle)` |
| 0x0002A224 | `esp_lcd_panel_init(_panel_handle)` |
| 0x0002A284 | `Arduino_ESP32RGBPanel::getFrameBuffer(...)` |
| 0x0002A4F5 | `<WINDOWS_PATH>\2.0.3\cores\esp32\esp32-hal-uart.c` |
| 0x00031CDB | `esp_lcd_panel_init` |
| 0x00031CEE | `esp_lcd_panel_reset` |
| 0x00032037 | `esp_lcd_new_rgb_panel` |

## Interpretation

The preserved factory application in `app0` is strongly identified as a vendor/demo application using:

- LVGL v8;
- an `LVGL Widgets Demo` application label;
- Arduino ESP32 core 2.0.3 evidence;
- Arduino_GFX RGB panel integration;
- ESP-IDF `esp_lcd` RGB panel driver calls.

This supports the working description:

```text
Factory app = Arduino/LVGL RGB-panel vendor demo, stored in app0
```

## What is not proven yet

The current string evidence does not prove:

- exact runtime pin mapping;
- touch controller model;
- GT911 or Goodix usage;
- a factory diagnostic/test mode;
- runtime behavior on the physical LCD/touch panel;
- boot selection state from `otadata`.

## Current image structure context

From the partition-level report:

```text
app0   : contains the active ESP image and application strings
app1   : erased
spiffs : erased
nvs    : erased
```

Therefore, the useful factory logic is concentrated in `app0`. The empty `app1` and `spiffs` partitions indicate that this specimen does not ship with a second application image or populated filesystem data in the preserved dump.

## Boundary

This is static reverse-engineering evidence only. It is enough to guide safe lab examples and reproduction notes, but not enough to claim a physical display/touch PASS or a factory-test entry path.
