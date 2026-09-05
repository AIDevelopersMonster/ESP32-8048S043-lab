# Test 36C — GT911 modern I2C backend isolation

## Status

**CONTROLLED DERIVATIVE PREPARED / BUILD AND PHYSICAL VERDICT PENDING**

Parent baseline: Test 36 (`halyssonJr/lvgl-demo-esp32s3`, pinned commit `79e862ca332525ba8721c4691f450fb44ec08738`).

Test 36 physical result:

```text
Display/UI               PASS
GT911 touch              functional but degraded
Missed button presses    observed
Delayed/stuck press      observed
Production-quality UX    FAIL
```

Test 36B changed only `task_max_sleep_ms` from 500 ms to 16 ms and produced **no touch improvement**. Therefore Test 36C returns to the exact Test 36 runtime baseline (`LVGL_TASK_SLEEP=500`) and tests the next independent variable.

## Single experimental variable

Replace only the GT911 I2C transport backend:

```text
legacy ESP-IDF I2C backend
  driver/i2c.h
  i2c_param_config()
  i2c_driver_install()
  integer bus id passed to esp_lcd_new_panel_io_i2c()

->

modern ESP-IDF I2C master backend
  driver/i2c_master.h
  i2c_new_master_bus()
  i2c_master_bus_handle_t passed to esp_lcd_new_panel_io_i2c()
```

The small CMake dependency update required to compile the new driver and the handle-plumbing changes required to pass the modern bus handle are part of this one transport-backend variable.

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

### Why 400 kHz is set explicitly

The GT911 panel-IO convenience macro in the current component defaults to 100 kHz. In Test 36 the legacy bus itself was initialized at 400 kHz. Test 36C therefore explicitly sets:

```c
tp_io_cfg.scl_speed_hz = 400000;
```

when using the modern per-device I2C API. This preserves the intended/effective Test 36 bus speed rather than introducing a second experimental variable.

## Why this is a strong next test

Known-good physical reference Test 20 (`limpens/esp32-8048S043-lvgl9`) uses the modern ESP-IDF I2C master API and had excellent GT911 touch behavior on this board family. Its relevant I2C pattern includes:

```text
driver/i2c_master.h
i2c_new_master_bus()
glitch_ignore_cnt = 7
internal pull-up enabled
```

Test 36 uses the deprecated legacy I2C driver and ESP-IDF 5.5.5 prints the legacy-driver warning during boot.

Test 36C asks only:

```text
Does replacing the legacy I2C transport with the modern master-bus transport make GT911 acquisition reliable?
```

## Expected boot evidence

The Test 36 legacy boot contains:

```text
W (...) i2c: This driver is an old driver ...
```

For a valid Test 36C build this warning should disappear because `driver/i2c.h` is no longer used by our GT911 bus setup.

The GT911 itself should still identify as before, for example:

```text
GT911: TouchPad_ID:0x39,0x31,0x31
GT911: TouchPad_Config_Version:65
```

## Run

From the lab repository:

```powershell
git fetch
git switch --track origin/agent/test36c-modern-i2c-isolation

powershell -ExecutionPolicy Bypass -File .\controlled_tests\36C_LVGL_Editor_ModernI2C_Isolation\run-test36c.ps1
```

Then flash, for example COM7:

```powershell
powershell -ExecutionPolicy Bypass -File .\controlled_tests\36C_LVGL_Editor_ModernI2C_Isolation\run-test36c.ps1 -Upload -UploadPort COM7
```

## Physical comparison

Use exactly the same six Stream Deck cards and compare directly with Tests 36/36B:

```text
20 rapid short taps on one card
20 slow deliberate taps on one card
alternate two adjacent cards 20 times
press + immediate release
press-and-hold + release
repeat after 30-60 seconds idle
```

Record:

```text
attempted presses
serial callback count
missed taps
late callbacks
stuck pressed state
release delay
subjective responsiveness
any display regression
```

A dramatic improvement would strongly implicate the old I2C transport/integration. No improvement would falsify that candidate and move the investigation deeper into GT911 acquisition/state handling or LVGL integration.
