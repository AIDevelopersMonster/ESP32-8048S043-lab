# App 05 - Wi-Fi Network Provisioning

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Programmer:** Sol  
**Engineer:** Alex Malachevsky  
**Status:** IMPLEMENTATION / BUILD PENDING / PHYSICAL PENDING

## Goal

Validate the first network service layered on the closed App04 storage contract:

- saved Wi-Fi credentials in NVS;
- bounded STA connection attempts;
- AP fallback on first boot or failed saved network;
- local HTTP setup page;
- Wi-Fi scan endpoint;
- save/apply credentials without printing passwords;
- persistent reconnect after reboot;
- clear credentials and return to AP setup.

App05 deliberately remains headless. Display/touch integration is restored only after the network state machine passes independently on the real board.

## State model

```text
BOOT
  |
  +-- saved credentials --> STA_CONNECTING
  |                            |
  |                            +-- DHCP success --> STA_ONLINE
  |                            |
  |                            +-- bounded failure --> AP_SETUP
  |
  +-- no credentials -----------------------------> AP_SETUP

AP_SETUP
  |
  +-- KONTAKTS-XXXXXX access point
  +-- HTTP setup page
  +-- scan/select router SSID
  +-- save credentials to NVS
  +-- request STA connection
```

## HTTP endpoints

- `GET /` - setup/status page;
- `GET /scan` - current visible SSID list as JSON;
- `POST /save` - save `ssid` and `password`, then attempt STA connection;
- `POST /clear` - erase saved Wi-Fi credentials and return to AP setup;
- `GET /status` - compact JSON state and IP address.

Passwords are accepted by `/save` but are never returned by `/status`, `/`, `/scan`, or serial logging.

## Physical PASS boundary

1. clean NVS / no credentials enters AP setup;
2. AP name starts with `KONTAKTS-` and a device-derived suffix;
3. client can join the AP and reach the setup page;
4. `/scan` returns nearby Wi-Fi networks;
5. real router credentials can be saved and used;
6. DHCP address is obtained;
7. reboot reconnects without re-entering credentials;
8. saved-network failure falls back to AP setup after a bounded attempt window;
9. `/clear` returns the device to provisioning mode;
10. serial logs never contain the Wi-Fi password.

## Next controlled variable

After App05 physical pass: App06 OTA / rollback / recovery.
