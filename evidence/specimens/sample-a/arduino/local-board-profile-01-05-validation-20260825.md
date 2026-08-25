# Sample A / local Arduino board profile / 01-05 validation

Status: `LOCAL BOARD PROFILE 01-05 VALIDATION / SAMPLE A`

Date: `2026-08-25`

Board target:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Local platform path:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

## Important implementation note

The local board profile required `platform.local.txt` so ESP32-S3 target macros were visible to third-party library compilation units, especially Arduino_GFX_Library.

Observed failure before this fix:

```text
Arduino_ESP32RGBPanel does not name a type
undefined reference to Arduino_ESP32RGBPanel::Arduino_ESP32RGBPanel(...)
undefined reference to Arduino_RGB_Display::begin(long)
```

The working local override defines ESP32-S3 target macros through `compiler.cpp.extra_flags`, `compiler.c.extra_flags` and `compiler.S.extra_flags`.

## 01_BoardInfo result

Separate evidence file:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
```

Key result:

```text
Chip                    : ESP32-S3 rev 2
Flash chip size         : 16777216 bytes / 16384 KB / 16 MB
PSRAM size              : 8388608 bytes / 8192 KB / 8 MB
Running app             : label=app0 address=0x010000 size=3145728
Boot app                : label=app0 address=0x010000 size=3145728
ALIVE                   : observed
```

Status:

```text
01_BoardInfo under local board profile: PASS
```

## 02_DisplayRGBTest result

Serial evidence:

```text
ESP32-8048S043 Lab / 02_DisplayRGBTest
Minimal RGB panel + backlight validation

[PIN MAP]
LCD 800x480
DE=40 VSYNC=41 HSYNC=39 PCLK=42 BL=2
R0..R4=45,48,47,21,14
G0..G5=5,6,7,15,16,4
B0..B4=8,3,46,9,1
Display begin: OK
Test sequence started.
Screen: RED
Screen: GREEN
Screen: BLUE
Screen: WHITE
Screen: BLACK
Screen: orientation frame
Screen: RGB color bars
Screen: stripe/data-line pattern
02_DisplayRGBTest cycle complete. If all screens are correct: DISPLAY RGB TEST VISUAL PASS candidate.
```

Operator result:

```text
02_DisplayRGBTest under local board profile: PASS / visual check reported OK
```

## 03_TouchGT911Test result

Serial evidence:

```text
ESP32-8048S043 Lab / 03_TouchGT911Test
Known-good-style Arduino_GFX + GT911 polling test

gfx->begin() start
gfx->begin(): OK
Wire.begin(SDA=19, SCL=20, speed=400000)
GT911 reset: RST38 toggle, INT18 passive pull-up, polling mode
GT911 at 0x5D, product id raw: 39 31 31 00
GT911 product id text: 911.
I2C scan: 0x5D
Active GT911 address: 0x5D
GT911 FW version: 0x1060 (4192)
GT911 resolution registers: X=480 Y=272
```

Non-fatal observed status:

```text
Touch status unusual: 0x80 points=0
```

Boundary:

```text
The controller identity, address, product ID and firmware register are confirmed. Touch movement was validated further in 05_TestConsole.
```

Status:

```text
03_TouchGT911Test under local board profile: PASS / GT911 detected / display path OK
```

## 04_BacklightTest result

Serial evidence:

```text
Backlight stage: BLINK 1 OFF        duty=  0 note=blink check
Backlight stage: BLINK 1 ON         duty=255 note=blink check
Backlight stage: BLINK 2 OFF        duty=  0 note=blink check
Backlight stage: BLINK 2 ON         duty=255 note=blink check
Backlight stage: BLINK 3 OFF        duty=  0 note=blink check
Backlight stage: BLINK 3 ON         duty=255 note=blink check
Backlight stage: PWM STEP 1         duty=  0 note=look for brightness change
Backlight stage: PWM STEP 2         duty= 16 note=look for brightness change
Backlight stage: PWM STEP 3         duty= 32 note=look for brightness change
```

Operator result:

```text
04_BacklightTest under local board profile: PASS reported by operator
```

Boundary:

```text
GPIO2 backlight ON/OFF/blink is confirmed by operator observation. PWM duty stepping is PASS candidate unless separately characterized with measured brightness or detailed video.
```

## 05_TestConsole result

Serial evidence:

```text
ESP32-8048S043 Lab / 05_TestConsole
Combined RGB + GT911 + backlight diagnostic console
Build  : Arduino IDE preprocessor-safe version, no struct function signatures

