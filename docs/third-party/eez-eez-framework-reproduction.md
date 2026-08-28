# EEZ Studio + eez-framework on ESP32-8048S043

Status: `PHYSICALLY REPRODUCED / EXTERNAL THIRD-PARTY EXPERIMENT`.

This document records the exact Windows + PlatformIO workflow used to reproduce a working EEZ Studio / eez-framework / LVGL 9 firmware on the ESP32-8048S043 Sample A board.

The external application used for the experiment is:

```text
https://github.com/clumsyCoder00/Sunton-ESP32-8048S043
```

The experiment was intentionally performed outside this repository. The upstream application is GPL-2.0 and is treated here as an external firmware/reference project. This repository stores only the audit, reproduction procedure and our independently implemented follow-up experiments.

## Result

The external firmware was successfully:

```text
cloned;
dependency-pinned;
compiled;
flashed to Sample A;
booted;
rendered correctly;
operated by GT911 touch;
used to switch between its EEZ-generated screens.
```

Observed Serial identity:

```text
Arduino_GFX LVGL_Arduino example v9
Hello Arduino! V9.1.0
Init Display
TFT_BL
Setup done
```

Observed touch events included normal LVGL press/release reporting.

Physical operator result:

```text
Display works correctly.
Touch works correctly.
EEZ-generated screen navigation works correctly.
Overall visual behavior is good enough to use as a reference for our own redraw experiments.
```

Successful build summary:

```text
RAM   : 100308 / 327680 bytes = 30.6%
Flash : 641569 / 6553600 bytes = 9.8%
Result: SUCCESS
```

## What EEZ Studio does here

EEZ Studio is the UI authoring/generation tool.

The upstream project contains:

```text
Sunton-ESP32-8048S043.eez-project
```

The project is configured around:

```text
Project type   : LVGL
LVGL target    : 9.0
Display width  : 800
Display height : 480
Color format   : BGR
Flow support   : enabled
```

The generated UI uses EEZ Flow. That is why the firmware also needs `eez-framework` at build time.

Generated UI files include concepts such as:

```text
ui_init()
ui_tick()
eez_flow_init(...)
eez_flow_tick()
flowPropagateValue(...)
```

The external demo contains two generated screens and changes screens from LVGL release events.

## What eez-framework does here

`eez-framework` is the runtime library used by EEZ Flow generated code.

Upstream repository:

```text
https://github.com/eez-open/eez-framework
```

Its `library.json` identifies it as:

```text
name    : eez-framework
version : 0.0.1
```

and declares an LVGL dependency.

For reproducibility we did not use the moving current `master` head. We pinned a commit from the same date as the original board project work:

```text
7c83e763a2e5350136777d9ba8f08b6af66e8b6a
2024-05-23
```

This reduces the chance of modern EEZ API changes breaking a 2024-generated project.

## Required software

Windows setup used for this experiment:

```text
Git
PowerShell
PlatformIO Core / PlatformIO IDE
ESP32-S3 USB serial connection
```

Optional but useful:

```text
Visual Studio Code
PlatformIO IDE extension
C/C++ extension
Serial Monitor extension
EEZ Studio
```

EEZ Studio is only required when editing/regenerating the UI. It is not required merely to compile already-generated `src/ui` files.

## PlatformIO command on Windows

If `pio` is already in PATH:

```powershell
pio --version
```

If PowerShell reports that `pio` is not recognized, PlatformIO installed by the VS Code extension is commonly available here:

```powershell
$Pio = "$HOME\.platformio\penv\Scripts\platformio.exe"
Test-Path $Pio
& $Pio --version
```

Expected first check:

```text
True
```

The rest of this guide uses:

```powershell
& $Pio ...
```

You can also temporarily add PlatformIO to PATH:

```powershell
$env:Path += ";$HOME\.platformio\penv\Scripts"
pio --version
```

## Clone the external firmware

Keep external code outside the ESP32-8048S043-lab repository.

Recommended location:

```text
C:\Users\<USER>\Documents\GitHub\third-party\
```

Commands:

```powershell
New-Item -ItemType Directory -Path "$HOME\Documents\GitHub\third-party" -Force | Out-Null
cd "$HOME\Documents\GitHub\third-party"

git clone https://github.com/clumsyCoder00/Sunton-ESP32-8048S043.git clumsyCoder00-Sunton-ESP32-8048S043

cd "$HOME\Documents\GitHub\third-party\clumsyCoder00-Sunton-ESP32-8048S043\Sunton-ESP32-8048S043"
```

The nested folder containing `platformio.ini` is the actual PlatformIO project directory.

## Dependencies

The upstream README names these application dependencies:

```text
Arduino GFX       1.4.7
eez-framework     0.0.1
GT911             1.0.2
LVGL              9.1.0
```

For our reproduced build we let PlatformIO resolve them from `lib_deps` instead of manually copying libraries into `lib/`.

The exact dependency set used:

