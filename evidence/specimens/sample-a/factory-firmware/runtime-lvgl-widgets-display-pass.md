# Sample A factory LVGL Widgets display runtime PASS

Status: `PHYSICAL DISPLAY RUNTIME PASS`.

This note records user-confirmed photo evidence from the physical Sample A board booting the preserved factory application. The photos were used to identify the running UI as the standard LVGL 8 widgets demo (`lv_demo_widgets`).

## Observed UI identity

The physical screen showed the recognizable LVGL 8 widgets demo with the following visible elements:

- `LVGL v8 / Widgets demo` text;
- `Profile / Analytics / Shop` tabs;
- `Elena Smith` profile section;
- charts / analytics widgets;
- images, fonts and complex LVGL widgets;
- lower performance indicator.

## Observed display behavior

The photo evidence supports the following display-level result:

| Item | Result |
|---|---|
| Factory app launch | PASS |
| LVGL widgets demo visible | PASS |
| 800x480 panel output | PASS |
| Orientation | PASS / visually correct |
| RGB color order | PASS / visually correct |
| Complex widgets/charts/images/fonts | PASS |
| Approximate FPS shown by demo | about 66 FPS |
| CPU load shown by demo | about 16-18% |

## Interpretation

This is stronger than a basic color-bar or backlight test. The factory application successfully runs a full LVGL GUI workload on the ESP32-8048S043 800x480 RGB display.

Working result:

```text
Factory LVGL Widgets Demo display output = PASS
```

This also provides a useful benchmark baseline for comparing this board family with other lab boards, because `lv_demo_widgets` is a common LVGL visual/performance demo.

## Relationship to previous evidence

This runtime display result is consistent with:

- static strings identifying `LVGL Widgets Demo` and `LVGL v8`;
- static strings identifying `Arduino_GFX` and `Arduino_ESP32RGBPanel`;
- serial boot evidence where the app prints `LVGL Widgets Demo` and reaches `Setup done`;
- partition evidence showing the useful factory application is stored in `app0`.

## Boundary

This is a display/runtime PASS, not a full board PASS.

Still not proven by this evidence alone:

- touch behavior;
- touch controller identity;
- exact GT911/Goodix confirmation;
- exact RGB pin map;
- SD card behavior;
- Wi-Fi/BLE behavior;
- factory diagnostic/test menu.

Photos/video should be added separately under the evidence tree or linked from the video registry when available.
