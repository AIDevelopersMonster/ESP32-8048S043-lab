# Applications and platform components

The `apps/` directory preserves the incremental development line that produced the current KONTAKTS platform. It is not a list of separate current products.

## Current product components

| Directory | Role | Status |
|---|---|---|
| `06_OTARecovery` | canonical platform core, OTA, services | Platform 0.4.0 |
| `09_SDWidgetLibrary` | SD applications + Help | current content channel |
| `17_MobileControl` | native Android BLE client | physical MVP pass |
| `18_ClimateController` | temperature/humidity control service + HMI/Web integration | physical + Web pass |

Current control architecture:

```text
SD HMI --------+
Web -----------+
UART0/P1 ------+
BLE -----------+--> command/service layer --> providers --> hardware
Android -------+
```

## Historical development stages

The remaining numbered directories document useful milestones that were folded into the current platform:

| Directory | Historical role |
|---|---|
| `01_SixCardSerialDeck` | early touch/serial application |
| `02_MixedWidgets` | LVGL mixed-controls lab |
| `03_LiveDashboard` | live system telemetry |
| `04_StorageConfig` | persistent configuration |
| `05_NetworkProvisioning` | standalone Wi-Fi provisioning |
| `10_BLESliderSync` | early BLE synchronization experiment |
| `11_USBSerialTerminal` | serial UI milestone |
| `14_ModbusController` | field Modbus/RS485 provider development |
| `15_WebControl` | browser transport milestone |
| `16_BLEControl` | BLE transport / Platform 0.3.9 milestone |

These directories may be referenced by older videos, evidence records and commits, so Platform 0.4.0 stabilization does not mass-rename them. New product features should extend the canonical platform/services rather than create another duplicated control stack.

## Start here

- Platform: `06_OTARecovery/README.md`
- SD applications: `09_SDWidgetLibrary/README.md`
- Climate Controller: `18_ClimateController/README.md`
- Android: `17_MobileControl/README.md`
- User installation: `../docs/QUICKSTART.md`
