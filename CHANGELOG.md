# Changelog

## 2026-09-30 — Platform 0.4.0 / Climate Controller milestone

- Promote KONTAKTS Platform 0.4.0 as the current accepted platform.
- Merge App18 Climate Controller into `main`.
- Physically validate temperature/humidity control with EID041 + MA01 over shared Modbus RTU.
- Validate DO1 HEATER, DO2 COOLER, DO3 HUMIDIFIER and DO4 DEHUMIDIFIER paths, hysteresis and mutual interlocks.
- Validate 60-second stale-sensor grace, FAILSAFE all-off attempt and automatic recovery.
- Add responsive local `/climate` Web control through the common command/service layer.
- Verify AUTO ownership: AUTO ON reserves MA01; AUTO OFF releases external relay control.
- Stage the accepted image in inactive OTA slot, verify flash digest, boot PENDING_VERIFY, physically test Web/HMI, then confirm VALID.
- Publish Platform 0.4.0 exact accepted OTA artifact:
  `F5A494AB887BA010C130C5AFB29F1478FE42BB4D8384B5F136A414842DD554B6`.
- Publish Platform 0.4.0 full image for first install/Web Flasher:
  `F2787661FD55A8A6DA1CD8131B644D3C874D89182D1CC5A7F057255A777893B0`.
- Publish `widget-climate-controller.zip` in the current SD release.
- Update Web Flasher catalog and deploy GitHub Pages successfully.
- Start repository productization: user-first README, Quick Start, security boundary, roadmap and branch-consolidation programme.
- Dedicated MA01 communication-loss physical test remains deferred because it requires rewiring the current bench.


## 2026-09-29 — Platform transport milestone

- Promote KONTAKTS Platform 0.3.9 to the current physically accepted integrated platform.
- Validate RGB/LVGL, GT911, SD application library, Wi-Fi STA/AP, Web, technological UART0/P1, GitHub OTA CHECK, BLE and MA01 UART1/RS485 in one platform line.
- Physically regression-test Web Flasher 0.3.7 -> GitHub OTA 0.3.9, then re-check GitHub OTA successfully from 0.3.9.
- Fix release-channel ownership after App17 temporarily displaced the firmware `releases/latest` pointer; Android and mutable SD channels are prereleases so the firmware OTA manifest remains reachable.
- Publish the exact hardware-accepted Platform 0.3.9 full image and SHA-256:
  `20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76`.
- Update Web Flasher to Platform 0.3.9.
- Refresh SD Help for user, application programmer, system programmer and hardware roles.
- Add App15 browser MA01 control and record physical MVP PASS.
- Add App16 NimBLE command transport and record physical PASS.
- Harden memory ownership for Wi-Fi/BLE/display/OTA coexistence: PSRAM LVGL buffer, external NimBLE allocations where supported, external OTA task stack, internal DMA RGB bounce buffer.
- Add App17 native Android BLE client.
- Lower App17 minimum SDK to API 23 and validate it on Android 7.1.2 / API 25.
- Physically validate Android App17 -> BLE -> command/service -> MA01 provider -> UART1/RS485 -> real relay ON/OFF.
- Add App17 video evidence: https://youtube.com/shorts/FxDnALva3xM
- Consolidate current repository documentation and close superseded development PRs.

## Historical development

The repository began as an evidence-first ESP32-8048S043 hardware laboratory and progressively added:

- factory firmware double-read preservation and analysis;
- Sample A board passport and pin-map research;
- Arduino BSP / board-profile experiments;
- RGB display and GT911 physical validation;
- Wi-Fi and network provisioning;
- OTA, rollback and recovery;
- Widget Runtime and SD application packages;
- serial terminal tools;
- field Modbus/RS485 providers;
- Web, BLE and Android control transports.

Detailed stage history remains in per-application READMEs, evidence records and Git history.