gfx->begin() start
gfx->begin(): OK
Wire.begin(SDA=19, SCL=20, speed=400000)
GT911 reset: RST38 toggle, INT18 passive pull-up, polling mode
GT911 at 0x5D, product id raw: 39 31 31 00
GT911 product id text: 911.
I2C scan: 0x5D
Active GT911 address: 0x5D
GT911 FW version: 0x1060 (4192)
GT911 resolution registers: X=480 Y=272

05_TestConsole REPORT
Chip              : ESP32-S3 rev 2, 240 MHz
Flash             : 16777216 bytes
PSRAM             : 0 bytes
Free heap         : 338752 bytes
Backlight         : GPIO2 ON
GT911             : DETECTED at 0x5D
Touch events      : 0
Last action       : GT911 ready at 0x5D

Touch #1: status=0x81 points=1 track=0 raw=(133,108) screen=(220,207) size=67
Touch #2: status=0x81 points=1 track=0 raw=(354,104) screen=(587,205) size=76
Touch #3: status=0x81 points=1 track=0 raw=(200,183) screen=(330,343) size=54
Touch #4: status=0x81 points=1 track=0 raw=(48,198) screen=(77,367) size=52
Touch #5: status=0x81 points=1 track=0 raw=(106,256) screen=(172,472) size=42
Touch #6: status=0x81 points=1 track=0 raw=(236,243) screen=(388,452) size=53
Touch #7: status=0x81 points=1 track=0 raw=(344,221) screen=(568,415) size=55

05_TestConsole REPORT
Chip              : ESP32-S3 rev 2, 240 MHz
Flash             : 16777216 bytes
PSRAM             : 0 bytes
Free heap         : 338752 bytes
Backlight         : GPIO2 ON
GT911             : DETECTED at 0x5D
Touch events      : 7
Last touch        : status=0x81 points=1 track=0 raw=(344,221) screen=(568,415) size=55
Last action       : serial report printed

Touch #8: status=0x81 points=1 track=0 raw=(304,158) screen=(503,301) size=74
Touch #9: status=0x81 points=1 track=0 raw=(386,193) screen=(638,365) size=59
Touch #10: status=0x81 points=1 track=0 raw=(281,239) screen=(463,446) size=60
Touch #11: status=0x81 points=1 track=0 raw=(199,206) screen=(328,385) size=46
Touch #12: status=0x81 points=1 track=0 raw=(157,168) screen=(259,315) size=54
Touch #13: status=0x81 points=1 track=0 raw=(172,149) screen=(284,282) size=57
Touch #14: status=0x81 points=1 track=0 raw=(263,143) screen=(435,273) size=60
Touch #15: status=0x81 points=1 track=0 raw=(335,186) screen=(554,352) size=55
```

Result:

```text
05_TestConsole under local board profile: PASS for combined RGB display + GT911 touch + backlight diagnostic console
```

Important PSRAM note:

```text
This 05_TestConsole run reported PSRAM as 0 bytes, while 01_BoardInfo under the same local board-profile validation path reported 8388608 bytes / 8 MB. Until rechecked, treat 01_BoardInfo as the PSRAM acceptance test and treat 05_TestConsole as the integration test for RGB + GT911 + backlight.
```

## Overall result

```text
Local board target visible in Arduino IDE       : PASS
01_BoardInfo                                    : PASS
02_DisplayRGBTest                               : PASS / serial + operator visual
03_TouchGT911Test                               : PASS / GT911 detected / display OK
04_BacklightTest                                : PASS / operator report
05_TestConsole                                  : PASS / RGB + GT911 + backlight integration
Arduino_GFX under custom board profile          : PASS after platform.local.txt target macro fix
Board Manager package                           : OPEN / not created
PSRAM in 05_TestConsole                         : RECHECK NEEDED; 01_BoardInfo remains PSRAM authority
```
