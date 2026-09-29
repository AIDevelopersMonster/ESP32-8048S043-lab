# Application and platform stages

This directory contains the incremental KONTAKTS / ESP32-8048S043 development line.

The numbers are laboratory milestones, not a promise that every integer has a permanent top-level directory. Some intermediate experiments were folded into the platform core once their useful functionality became canonical.

## Current chain

```text
hardware/BSP evidence
 -> local HMI
 -> storage/network
 -> OTA/recovery
 -> SD applications
 -> serial tools
 -> field Modbus/RS485
 -> Web control
 -> BLE transport
 -> Android client
```

| Directory | Role | Current status |
|---|---|---|
| `01_SixCardSerialDeck` | early touch/serial application | PHYSICAL PASS |
| `02_MixedWidgets` | LVGL mixed-controls lab | PHYSICAL PASS / CLOSED |
| `03_LiveDashboard` | system telemetry dashboard | PHYSICAL PASS / CLOSED |
| `04_StorageConfig` | persistent storage/config stage | historical development |
| `05_NetworkProvisioning` | Wi-Fi provisioning | PHYSICAL PASS / legacy standalone lab |
| `06_OTARecovery` | canonical platform core, OTA, services | Platform 0.3.9 foundation |
| `09_SDWidgetLibrary` | SD apps + Help | PHYSICAL PASS / current content channel |
| `10_BLESliderSync` | earlier BLE experiment | historical lab |
| `11_USBSerialTerminal` | serial UI/application stage | PHYSICAL PASS lineage |
| `14_ModbusController` | generic field-bus/service record | MA01 path proven in later stages |
| `15_WebControl` | browser transport | PHYSICAL MVP PASS |
| `16_BLEControl` | NimBLE command transport | PHYSICAL PASS / Platform 0.3.9 |
| `17_MobileControl` | native Android BLE client | PHYSICAL MVP PASS |

## Canonical platform architecture

```text
HMI -----------+
UART0/P1 ------+
Web -----------+
BLE -----------+--> command/service layer --> providers --> hardware
Android -------+
```

The application/client layer must not duplicate hardware register maps. Device-specific Modbus knowledge belongs in providers/services.

## Current physical endpoint

The present end-to-end field-control reference is:

```text
ESP32-S3
 -> UART1 GPIO17/18
 -> automatic-direction RS485
 -> Ebyte MA01-XXCX0080
 -> relay outputs
```

The same provider is controlled by HMI, service UART, Web, BLE and Android.

## Where to start

- Platform: `06_OTARecovery/README.md`
- SD applications: `09_SDWidgetLibrary/README.md`
- Field bus: `14_ModbusController/README.md`
- Web: `15_WebControl/README.md`
- BLE: `16_BLEControl/README.md`
- Android: `17_MobileControl/README.md`
