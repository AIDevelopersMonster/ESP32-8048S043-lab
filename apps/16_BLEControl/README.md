# App16 — BLE control transport

Status: **PHYSICAL PASS / PLATFORM 0.3.9**.

App16 adds a native ESP-IDF NimBLE transport to the existing KONTAKTS command/service layer. BLE is a transport only: it does not duplicate MA01 Modbus register knowledge.

```text
HMI -----------+
UART0/P1 ------+
Web browser ---+--> command/service layer --> MA01 provider --> UART1 GPIO17/18 --> RS485 --> MA01
BLE -----------+
Mobile app ----+
```

## BLE contract

```text
Device name : KONTAKTS-8048
Service     : 0xFFF0
Command     : 0xFFF1  WRITE
Response    : 0xFFF2  READ
Connections : 1
```

Commands are UTF-8 / ASCII text and are the same commands accepted by the technological UART0 service, for example:

```text
HELP
MA01 ADDR
MA01 READ
MA01 CONFIG
MA01 DO1 INFO
MA01 DO1 ACTION
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 TOGGLE
MA01 DO1 MODE LEVEL
MA01 DO1 MODE PULSE
MA01 DO1 PULSEMS 5000
```

The client writes the command to characteristic `FFF1` and then reads the latest textual response from `FFF2`. The current MVP deliberately uses READ rather than notifications.

## Physical acceptance

Sample A physical acceptance completed on 2026-09-29.

Confirmed together on the real platform:

- 800x480 RGB display and GT911 touch;
- SD library and SD application launcher;
- MA01 relay control through UART1 GPIO17/18 + external automatic-direction RS485 adapter;
- browser MA01 control and HMI state synchronization;
- Wi-Fi setup AP and saved-credential STA connection;
- technological UART0/P1 control through KONTAKTSerial 0.3.1;
- GitHub OTA check after runtime memory hardening;
- BLE advertising as `KONTAKTS-8048`;
- BLE GATT connection and command/response control path from external BLE clients.

Validated full-image artifact:

```text
Version : 0.3.9
SHA-256 : 20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

This hash identifies the physically tested CI full image from workflow run `36491495390`. A later release rebuild from the same source may have a different binary hash because build metadata can change.

## Memory/coexistence fixes required for PASS

The first BLE build exhausted scarce internal/DMA-capable RAM before the RGB/LVGL stack could stabilize. App16 therefore fixes ownership/order instead of reducing platform functionality:

- initialize display/LVGL before optional BLE;
- place NimBLE host allocations in PSRAM where supported;
- keep the LVGL partial draw buffer in PSRAM;
- preserve the RGB bounce buffer in internal/DMA-capable memory;
- create the 16 KiB OTA worker stack explicitly in PSRAM;
- keep BLE optional so a BLE allocation failure cannot prevent the platform shell from booting.

The physically accepted runtime showed display, SD, HTTP, Wi-Fi, BLE and MA01 control coexisting.

## Security boundary

The current laboratory BLE MVP has no application-layer authentication or pairing requirement. Use it only in a trusted local/lab environment.

Production follow-up items:

- vendor-unique 128-bit UUIDs instead of 16-bit FFFx development UUIDs;
- authenticated pairing/bonding or application authentication;
- optional response notifications;
- explicit connection/status UI;
- mobile application UX.

## Client guidance

A generic BLE GATT client can be used from a phone or computer:

1. scan for `KONTAKTS-8048`;
2. connect;
3. discover service `FFF0`;
4. write a command to `FFF1`;
5. read `FFF2` for the result.

Only one BLE connection is supported at a time.

## Result

App16 closes the transport proof:

```text
BLE client
 -> GATT FFF1
 -> command/service layer
 -> MA01 provider
 -> UART1 / RS485
 -> physical relay
 -> shared HMI/Web state
```

The dedicated mobile-client stage is now completed as **App17 v0.1.1 physical MVP PASS**. The Android client uses this same GATT contract and does not duplicate MA01-specific Modbus logic.

Physical App17 video:

https://youtube.com/shorts/FxDnALva3xM

Confirmed path:

```text
Android App17
 -> BLE FFF1 / FFF2
 -> command/service layer
 -> MA01 provider
 -> UART1 / RS485
 -> physical relay ON/OFF
```



## Wi-Fi + BLE coexistence note

Platform 0.3.9 physically ran Wi-Fi and BLE in the same ESP32-S3 platform while the RGB/LVGL display, SD, HTTP and MA01/RS485 services remained active. This demonstrates coexistence on the accepted workload.

The earlier failures were resolved by memory-placement and initialization-order fixes; they were not evidence that Wi-Fi and BLE are mutually exclusive on ESP32-S3. Radio airtime is still shared, so this result should not be generalized into a claim of unlimited simultaneous throughput.
