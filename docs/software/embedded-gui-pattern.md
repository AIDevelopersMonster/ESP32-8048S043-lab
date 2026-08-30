# Embedded GUI pattern for ESP32-8048S043

Status: `DESIGN GUIDE / DERIVED FROM PROJECT EXPERIMENTS`.

This note records the GUI architecture that currently looks most promising for ESP32-S3 800x480 RGB embedded systems in this lab.

It is based on our own physical experiments plus mechanism-level observations from external reference projects. It is not a copy of any third-party application.

## 1. Separate the layers

Use a strict stack:

```text
Application state
      ↓
UI components / pages
      ↓
LVGL event model
      ↓
LVGL display + input port
      ↓
Board BSP
      ↓
RGB display / GT911 / backlight
```

Application logic should not know RGB pin numbers, GT911 calibration constants or framebuffer details.

The BSP should not know the meaning of buttons, recipes, pumps, dashboards or navigation.

## 2. Prefer event-driven redraw

For embedded HMI, redraw only when state changes.

Good baseline:

```text
user input
  -> event callback
  -> update application state
  -> update only affected widgets
  -> LVGL invalidates only changed areas
```

Avoid periodic label changes, decorative timers and animation while validating the display path. They make transport artifacts harder to separate from application behavior.

## 3. Treat draw memory and scan-out memory as different resources

On ESP32-S3 RGB panels, the display engine continuously consumes memory bandwidth while LVGL is also producing pixels.

The current promising strategy is:

```text
LVGL working buffers in INTERNAL SRAM
              ↓
partial-area copy through Arduino_GFX
              ↓
RGB bounce buffer in INTERNAL SRAM
              ↓
main RGB framebuffer / scan-out path
```

The important concept is isolation: rendering work should not unnecessarily compete with RGB scan-out for the same PSRAM access path.

## 4. Prefer two modest draw buffers over one huge buffer when testing partial mode

A useful starting point for 800x480 RGB565 is:

```text
20 lines per draw buffer
800 * 20 * 2 bytes = 32,000 bytes per buffer
2 buffers = about 64 KB total
```

This is small enough to make strict internal-SRAM allocation realistic while still giving LVGL a double-buffered partial-render pipeline.

Larger buffers can improve throughput but can force allocation into PSRAM and therefore change the memory-contention problem being tested.

## 5. Fail loudly when a controlled memory condition is not met

For experiments, do not silently fall back from SRAM to PSRAM when SRAM placement is the variable under test.

Preferred diagnostic behavior:

```text
allocate buffer A in internal SRAM
allocate buffer B in internal SRAM
verify esp_ptr_internal()
if either fails -> stop test and print FAIL
```

A production application may choose graceful fallback, but a laboratory comparison should preserve the experimental condition.

## 6. Distinguish redraw speed from redraw stability

Visual classification should separate at least four cases:

```text
FAST + STABLE
SLOW + STABLE
FLICKER / BLACK-LIKE TRANSITION
JITTER / CHATTER
```

A slow deterministic redraw is not the same failure mode as unstable scan-out or repeated visual chatter.

This distinction was important in the Robot-Core-Display physical reproduction: redraw was visibly slow, but the previous jitter/chatter behavior was not observed.

## 7. Turn the backlight on after the first valid frame

A useful startup sequence is:

```text
backlight OFF
initialize RGB panel
initialize LVGL
create UI
run several LVGL handler cycles
backlight ON
```

This avoids exposing uninitialized framebuffer contents and makes startup visually cleaner.

## 8. Keep UI components independent from transport

Reusable components should expose semantic behavior:

```text
Button -> clicked event
Slider -> value changed event
Card -> selected / inactive state
Status -> text / severity
```

They should not call Arduino_GFX directly.

All display writes should remain inside the LVGL flush callback / display-port layer.

## 9. Keep touch normalization in the BSP

The application should receive screen coordinates, not GT911 raw coordinates.

Preferred flow:

```text
GT911 raw point
  -> BSP normalization/calibration
  -> 0..799 / 0..479 coordinates
  -> LVGL pointer callback
  -> widget event
```

This keeps UI code portable across panel revisions and touch-controller calibration changes.

## 10. Use a minimal HMI before a dashboard

A good acceptance UI has only a few dynamic elements:

```text
one button
one counter
one slider
one state card
one static status/footer
```

Only after this path is visually stable should the project add:

```text
multiple pages
animations
images
network-driven updates
charts
Widget Runtime
```

## 11. Add radio/network activity only after the local HMI is stable

Wi-Fi and ESP-NOW can change memory/bus timing and CPU scheduling.

Recommended progression:

```text
local static HMI
-> local interactive HMI
-> repeated touch stress
-> Wi-Fi scan
-> active Wi-Fi traffic
-> ESP-NOW traffic
-> OTA / web activity
```

The display should be judged again at each stage.

## 12. Instrument the runtime

Every serious GUI example should print enough diagnostics to explain what was actually tested:

```text
firmware ID
ESP-IDF version
LVGL version
PCLK/timings
buffer sizes and memory region
bounce-buffer size
GT911 identity
flush count
input read count
press/release count
free heap
free PSRAM
```

This makes a visual video useful as reproducible evidence rather than just a demonstration.

## Current implementation

The first library example that applies this pattern is:

```text
libraries/ESP32_8048S043/examples/18_LVGL8_ArduinoGFX_BounceBufferUI/
```

Its controlled purpose is to compare Test 17 against:

```text
LVGL 8
+ two strict internal-SRAM buffers
+ RGB bounce buffer
+ partial redraw
+ own BSP GT911
```

Physical validation is still required before promoting the mechanism to the main BSP architecture.
