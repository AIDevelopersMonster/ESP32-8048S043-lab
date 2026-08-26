# Video plan and evidence registry

## Published videos

| # | Video | Link | Purpose | Evidence status |
|---:|---|---|---|---|
| 01 | ESP32-8048S043 Lab — repository structure and first board overview | https://youtube.com/shorts/jd5o4FLpYEA | Introduce the board family, the new lab repository structure, planned checks, third-party firmware analysis and continuation of the evidence-first workflow from the WT32-SC01-PLUS-Lab project. | OVERVIEW ONLY / NO PHYSICAL PASS CLAIM |
| 02 | ESP32-8048S043 — visual display and touchscreen check on LVGL 8 | https://youtube.com/shorts/XVaWqrtXHE4 | Show the factory LVGL 8 Widgets Demo running on the physical 800x480 display and visually checking touchscreen interaction. | SAMPLE A FACTORY LVGL DISPLAY + TOUCHSCREEN VISUAL PASS |
| 03 | ESP32-8048S043 — first test from our Arduino library: BoardInfo | https://youtube.com/shorts/wELRdRWqlnw | Show `01_BoardInfo` from the `ESP32_8048S043` Arduino library, using the current Sample A Arduino IDE profile and confirming ESP32-S3, 16 MB flash, 8 MB PSRAM and stable serial ALIVE output. | SAMPLE A ARDUINO BOARDINFO PASS |
| 04 | ESP32-8048S043 — second test from our Arduino library: RGB display | https://youtube.com/shorts/sKGejpLF3ZA | Show `02_DisplayRGBTest` from the `ESP32_8048S043` Arduino library, validating our own Arduino_GFX RGB display path with full-screen colors, orientation frame, RGB color bars and stripe/data-line pattern. | SAMPLE A OWN MINIMAL RGB DISPLAY VISUAL PASS |
| 05 | ESP32-8048S043 — third test from our Arduino library: GT911 touchscreen | https://youtube.com/shorts/_zhtl-AWcCE | Show `03_TouchGT911Test` from the `ESP32_8048S043` Arduino library, validating the GT911 touch controller at I2C address 0x5D, Product ID `911`, raw coordinate polling and visible touch marker movement on the 800x480 screen. | SAMPLE A OWN GT911 TOUCH VISUAL PASS |
| 06 | ESP32-8048S043 — Wi-Fi scan from our Arduino library | https://youtube.com/shorts/DOus0uNBBZI | Show `06_WiFiTest` in scan-only mode under the local Arduino board profile, confirming STA MAC readout and active Wi-Fi scan with nearby networks found. | SAMPLE A WIFI RADIO SCAN PASS CANDIDATE / INFRASTRUCTURE PENDING |
| 07 | ESP32-8048S043 — read-only microSD / TF test from our Arduino library | https://youtube.com/shorts/vACvK85U0Lw | Show `08_SDCardTest` mounting a microSD card on the source-backed SPI pins CS=10, MOSI=11, CLK=12, MISO=13, reading SDHC/SDXC metadata and listing the root directory without write/format/delete operations. | SAMPLE A READ-ONLY SD PHYSICAL PASS CANDIDATE |

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
| 08 Wi-Fi radio scan | run `06_WiFiTest` scan-only mode, read STA MAC and list nearby networks | PUBLISHED / WIFI RADIO SCAN PASS CANDIDATE |
| 09 Wi-Fi infrastructure | association, DHCP, DNS, TCP/HTTP and reconnect using local `wifi_secrets.h` | DONE / FULL WIFI PASS CANDIDATE |
| 10 HTTP WebServer | browser page, `/status.json` and `/ping` from the board | DONE / WEB SERVER PASS CANDIDATE |
| 11 SD card read-only | mount microSD through SPI, read card metadata and list directories without writing | PUBLISHED / READ-ONLY SD PASS CANDIDATE |
| 12 LVGL basic UI | button, slider, touch using lab firmware | OPEN |
| 13 Web setup | AP/browser setup path | OPEN |
| 14 Web Flasher | browser firmware install | OPEN |
| 15 Widget Runtime | JSON UI without reflashing | OPEN |
| 16 GitHub OTA | manifest/download/SHA/reboot/up-to-date | OPEN |

## Rule

A video can support PHYSICAL PASS only when it clearly identifies the specimen and the firmware/example being demonstrated. Overview videos may document intent, repository structure and project direction, but they do not create hardware PASS status by themselves.

Visual touch evidence can support a touchscreen runtime PASS, but it does not identify the controller model or replace a dedicated I2C scan and coordinate-target test.

Wi-Fi scan evidence supports only radio/scan status. Full Wi-Fi PASS requires association, DHCP, DNS, TCP/HTTP and reconnect validation with local credentials that are not committed to the repository.

SD card read-only evidence supports mount, metadata and directory listing only. It does not prove write safety, formatting, long-duration SD stress or SD-backed Web/Widget Runtime storage.
