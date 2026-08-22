# Video plan and evidence registry

## Published videos

| # | Video | Link | Purpose | Evidence status |
|---:|---|---|---|---|
| 01 | ESP32-8048S043 Lab — repository structure and first board overview | https://youtube.com/shorts/jd5o4FLpYEA | Introduce the board family, the new lab repository structure, planned checks, third-party firmware analysis and continuation of the evidence-first workflow from the WT32-SC01-PLUS-Lab project. | OVERVIEW ONLY / NO PHYSICAL PASS CLAIM |

## Planned shooting sequence

| Video | Goal | Evidence status |
|---|---|---|
| 01 Board overview | show physical board, markings, connectors, repository structure and project goals | PUBLISHED / OVERVIEW ONLY |
| 02 Board passport | esptool/chip/flash/PSRAM identity | OPEN |
| 03 Factory firmware dump | read full flash twice, compare SHA-256 and preserve evidence | OPEN |
| 04 Factory firmware analysis | scan partitions, strings, possible factory tests and entry points | OPEN |
| 05 RGB display first light | 800x480 color/timing/backlight | OPEN |
| 06 GT911 touch | coordinate targets and serial output | OPEN |
| 07 LVGL basic UI | button, slider, touch | OPEN |
| 08 Web setup | AP/browser setup path | OPEN |
| 09 Web Flasher | browser firmware install | OPEN |
| 10 Widget Runtime | JSON UI without reflashing | OPEN |
| 11 GitHub OTA | manifest/download/SHA/reboot/up-to-date | OPEN |

## Rule

A video can support PHYSICAL PASS only when it clearly identifies the specimen and the firmware/example being demonstrated. Overview videos may document intent, repository structure and project direction, but they do not create hardware PASS status by themselves.
