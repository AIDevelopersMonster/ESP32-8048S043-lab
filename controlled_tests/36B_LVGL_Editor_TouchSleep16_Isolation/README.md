# Test 36B — LVGL touch wake/sleep isolation

## Status

**BUILD PASS / FLASH PASS / PHYSICAL NO IMPROVEMENT / HYPOTHESIS FALSIFIED**

Parent baseline: Test 36 (`halyssonJr/lvgl-demo-esp32s3`, commit `79e862ca332525ba8721c4691f450fb44ec08738`).

Observed parent behavior on the physical ESP32-8048S043 specimen:

```text
Display/UI               PASS
GT911 touch              functional but degraded
Missed button presses    observed
Delayed/stuck press      observed
Production-quality UX    FAIL
```

## Single intended delta

```text
#define LVGL_TASK_SLEEP 500
->
#define LVGL_TASK_SLEEP 16
```

No other source or configuration change was allowed in this test.

Unchanged:

```text
GT911 interrupt          disabled / GPIO_NUM_NC
I2C                      400 kHz
GT911 coordinate map     unchanged
LVGL                     9.3.0
esp_lvgl_port             2.6.0
PCLK                      18 MHz
RGB timing                unchanged
RGB framebuffers          2 in PSRAM
bounce                    10 lines
direct mode               true
bb_mode                   true
avoid_tearing             true
XML/generated C UI        unchanged
```

## Physical result

The Test 36B firmware was built, flashed and monitored on the physical board.

User observation:

```text
COM output is the same as Test 36.
Touch acquisition remains non-guaranteed and highly selective after the first press.
Missed presses remain.
Apparently stuck/delayed button presses remain.
No meaningful responsiveness improvement from 500 ms -> 16 ms.
```

Representative application log contained only a small subset of attempted clicks, for example:

```text
DEMO: Button Name : Power
DEMO: Button Name : Settings
DEMO: Button Name : Social
```

The firmware otherwise booted normally, identified the GT911 (`TouchPad_ID: 0x39,0x31,0x31`) and reported GT911 config version 65.

## Conclusion

```text
Test 36 baseline   task_max_sleep_ms = 500   -> degraded touch
Test 36B           task_max_sleep_ms = 16    -> same degraded touch
```

Therefore:

> Reducing `task_max_sleep_ms` from 500 ms to 16 ms is not sufficient to improve the observed GT911 input behavior on this specimen.

The original hypothesis that the degraded touch was primarily caused by the 500 ms LVGL task maximum sleep is **falsified for this configuration**.

## Why the sleep change had little leverage

Further source audit showed that the non-interrupt input device is created in LVGL timer mode. LVGL 9.3 creates the indev read timer using `LV_DEF_REFR_PERIOD`, and the pinned upstream `sdkconfig` sets:

```text
CONFIG_LV_DEF_REFR_PERIOD=33
```

Thus the touch path already has an approximately 33 ms LVGL indev timer. `task_max_sleep_ms` is an upper sleep bound, not the direct GT911 polling interval in this configuration. This explains why `500 -> 16 ms` was not a strong control variable.

## New strongest comparison

A known-good physical reference, Test 20 (`limpens/esp32-8048S043-lvgl9`), uses the same board family and GT911 but uses the modern ESP-IDF I2C master API:

```text
driver/i2c_master.h
i2c_new_master_bus()
glitch_ignore_cnt = 7
internal pull-up enabled
```

Its touch was physically observed as excellent.

Test 36 instead uses the deprecated legacy I2C driver:

```text
driver/i2c.h
i2c_param_config()
i2c_driver_install()
legacy bus index cast into esp_lcd panel IO
```

and ESP-IDF 5.5.5 prints the corresponding legacy-driver warning at boot.

The next controlled derivative should therefore return to the exact Test 36 baseline (`task_max_sleep_ms=500`) and change only the GT911 I2C transport backend from legacy to modern master-bus API, while preserving pins, 400 kHz, GT911 component, coordinate mapping, LVGL, UI and display transport.

## Final classification

```text
BUILD                     PASS
FLASH                     PASS
PHYSICAL EXECUTION        PASS
TOUCH IMPROVEMENT         NO
CAUSAL HYPOTHESIS         FALSIFIED
DISPLAY REGRESSION        NONE REPORTED
NEXT                      Test 36C modern-I2C backend isolation
```