```ini
lib_deps =
    lvgl/lvgl@9.1.0
    moononournation/GFX Library for Arduino@1.4.7
    tamctec/TAMC_GT911@1.0.2
    https://github.com/eez-open/eez-framework.git#7c83e763a2e5350136777d9ba8f08b6af66e8b6a
```

## Why the PlatformIO platform is pinned

The unmodified project was first built in 2026 using the current PlatformIO Espressif32 environment. PlatformIO selected a much newer Arduino-ESP32 stack than the 2024 project was written against.

To reproduce the original-era application more predictably, the successful experiment pinned:

```ini
platform = espressif32@6.7.0
```

Do not interpret this as the required platform for our own BSP. It is a reproduction choice for this external firmware.

## Reproducible platformio.ini

Back up the upstream file first:

```powershell
Copy-Item .\platformio.ini .\platformio.ini.upstream -Force
```

Use the following local experiment configuration:

```ini
[env:ESP32S3-8048S043]
platform = espressif32@6.7.0
board = esp32-s3-devkitc-1
framework = arduino

monitor_speed = 115200

board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.arduino.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_opi
board_upload.flash_size = 16MB

build_flags =
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
    -I lib

lib_deps =
    lvgl/lvgl@9.1.0
    moononournation/GFX Library for Arduino@1.4.7
    tamctec/TAMC_GT911@1.0.2
    https://github.com/eez-open/eez-framework.git#7c83e763a2e5350136777d9ba8f08b6af66e8b6a
```

Important board-profile assumptions in this reproduction:

```text
ESP32-S3
CPU 240 MHz
flash clock 80 MHz
16 MB flash
default_16MB.csv partition table
QIO flash / OPI PSRAM profile
BOARD_HAS_PSRAM
```

The base PlatformIO board is still `esp32-s3-devkitc-1`; the 16 MB / PSRAM properties are overridden by the project configuration.

## Windows UTF-8 BOM warning

A real failure encountered during this experiment was caused by PowerShell writing `platformio.ini` with a UTF-8 BOM.

PlatformIO reported:

```text
InvalidProjectConfError
File contains no section headers
'\ufeff[env:ESP32S3-8048S043]'
```

When creating `platformio.ini` from Windows PowerShell, write UTF-8 without BOM.

Safe PowerShell pattern:

```powershell
$Content = @'
[env:ESP32S3-8048S043]
platform = espressif32@6.7.0
board = esp32-s3-devkitc-1
framework = arduino

monitor_speed = 115200

board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.arduino.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_opi
board_upload.flash_size = 16MB

build_flags =
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
    -I lib

lib_deps =
    lvgl/lvgl@9.1.0
    moononournation/GFX Library for Arduino@1.4.7
    tamctec/TAMC_GT911@1.0.2
    https://github.com/eez-open/eez-framework.git#7c83e763a2e5350136777d9ba8f08b6af66e8b6a
'@

[System.IO.File]::WriteAllText(
    (Join-Path (Get-Location) 'platformio.ini'),
    $Content,
    (New-Object System.Text.UTF8Encoding($false))
)
```

Verify the beginning of the file:

```powershell
Format-Hex .\platformio.ini | Select-Object -First 2
```

Correct first bytes:

```text
5B 65 6E 76
```

which represent:

```text
[env
```

Incorrect BOM prefix:

```text
EF BB BF
```

## Clean and build

Remove previous build output after changing platform/dependencies:

```powershell
Remove-Item .\.pio -Recurse -Force -ErrorAction SilentlyContinue
```

Build:

```powershell
& $Pio run -e ESP32S3-8048S043
```

The first run can take several minutes because PlatformIO downloads the Espressif platform, framework, toolchain and libraries.

Successful reproduced build ended with:

```text
RAM:   [===       ]  30.6% (used 100308 bytes from 327680 bytes)
Flash: [=         ]   9.8% (used 641569 bytes from 6553600 bytes)
Building .pio\build\ESP32S3-8048S043\firmware.bin
Successfully created esp32s3 image.
[SUCCESS]
```

Generated firmware:

```text
.pio\build\ESP32S3-8048S043\firmware.bin
```

## Find the serial port

List ports:

```powershell
& $Pio device list
```

The physical experiment used COM12, but every system can assign a different COM number.

## Upload

Example for COM12:

```powershell
& $Pio run -e ESP32S3-8048S043 -t upload --upload-port COM12
```

Do not erase the whole flash unless there is a specific reason. A normal application upload is enough for this experiment.

## Serial monitor

Start at 115200 baud:

```powershell
& $Pio device monitor --port COM12 --baud 115200
```

or allow PlatformIO to choose the port:

```powershell
& $Pio device monitor --baud 115200
```

Exit monitor:

```text
Ctrl+C
```

Expected boot/application lines:

```text
ESP-ROM:esp32s3-20210327
...
Arduino_GFX LVGL_Arduino example v9
Hello Arduino! V9.1.0
Init Display
TFT_BL
...
Setup done
```

