# Test 35 — Robot-Core-Display / applied LVGL HMI

## Status

**THIRD-PARTY CANDIDATE / BUILD + PHYSICAL VERDICT PENDING**

Upstream repository:

```text
Albert-Benavent-Cabrera/Robot-Core-Display
```

Upstream URL:

https://github.com/Albert-Benavent-Cabrera/Robot-Core-Display

Pinned upstream commit:

```text
1d95120dc43e7663dfb5888a4da079aae1929153
2026-01-26 06:08:22 UTC
chore: Complete Redesign & Documentation Update
```

License: GPL-3.0.

Test 35 does not vendor the upstream application source into the lab. The harness clones the exact public commit into a disposable work directory and restores the exact tree after the test.

## Why Test 35

After Tests 33 and 34 proved two ESPHome/LVGL dashboard architectures, the laboratory focus expands beyond transport stability into **GUI architecture, ergonomics and applied interaction design**.

Robot-Core-Display is especially valuable because it is not a generic widgets demo. It is a real application HMI built specifically for the 4.3-inch ESP32-8048S043.

The upstream application contains three main user flows:

```text
1. Cocktail selection
   -> visual gallery / cards
   -> choose a drink

2. Recipe configuration
   -> modal interaction
   -> ingredient quantities
   -> sliders / confirmation

3. Pump configuration
   -> four pump controls
   -> PWM and timing adjustment
   -> calibration-oriented layout
```

The UI is manually structured in native LVGL C++ and split into reusable components rather than one monolithic screen file.

## GUI / ergonomics value

Reusable UI structure at the pinned commit includes:

```text
display/src/ui/
  assets/
  components/
    MyButton
    MyIcon
    MyTitle
    card/
    footer/
    modal/
    slider/
  pages/
    page_cocktails.cpp
    page_config.cpp
    page_pumps.cpp
```

This makes Test 35 useful for studying:

- card-based selection on 800x480;
- visual hierarchy and page composition;
- modal editing instead of permanent control clutter;
- touch target sizing;
- slider ergonomics;
- color-coded controls;
- persistent footer/navigation patterns;
- separation of reusable components from application pages;
- image-heavy UI asset handling;
- behavior with mock/offline data versus remote ESP-NOW data.

The upstream redesign also introduced a shared data model/DataManager and remote persistence logic, so the UI is tied to an actual application state model rather than static demo values.

## Software stack

Upstream `platformio.ini` declares:

```text
Framework       Arduino
Board           esp32-s3-devkitc-1
Platform        Jason2866/platform-espressif32.git (moving URL upstream)
LVGL            ^9.1.0
TAMC_GT911      ^1.0.2
Arduino_GFX     vendored in upstream lib/
Flash           16 MB
PSRAM           qio_opi
CPU             240 MHz
```

The README identifies the intended GUI baseline as LVGL 9.1.0 and Arduino_GFX 1.6.4.

## Historical PlatformIO reconstruction

The upstream project uses a moving Git URL:

```text
https://github.com/Jason2866/platform-espressif32.git
```

For reproducibility, Test 35 temporarily pins that build-environment dependency to the latest commit available before the Robot-Core-Display upstream commit:

```text
Jason2866/platform-espressif32
816219db19399d376cfeac3bab6edd14b781701c
2026-01-19
```

That platform commit reports:

```text
Platform version                 2025.01.50
framework-arduinoespressif32     1901-1318-5.5
ESP-IDF package                  v5.5.2.260104
PlatformIO requirement           >= 6.1.18
```

Test 35 also resolves the upstream caret dependencies to the versions documented by the project:

```text
LVGL        9.1.0 exact
TAMC_GT911  1.0.2 exact
```

These are **temporary build-harness pins only**. The original upstream files are restored after build/upload.

## Exact display path at the pin

The current upstream `Config.hpp` contains:

```text
800 x 480 RGB
PCLK 16 MHz
PCLK inverted
H: front 8 / pulse 4 / back 20
V: front 8 / pulse 4 / back 8
RGB bounce buffer: 20 lines
LVGL draw lines: 100
LVGL double buffer: enabled
```

This is interesting relative to our lab history because it combines a driver-level RGB bounce buffer with large LVGL buffers and a real Wi-Fi/ESP-NOW application workload.

## Important touch compatibility observation

The exact pinned upstream source enables custom GT911 calibration:

```text
TOUCH_MAP_X2 330
TOUCH_MAP_Y2 220
```

Our own board has previously shown near-full native GT911 coverage in other references. Therefore **we do not assume this calibration is correct for our physical unit**.

The first Test 35 physical run keeps the upstream calibration unchanged. If display/UI is good but touch mapping is compressed or offset, that will be recorded as a board-revision/application calibration compatibility issue, not silently patched inside the baseline.

Any calibration correction would become a separate derived test.

## Offline physical test

The application normally uses ESP-NOW to communicate with the drinks machine and scans for a configured Wi-Fi SSID to synchronize the radio channel.

For our display/touch baseline, the runner creates a temporary `display/secrets.h` with a dummy SSID unless another SSID is supplied.

The upstream application includes mock cocktail and pump data, so the main UI can still be evaluated without the external drinks machine.

This lets us judge separately:

```text
Display / transport
Touch / ergonomics
UI architecture
Remote ESP-NOW integration
```

## Physical observation checklist

Record:

```text
Boot
Backlight
Cocktail gallery appears
Card size / spacing / readability
Card tap response
Recipe modal behavior
Slider capture and drag quality
Pump page layout
Footer/navigation behavior
Touch mapping / edge reachability
Image rendering quality
Redraw stability during interaction
Animation/transition behavior if present
Visible flicker
Horizontal jump
Reset/crash
```

Also record subjective ergonomic observations:

```text
Can controls be acquired reliably with a finger?
Are important controls visually obvious?
Is modal use clearer than a dense permanent settings page?
Are text and icons readable at normal viewing distance?
Is the page hierarchy immediately understandable?
```

## Relation to our existing transport evidence

Robot-Core-Display is especially interesting because its upstream authors independently converged on a transport pattern already supported by our causal tests:

```text
INTERNAL LVGL drawing resources
+ driver-level RGB bounce buffering
+ Arduino_GFX
```

Our earlier Test 29 established that, in our Arduino_GFX PSRAM-draw-buffer experiment, switching RGB bounce from zero to any tested non-zero value removed visible redraw flicker. Test 35 is **not** a causal continuation of that experiment, but it provides an independent real-application reference using bounce buffering under ESP-NOW/Wi-Fi workload.

## Planned next GUI-oriented projects

After Test 35:

```text
Test 36 candidate
halyssonJr/lvgl-demo-esp32s3
-> LVGL Editor XML workflow
-> Stream Deck UI
-> generated C
-> image/font resources and SPIFFS

Test 37 candidate
CelliesProjects/flocking-esp32
-> real-time boids animation
-> continuous redraw stress
-> touch interaction during animation
-> live parameter sliders

Test 38 candidate
clackups/draftling
-> full editor/file-browser application
-> split-screen
-> touch gestures: tap/double-tap/drag/flick
-> themes
-> keyboard + touch ergonomics
```

The new test series therefore evaluates not only whether the screen works, but also **what GUI architecture and interaction patterns are worth carrying into our own future firmware**.
