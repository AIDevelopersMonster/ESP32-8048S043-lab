# Test 36C — GT911 modern I2C backend isolation

## Status

**BUILD PASS / FLASH PASS / PHYSICAL NO IMPROVEMENT / MODERN-I2C HYPOTHESIS FALSIFIED**

Parent baseline: Test 36 (`halyssonJr/lvgl-demo-esp32s3`, pinned commit `79e862ca332525ba8721c4691f450fb44ec08738`).

Test 36 physical result:

```text
Display/UI               PASS
GT911 touch              functional but degraded
Missed button presses    observed
Delayed/stuck press      observed
Production-quality UX    FAIL
```

Test 36B changed only `task_max_sleep_ms` from 500 ms to 16 ms and produced no touch improvement. Test 36C returned to the exact Test 36 runtime baseline (`LVGL_TASK_SLEEP=500`) and changed only the GT911 I2C transport backend.

## Single experimental variable

```text
legacy ESP-IDF I2C backend
  driver/i2c.h
  i2c_param_config()
  i2c_driver_install()
  integer bus id

->

modern ESP-IDF I2C master backend
  driver/i2c_master.h
  i2c_new_master_bus()
  i2c_master_bus_handle_t
```

Required handle plumbing and the CMake dependency update are part of the transport-backend substitution.

## Preserved variables

```text
upstream commit           79e862ca332525ba8721c4691f450fb44ec08738
LVGL task max sleep       500 ms
LVGL refresh period       33 ms
GT911 component           unchanged
GT911 INT                 disabled / GPIO_NUM_NC
GT911 RST                 GPIO38
SDA                       GPIO19
SCL                       GPIO20
I2C logical port          I2C_NUM_1
I2C speed                 400 kHz
coordinate mapping        raw X 0..477 -> 0..800
                          raw Y 0..269 -> 0..480
LVGL                      9.3.0
esp_lvgl_port             2.6.0
PCLK                      18 MHz
RGB timings               unchanged
RGB framebuffers          2 in PSRAM
bounce                    10 lines
direct mode               true
bb_mode                   true
avoid_tearing             true
XML/generated-C UI        unchanged
button callback           serial log only
```

## Valid boot evidence

The physical Test 36C boot confirmed that the intended new backend was actually running:

```text
I2C DEV: Modern I2C master bus installed: port=1 SDA=19 SCL=20
GT911: TouchPad_ID:0x39,0x31,0x31
GT911: TouchPad_Config_Version:65
```

The legacy-driver warning seen in Test 36 disappeared.

Therefore this was a valid modern-I2C reproduction rather than an accidental rebuild of the legacy path.

## Quantified physical result

The user pressed each of the six Stream Deck cards exactly ten times with approximately 500 ms between presses:

```text
6 buttons x 10 presses = 60 physical click attempts
```

Only nine `LV_EVENT_CLICKED` application callbacks appeared in the captured COM log:

```text
Power      1
Media      7
Settings   1
----------------
Total      9 / 60
```

The upstream UI contains duplicate card labels, so the textual label distribution is not suitable for identifying every physical card. The decisive metric is the total callback count.

Observed callback efficiency:

```text
9 / 60 = 15%
51 / 60 = 85% missed at application-click level
```

The 500 ms press spacing is far slower than the pinned LVGL 33 ms input refresh period, so this cannot reasonably be explained merely by taps being shorter than one normal LVGL polling interval.

## Conclusion

```text
Test 36    legacy I2C  -> severely degraded touch
Test 36C   modern I2C  -> severely degraded touch, 9/60 clicked callbacks
```

Therefore:

> Replacing the legacy ESP-IDF I2C transport with the modern `i2c_master` backend is not sufficient to make the Test 36 GT911 input reliable.

The modern-I2C backend hypothesis is **falsified as the primary cause** for the observed application-level missed clicks.

This does not prove that both I2C implementations are electrically or temporally identical. It proves only that changing this transport backend did not remove the defect under the tested conditions.

## Next diagnostic step

The failure must now be localized across the input stack:

```text
physical touch
  -> GT911 status/data acquisition
  -> esp_lcd_touch_get_coordinates()
  -> esp_lvgl_port indev state
  -> LVGL object hit/press/release processing
  -> LV_EVENT_CLICKED
```

Test 36D should be diagnostic rather than another blind parameter change. It should log state transitions at the GT911/esp_lvgl_port boundary before LVGL object processing and, separately, the button-level `PRESSED`, `RELEASED`, `PRESS_LOST`, and `CLICKED` events.

With the same 10 presses per card / 500 ms protocol:

```text
raw transitions ~60, clicked ~9
    -> acquisition is good; loss occurs in LVGL hit/click semantics

raw transitions also ~9
    -> loss occurs at/below GT911 acquisition/driver boundary

PRESS seen but RELEASE/PRESS_LOST abnormal
    -> explains stuck-looking state and missing CLICKED events
```

## Final classification

```text
BUILD                     PASS
FLASH                     PASS
MODERN I2C ACTIVE         CONFIRMED
GT911 IDENTIFICATION      PASS
DISPLAY                   PASS
PHYSICAL CLICK ATTEMPTS   60
APPLICATION CLICKED       9
CLICK YIELD               15%
TOUCH IMPROVEMENT         NO
CAUSAL HYPOTHESIS         FALSIFIED
NEXT                      Test 36D raw-to-LVGL diagnostic instrumentation
```
