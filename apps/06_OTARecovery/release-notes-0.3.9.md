# KONTAKTS Platform 0.3.9 — physical acceptance

Status: **PHYSICAL PASS / Sample A / 2026-09-29**.

Platform 0.3.9 adds the App16 BLE transport while preserving HMI, Web, SD, OTA and technological UART0/P1 operation.

## Accepted control architecture

```text
HMI -----------+
UART0/P1 ------+
Web browser ---+--> command/service layer --> providers --> hardware
BLE -----------+
Mobile app ----+
```

MA01 remains implemented in the provider layer. User transports do not duplicate its Modbus register map.

## BLE

```text
Device   KONTAKTS-8048
Service  FFF0
Command  FFF1  WRITE
Response FFF2  READ
```

A BLE client writes the same ASCII/UTF-8 commands used by UART0, then reads the latest textual result from FFF2. Current laboratory MVP supports one BLE connection.

## Physical PASS

Verified together on Sample A:

- 800x480 RGB display and GT911;
- SD library and relay application;
- MA01 over UART1 GPIO17/18 + automatic-direction RS485;
- HMI/Web/relay shared state;
- Wi-Fi setup AP and STA/router operation;
- technological UART0/P1 using KONTAKTSerial 0.3.1;
- GitHub OTA CHECK;
- BLE discovery, GATT connection and command/response operation.

Physically tested full image from CI run 36491495390:

```text
SHA-256
20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

The published release is rebuilt from the accepted source on main and therefore receives its own release-asset SHA-256.

## Memory/coexistence hardening

The acceptance required explicit ownership of scarce internal/DMA-capable RAM:

- display/LVGL resources are allocated before optional BLE;
- LVGL partial draw buffer is in PSRAM;
- NimBLE host allocations use external RAM where supported;
- OTA worker stack is allocated in PSRAM;
- RGB bounce buffer remains in internal DMA-capable RAM;
- failure of optional BLE does not block platform boot.

This removed the startup failures and allowed display, SD, HTTP, Wi-Fi, BLE, OTA and MA01 control to coexist.
