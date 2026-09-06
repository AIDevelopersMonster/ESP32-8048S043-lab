# App 05 - Wi-Fi Network Provisioning

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Programmer:** Sol  
**Engineer:** Alex Malachevsky  
**Status:** BUILD PASS / PHYSICAL PASS / WEB FLASHER CANDIDATE

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

## Modular runtime structure

```text
main.c
  -> storage_credentials.*
  -> network_manager.*
  -> web_setup.*
```

`main.c` is now only the startup orchestrator. Storage, Wi-Fi state management and HTTP provisioning are separate functional modules so future OTA, GPIO-map, SD and project-loader services can be added without returning to a monolithic source file.

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

## Build validation

Modular App05 build run `34006797499` completed successfully on ESP-IDF 5.5.5.

Artifact:

- `app05-network-provisioning-v0.1.0`
- digest `sha256:ee159332e966c09b995c1ac705e18353ca0f5a3ec687c69adcc757abc1844aee`

## Physical validation

Real-board testing confirmed the network service is operational.

Captured serial evidence shows:

- Wi-Fi starts in STA + SoftAP mode;
- SoftAP DHCP starts at `192.168.4.1`;
- local HTTP setup server starts;
- saved SSID is loaded from NVS while password remains hidden;
- first association refusal triggers bounded retry `1/5`;
- subsequent WPA2 association succeeds;
- DHCP assigns STA address `10.113.29.119`;
- App05 reaches `STA online ip=10.113.29.119`;
- no panic, reset loop, NVS failure or network crash occurs.

The user confirmed the exercised App05 flow with `Все работает`.

See `evidence/app05-network-provisioning-v0.1.0-physical-pass.md`.

## Web Flasher status

The firmware is now eligible for a Web Flasher candidate entry. App05 should be exposed as a browser-installable network-provisioning stage because this is a real reusable platform capability, unlike the internal pass-through App04 storage experiment.

Web Flasher remains `CANDIDATE` until browser-to-board installation is physically confirmed.

## Next controlled variable

After App05 Web Flasher physical pass: App06 OTA / rollback / recovery.
