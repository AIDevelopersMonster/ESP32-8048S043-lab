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

## Follow-on transports completed

The transport-independent design was subsequently exercised by:

- **App16** — BLE transport, physically accepted in Platform 0.3.9;
- **App17** — native Android BLE client, physically proven through real MA01 relay ON/OFF.

Therefore Web, BLE, Android and UART0/P1 now share the same command/service/provider architecture without duplicating MA01 Modbus register knowledge.

## Deferred App15-specific work

The browser MVP remains accepted. Follow-up work is limited to Web/Modbus polling efficiency, a clearer explicit READ ALL operation and UI/diagnostic polish. Those items should not be mixed with the already completed BLE/mobile transport proof.

