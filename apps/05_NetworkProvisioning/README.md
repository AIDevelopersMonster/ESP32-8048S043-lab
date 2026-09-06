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

`main.c` is only the startup orchestrator. Storage, Wi-Fi state management and HTTP provisioning are separate functional modules so future OTA, GPIO-map, SD and project-loader services can be added without returning to a monolithic source file.

## State model

```text
BOOT
  |
  +-- saved credentials --> STA_CONNECTING (STA only)
  |                            |
  |                            +-- DHCP success --> STA_ONLINE
  |                            |                       |
  |                            |                       +-- setup AP disabled
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
  +-- request STA connection while setup AP remains available

STA_ONLINE loss
  |
  +-- bounded reconnect retries
  |
  +-- retry exhaustion --> AP_SETUP automatically
```

## HTTP endpoints

- `GET /` - setup/status page;
- `GET /scan` - visible SSID list as JSON while in AP setup mode;
- `POST /save` - save `ssid` and `password`, then attempt STA connection;
- `POST /clear` - erase saved Wi-Fi credentials and return to AP setup;
- `GET /status` - compact JSON state and IP address;
- `GET /favicon.ico` - `204 No Content` to avoid browser 404 noise.

Passwords are accepted by `/save` but are never returned by `/status`, `/`, `/scan`, or serial logging.

## Build validation

App05 v0.1.1 stability build run `34031657592` completed successfully on ESP-IDF 5.5.5.

Artifact:

- `app05-network-provisioning-v0.1.1`
- CI artifact digest `sha256:6ae0fb80d51cd08e4a5bb957c56a24ff369b7a9848f49fb1547a107df346a8cf`
- merged firmware SHA-256 `882A8D5FD461760FC0673B1A2BFDA071281F3F5F63EEBD5A92F67D26104F8D01`

The merged image size observed in the physical test package was 917632 bytes.

## Physical validation

### v0.1.0 historical result

The first App05 implementation passed the basic provisioning flow but a longer run exposed a real stability defect:

- setup AP remained active after STA success;
- retry exhaustion did not automatically restore AP setup after the initial one-shot supervisor task had exited;
- one `StoreProhibited` panic occurred after a setup-AP client rejoined while STA was already online.

The exact crashing function could not be proven because the backtrace was corrupted. The stability repair therefore removed the unnecessary post-connect AP lifecycle and restored a persistent network supervisor rather than attributing the crash to an unproven single cause.

### v0.1.1 physical stability pass

Real-board testing on Sample A confirmed the repaired lifecycle:

- ESP32-S3 rev v0.2, 16 MB flash, 8 MB PSRAM;
- first boot after full merged-image install correctly entered `AP_SETUP` because the full install erased NVS;
- setup AP `KONTAKTS-6C693D` started at `192.168.4.1`;
- client received `192.168.4.2` by DHCP;
- credentials for `TECNO CAMON 50` were saved without logging the password;
- STA associated using WPA2-PSK with RSSI about `-34 dBm`;
- DHCP assigned STA address `10.113.29.119`;
- runtime reached `STA online ip=10.113.29.119`;
- setup AP was automatically disabled immediately after successful STA/DHCP;
- Wi-Fi mode returned to `sta` only;
- the setup client was disconnected as a direct consequence of disabling the AP;
- no Guru Meditation, panic, reset loop, or unexpected reboot occurred in the observed post-connect period.

The key physical-pass sequence was:

```text
APP05_NET: STA online ip=10.113.29.119
wifi:mode : sta (84:fc:e6:6c:69:3c)
APP05_NET: Provisioning AP disabled; STA-only mode active
```

The observed `httpd_sock_err: error in recv : 113` lines immediately after AP shutdown are expected consequences of tearing down client sockets that existed on the setup AP. They are not runtime resets or network-state failures.

A final cosmetic handler fix after the physical test changes `/scan` handling so a completed `409 Conflict` response returns `ESP_OK` to the HTTP server, avoiding a false `uri handler execution failed` warning. This does not alter the tested Wi-Fi state machine.

See `evidence/app05-network-provisioning-v0.1.1-physical-pass.md`.

## Full install versus update boundary

A merged Web Flasher image is written from offset `0x0`. In the physical v0.1.1 test, esptool erased through `0x000e0fff`, which includes the NVS partition at `0x9000`. Therefore a full install is allowed to clear network credentials.

This defines an important App06 contract:

```text
FULL INSTALL / WEB FLASHER
  -> merged image at 0x0
  -> NVS may be erased

OTA / UPDATE
  -> app partition update
  -> NVS must be preserved
```

## Web Flasher status

App05 v0.1.1 is the current browser-install candidate. The catalog and manifest point to `app05-network-provisioning-v0.1.1.bin`.

Web Flasher remains `CANDIDATE` until browser-to-board installation of v0.1.1 is physically confirmed.

## Next controlled variable

After App05 v0.1.1 Web Flasher physical pass: App06 OTA / rollback / recovery.
