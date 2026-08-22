# Third-party reference projects and firmware

This folder is for audits of external projects. Do not copy code or firmware blindly.

For each project record:

- URL and license;
- target board variant;
- framework and library versions;
- display/touch driver approach;
- reusable ideas;
- incompatibilities with our specimen;
- whether code can be reused, rewritten or only referenced.

For external firmware images, use the repository firmware policy:

- [`../firmware/README.md`](../firmware/README.md)

Firmware binaries from vendor, factory or community sources must not be committed unless redistribution is explicitly permitted. By default, store only source URL, license status, SHA-256, partition analysis and compatibility notes.

Initial targets to inspect:

- ESP-IDF / LVGL / GT911 projects for ESP32-8048S043;
- Home Assistant / ESPHome configurations;
- Arduino_GFX RGB panel examples;
- LVGL dashboard and HMI examples;
- factory firmware from the physical specimen;
- third-party firmware images that claim ESP32-8048S043 compatibility.
