# Test 36B — LVGL touch wake/sleep isolation

## Status

**CONTROLLED DERIVATIVE PREPARED / PHYSICAL VERDICT PENDING**

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

No other source or configuration change is allowed in this test.

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

## Hypothesis

The parent config allows the LVGL port task to sleep for as long as 500 ms while the GT911 interrupt line is not connected to the port. A short press or release can therefore be observed late or missed between polling intervals.

Test 36B asks only:

```text
Does 500 ms -> 16 ms remove the missed/stuck touch behavior?
```

If yes, the no-interrupt + long-sleep scheduling configuration is strongly implicated.

If no, do not change anything else in this test. A later independent experiment can examine interrupt-driven touch or a different GT911 integration.

## Run

```powershell
git fetch
git switch --track origin/agent/test36b-touch-sleep16-isolation

powershell -ExecutionPolicy Bypass -File .\controlled_tests\36B_LVGL_Editor_TouchSleep16_Isolation\run-test36b.ps1
```

Then flash, for example COM7:

```powershell
powershell -ExecutionPolicy Bypass -File .\controlled_tests\36B_LVGL_Editor_TouchSleep16_Isolation\run-test36b.ps1 -Upload -UploadPort COM7
```

## Physical comparison

Use the same six cards and compare directly with Test 36:

```text
rapid short taps
slow deliberate taps
repeated tapping of one card
alternating adjacent cards
press + immediate release
press-and-hold + release
edge-of-card acquisition
```

Record:

```text
missed taps
late responses
stuck pressed state
release delay
subjective responsiveness
any display regression
```