Expected LVGL input messages during touch include press/release events with coordinates.

## GT911 settings in the external firmware

The upstream touch configuration uses:

```text
SCL       20
SDA       19
INT       0
RST       38
raw X     480 -> 0
raw Y     272 -> 0
rotation  normal
```

Our own isolated BSP testing independently confirmed the useful hardware facts:

```text
SDA 19
SCL 20
RST 38
GT911 raw space approximately 480x272
```

The external firmware worked physically as-is in this experiment, including its GT911 configuration.

For our own library we continue to use the independently validated BSP touch implementation rather than copying the upstream touch code.

## EEZ Studio workflow

To edit the UI rather than merely compile it:

1. Install EEZ Studio.
2. Open:

```text
Sunton-ESP32-8048S043.eez-project
```

3. Keep the project configured for:

```text
LVGL 9.0
800 x 480
BGR
Flow support enabled
```

4. Generate/update the UI sources.
5. Keep the generated code under the external firmware project while testing.
6. Build again with the pinned PlatformIO environment above.

The upstream project notes two compatibility defines required in generated `ui.h` for its EEZ Flow/LVGL setup:

```c
#ifndef EEZ_FOR_LVGL
#define EEZ_FOR_LVGL
#endif

#ifndef LV_LVGL_H_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE
#endif
```

When regenerating the UI with a newer EEZ Studio version, verify that these compatibility requirements are still needed before applying them blindly.

## lv_conf.h

The external project includes a project-specific LVGL 9 configuration in:

```text
lib/lv_conf.h
```

Important settings observed in that file include:

```text
LV_COLOR_DEPTH 16
LV_MEM_SIZE 64 KB
LV_DEF_REFR_PERIOD 33 ms
LV_USE_OS LV_OS_NONE
```

For this reproduction the PlatformIO build adds:

```text
-DLV_CONF_INCLUDE_SIMPLE
-I lib
```

so LVGL can find the supplied project configuration.

Do not overwrite the LVGL configuration used by our Arduino LVGL 8 examples with this LVGL 9 file.

## Why this experiment matters to ESP32-8048S043-lab

This external firmware demonstrated an important alternative rendering path on the same hardware family:

```text
EEZ-generated LVGL 9 UI
        -> full-screen LVGL framebuffer
        -> Arduino_GFX
        -> full 800x480 RGB565 frame transfer
```

The tested application defines a full-frame/direct rendering strategy and transfers the full framebuffer repeatedly from its main loop.

This is materially different from our earlier native `esp_lcd` partial-update path.

Our local evidence before this external test showed:

```text
15_LVGL_EspLcdBasicUI
    functional touch/UI
    dynamic redraw visually unacceptable

16_LVGL_EspLcdMinimalInvalidation
    stable idle screen
    much better normal touch behavior
    hard-tap jitter still open
```

Because the external EEZ/LVGL9/Arduino_GFX firmware behaved well, we created an independent A/B comparison example in this repository:

```text
15B_LVGL_ArduinoGFXFullFrameUI
```

That example keeps our own BSP and our own LVGL UI while testing the full-frame Arduino_GFX redraw mechanism separately from the GPL application.

## License boundary

External application:

```text
clumsyCoder00/Sunton-ESP32-8048S043
GPL-2.0
```

EEZ framework:

```text
eez-open/eez-framework
MIT
```

Project policy for this experiment:

```text
External source remains outside ESP32-8048S043-lab.
Do not copy GPL application code or generated application UI into this repository.
Document observed mechanisms and reproduce architecture independently.
Keep our BSP/tests under our own implementation.
```

## Quick reproduction checklist

```text
[ ] Install PlatformIO
[ ] Clone clumsyCoder00/Sunton-ESP32-8048S043 outside this repository
[ ] Enter the nested Sunton-ESP32-8048S043 PlatformIO directory
[ ] Back up upstream platformio.ini
[ ] Pin espressif32@6.7.0
[ ] Add exact lib_deps
[ ] Pin eez-framework commit 7c83e763...
[ ] Ensure platformio.ini is UTF-8 without BOM
[ ] Remove .pio
[ ] Build
[ ] Confirm firmware.bin is created
[ ] List COM ports
[ ] Upload
[ ] Open 115200 Serial monitor
[ ] Confirm LVGL 9.1.0 boot
[ ] Confirm display
[ ] Confirm GT911 touch
[ ] Confirm EEZ screen navigation
[ ] Compare visual redraw behavior with local tests 15, 16 and 15B
```

## Related local documents

```text
docs/third-party/clumsycoder00-sunton-esp32-8048S043.md
docs/lvgl-third-party-reference.md
libraries/ESP32_8048S043/examples/15_LVGL_EspLcdBasicUI/
libraries/ESP32_8048S043/examples/16_LVGL_EspLcdMinimalInvalidation/
libraries/ESP32_8048S043/examples/15B_LVGL_ArduinoGFXFullFrameUI/
```
