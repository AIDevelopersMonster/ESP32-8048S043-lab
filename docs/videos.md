# Video plan and evidence registry

## Published videos

| # | Video | Link | Purpose | Evidence status |
|---:|---|---|---|---|
| 01 | ESP32-8048S043 Lab — repository structure and first board overview | https://youtube.com/shorts/jd5o4FLpYEA | Introduce the board family, the new lab repository structure, planned checks, third-party firmware analysis and continuation of the evidence-first workflow from the WT32-SC01-PLUS-Lab project. | OVERVIEW ONLY / NO PHYSICAL PASS CLAIM |
| 02 | ESP32-8048S043 — visual display and touchscreen check on LVGL 8 | https://youtube.com/shorts/XVaWqrtXHE4 | Show the factory LVGL 8 Widgets Demo running on the physical 800x480 display and visually checking touchscreen interaction. | SAMPLE A FACTORY LVGL DISPLAY + TOUCHSCREEN VISUAL PASS |
| 03 | ESP32-8048S043 — first test from our Arduino library: BoardInfo | https://youtube.com/shorts/wELRdRWqlnw | Show `01_BoardInfo` from the `ESP32_8048S043` Arduino library, using the current Sample A Arduino IDE profile and confirming ESP32-S3, 16 MB flash, 8 MB PSRAM and stable serial ALIVE output. | SAMPLE A ARDUINO BOARDINFO PASS |
| 04 | ESP32-8048S043 — second test from our Arduino library: RGB display | https://youtube.com/shorts/sKGejpLF3ZA | Show `02_DisplayRGBTest` from the `ESP32_8048S043` Arduino library, validating our own Arduino_GFX RGB display path with full-screen colors, orientation frame, RGB color bars and stripe/data-line pattern. | SAMPLE A OWN MINIMAL RGB DISPLAY VISUAL PASS |
| 05 | ESP32-8048S043 — third test from our Arduino library: GT911 touchscreen | https://youtube.com/shorts/_zhtl-AWcCE | Show `03_TouchGT911Test` from the `ESP32_8048S043` Arduino library, validating the GT911 touch controller at I2C address 0x5D, Product ID `911`, raw coordinate polling and visible touch marker movement on the 800x480 screen. | SAMPLE A OWN GT911 TOUCH VISUAL PASS |

## Planned shooting sequence

| Video | Goal | Evidence status |
|---|---|---|
| 01 Board overview | show physical board, markings, connectors, repository structure and project goals | PUBLISHED / OVERVIEW ONLY |
| 02 Board passport | esptool/chip/flash/PSRAM identity | DONE / BOARDINFO PASS |
| 03 Factory firmware dump | read full flash twice, compare SHA-256 and preserve evidence | DONE / HASH MATCH |
| 04 Factory firmware analysis | scan partitions, strings, possible factory tests and entry points | FIRST-PASS DONE |
| 05 RGB display first light | 800x480 color/timing/backlight | DONE / OWN MINIMAL RGB DISPLAY PASS |
| 06 Touchscreen visual check | factory LVGL Widgets Demo responds to touch | PUBLISHED / FACTORY DEMO VISUAL PASS |
| 07 Dedicated touch scan | identify controller address and coordinate behavior | DONE / OWN GT911 TOUCH VISUAL PASS |
| 08 LVGL basic UI | button, slider, touch using lab firmware | OPEN |
| 09 Web setup | AP/browser setup path | OPEN |
| 10 Web Flasher | browser firmware install | OPEN |
| 11 Widget Runtime | JSON UI without reflashing | OPEN |
| 12 GitHub OTA | manifest/download/SHA/reboot/up-to-date | OPEN |

## Rule

A video can support PHYSICAL PASS only when it clearly identifies the specimen and the firmware/example being demonstrated. Overview videos may document intent, repository structure and project direction, but they do not create hardware PASS status by themselves.

Visual touch evidence can support a touchscreen runtime PASS, but it does not identify the controller model or replace a dedicated I2C scan and coordinate-target test.
