# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON / GROWING
01_BoardInfo            PHYSICAL PASS / SAMPLE A
02_DisplayRGBTest       PHYSICAL VISUAL PASS / SAMPLE A
03_TouchGT911Test       PHYSICAL VISUAL PASS / SAMPLE A
04_BacklightTest        SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
Display driver          OWN MINIMAL ARDUINO_GFX TEST PASS
Touch driver            GT911 POLLING VISUAL TEST PASS
Backlight driver        DIGITAL/PWM TEST ADDED / PHYSICAL VALIDATION OPEN
LVGL port               OPEN
Physical PASS claims    SAMPLE A BOARDINFO + OWN RGB DISPLAY + OWN GT911 TOUCH + FACTORY LVGL DISPLAY/TOUCH VISUAL
```

## Arduino IDE board setup

Recommended working profile for the examples in this library on Sample A:

```text
Board package : esp32 by Espressif Systems
Board         : ESP32S3 Dev Module
Port          : CH340 / USB-SERIAL port of the board
Upload Speed  : 921600 if stable; 460800 fallback
CPU Frequency : 240MHz (WiFi)
Flash Size    : 16MB / 128Mb
Flash Mode    : QIO 80MHz
Partition     : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM         : OPI PSRAM
USB CDC Boot  : Disabled when using CH340C USB-UART
Upload Mode   : UART0 / Hardware CDC, depending on Arduino menu wording
Core Debug    : None
Serial Monitor: 115200 baud
```

Menu names differ between ESP32 Arduino core versions. Keep the intent: ESP32-S3 target, 16 MB flash, 8 MB/OPI PSRAM enabled, external UART upload through CH340C, serial monitor at 115200.

## Example plan

```text
01_BoardInfo            first Arduino IDE smoke test, chip/flash/PSRAM/ALIVE
02_DisplayRGBTest       minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test       GT911 polling visual marker + serial diagnostics
04_BacklightTest        dedicated backlight GPIO2 ON/OFF/blink/PWM test
05_TestConsole          future combined diagnostic console
09_LVGL_BasicUI        future
10_LVGL_Dashboard      future
13_RetroClock_800x480  future
14_WidgetLoader        future
15_GitHubOTA           future
```

## 01_BoardInfo

Purpose:

```text
verify basic Arduino IDE upload, serial monitor, ESP32-S3 identity, 16 MB flash and 8 MB PSRAM
```

Open:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/01_BoardInfo.ino
```

See also:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/README.md
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
```

PASS boundary:

```text
PASS requires successful upload, serial output at 115200, ESP32-S3 identity, about 16 MB flash, about 8 MB PSRAM and stable ALIVE messages.
```

Current Sample A status:

```text
PHYSICAL PASS
```

## 02_DisplayRGBTest

Purpose:

```text
validate the source-backed ESP32-8048S043 RGB GPIO map with our own minimal Arduino sketch
```

What it tests:

- RGB panel bring-up through `Arduino_GFX_Library`;
- backlight GPIO 2 full ON;
- full-screen red/green/blue/white/black;
- orientation frame with corner markers;
- RGB color-bar pattern;
- stripe pattern for data-line sanity.

Dependency:

```text
Arduino_GFX_Library by moononournation
```

See also:

```text
libraries/ESP32_8048S043/examples/02_DisplayRGBTest/README.md
evidence/specimens/sample-a/arduino/02-display-rgbtest-20260823.md
```

PASS boundary:

```text
PASS requires physical photo/video evidence from a named specimen showing correct colors, orientation, color bars, stripe pattern and stable serial sequence.
```

Current Sample A status:

```text
PHYSICAL VISUAL PASS
```

## 03_TouchGT911Test

Purpose:

```text
validate the GT911 capacitive touch path with our own visual Arduino sketch using a known-good-style polling pattern
```

What it tests:

- display initializes through `Arduino_GFX_Library`;
- static 800x480 test screen is drawn;
- I2C starts on SDA=19 / SCL=20;
- I2C scan finds connected devices;
- GT911 candidate address is detected at 0x5D or 0x14;
- Product ID register at 0x8140 is readable;
- firmware/resolution registers are read where available;
- touch status register 0x814E is readable;
- touch point data starts at 0x814F;
- touch points print to Serial Monitor;
- touch points draw as a visible red marker on the 800x480 display.

Dependencies:

```text
Arduino_GFX_Library by moononournation
Arduino Wire for I2C
```

Open:

```text
libraries/ESP32_8048S043/examples/03_TouchGT911Test/03_TouchGT911Test.ino
```

See also:

```text
libraries/ESP32_8048S043/examples/03_TouchGT911Test/README.md
evidence/specimens/sample-a/arduino/03-touch-gt911-20260823.md
```

PASS boundary:

```text
PASS requires visual evidence that touching the panel moves the red marker on the display, plus serial evidence that GT911 is found at 0x5D or 0x14 and raw/screen x/y coordinates change.
```

Current Sample A status:

```text
PHYSICAL VISUAL PASS
```

## 04_BacklightTest

Purpose:

```text
validate the ESP32-8048S043 backlight control path separately from RGB display and touch
```

What it tests:

- display initializes through `Arduino_GFX_Library`;
- static 800x480 reference screen is drawn;
- source-backed backlight pin GPIO2 is used;
- GPIO2 HIGH / LOW behavior is tested;
- visible blink sequence is tested;
- PWM / `analogWrite()` duty steps are attempted;
- serial output records every backlight stage.

Dependencies:

```text
Arduino_GFX_Library by moononournation
Arduino analogWrite / LEDC backend
```

Open:

```text
libraries/ESP32_8048S043/examples/04_BacklightTest/04_BacklightTest.ino
```

See also:

```text
libraries/ESP32_8048S043/examples/04_BacklightTest/README.md
```

PASS boundary:

```text
PASS requires physical evidence that GPIO2 HIGH turns the backlight on, GPIO2 LOW turns it off, blink is visible, and no brownout/crash occurs. PWM dimming is a separate stronger PASS only if intermediate duty steps visibly change brightness.
```

Current Sample A status:

```text
SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
