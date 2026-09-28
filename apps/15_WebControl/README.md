# App15 — Browser control transport

Status: **PHYSICAL MVP PASS / FOLLOW-UP IMPROVEMENTS DEFERRED**.

App15 adds a browser transport to the existing ESP32-8048S043 platform. It does not duplicate MA01 Modbus register logic.

Architecture:

```text
HMI -----------+
UART0/P1 ------+
Web browser ---+--> command/service layer --> MA01 provider --> UART1 GPIO17/18 --> RS485 --> MA01
BLE -----------+
```

## Web UI

When the platform HTTP server is reachable, open:

```text
http://BOARD_IP/ma01
```

The page provides:

- MA01 address and bus state;
- DO1..DO8 current state;
- ACTION / ON / OFF / TOGGLE;
- LEVEL / PULSE mode;
- pulse width in milliseconds;
- SCAN;
- explicit slave-address setting;
- live state updates using Server-Sent Events.

The first implementation uses short SSE responses with browser reconnect. This keeps the ESP-IDF HTTP worker free between updates while still using the browser EventSource API.

## REST / SSE endpoints

```text
GET  /api/ma01/status
GET  /api/ma01/config
GET  /api/ma01/events

POST /api/ma01/command?ch=1&op=ACTION
POST /api/ma01/command?ch=1&op=ON
POST /api/ma01/command?ch=1&op=OFF
POST /api/ma01/command?ch=1&op=TOGGLE
POST /api/ma01/command?ch=1&op=MODE&value=LEVEL
POST /api/ma01/command?ch=1&op=PULSEMS&value=5000
POST /api/ma01/scan
POST /api/ma01/address?value=16
```

All control commands are validated and then passed to `command_service_execute()`. The web layer does not know MA01 register addresses.

## Security boundary

This MVP has no authentication. It is intended for a trusted local lab network or the board setup AP only.

Do not expose the HTTP port directly to the public Internet.

## Physical acceptance

Physical bench test completed on 2026-09-28 with:

- ESP32-8048S043 HMI active;
- MA01-XXCX0080 on UART1/RS485;
- browser MA01 Web Control page active;
- relay state visible in the browser and on the board HMI;
- browser control reaching the real relay module.

The MVP is accepted for continued platform development.

## Known limitations intentionally deferred

- a high number of Modbus timeouts is still observed and needs a separate timing/polling pass;
- the browser UI does not yet provide a dedicated **READ ALL** action;
- further UI/diagnostic polish is deferred unless a fault blocks normal use.

These are tracked as follow-up work and are not blockers for the current App15 MVP.

## Remaining control transports / clients

The common command/service architecture is intentionally kept transport-independent. Remaining planned user-facing work:

- **BLE transport** to the same command/service layer;
- **mobile application** for phone control, using the common service/API model rather than duplicating MA01-specific Modbus logic.



1. Build and flash the App15 branch.
2. Connect the board to Wi-Fi or its setup AP.
3. Open `/ma01` from a browser.
4. Confirm address and ONLINE state.
5. Operate DO1 from the browser and observe the physical relay.
6. Confirm the ESP32 HMI updates to the same state.
7. Confirm `MA01 DO1 INFO` over UART0/P1 reports the same state.
8. Change LEVEL/PULSE and pulse time in the browser and verify them on the real module.
9. Leave the page open for several minutes and confirm the HMI/touch remain responsive.

