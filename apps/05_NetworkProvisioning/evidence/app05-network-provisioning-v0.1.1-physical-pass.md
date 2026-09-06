# App05 Network Provisioning v0.1.1 — Physical Stability Pass

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Specimen:** Sample A  
**Date:** 2026-09-06  
**Firmware:** App05 Network Provisioning v0.1.1 stability candidate  
**Branch tested:** `agent/app05-stability`  
**Source commit before cosmetic post-pass cleanup:** `39de81ba0c9839089fefb8b4663679d6906dc466`

## CI provenance

GitHub Actions run:

- run ID `34031657592`
- workflow `App 05 - Network Provisioning Build`
- result `success`
- ESP-IDF `v5.5.5`

Artifact:

- name `app05-network-provisioning-v0.1.1`
- artifact digest `sha256:6ae0fb80d51cd08e4a5bb957c56a24ff369b7a9848f49fb1547a107df346a8cf`
- merged BIN size `917632` bytes
- merged BIN SHA-256 `882A8D5FD461760FC0673B1A2BFDA071281F3F5F63EEBD5A92F67D26104F8D01`

## Flash method

The merged image was written with esptool to offset `0x0` over COM12 at 921600 baud.

Observed flash operation:

```text
Flash will be erased from 0x00000000 to 0x000e0fff...
Wrote 917632 bytes (536344 compressed) at 0x00000000
Hash of data verified.
Hard resetting via RTS pin...
```

Because the App05 NVS partition begins at `0x9000`, this full merged-image install erased the existing NVS credentials. The subsequent `No saved Wi-Fi credentials` boot path was therefore expected and is not a persistence defect.

## Boot identity

Observed hardware/runtime identity:

```text
ESP32-S3 QFN56 rev v0.2
Flash 16 MB
Embedded PSRAM 8 MB
Crystal 40 MHz
ESP-IDF v5.5.5
```

Partition table reported:

```text
nvs      0x00009000  0x00006000
otadata  0x0000f000  0x00002000
phy_init 0x00011000  0x00001000
factory  0x00020000  0x00300000
ota_0    0x00320000  0x00300000
ota_1    0x00620000  0x00300000
storage  0x00920000  0x00600000
coredump 0x00f20000  0x00010000
```

## Provisioning path exercised

First boot after the full install entered AP setup:

```text
APP05_NET: No saved Wi-Fi credentials
wifi:mode : sta + softAP
APP05_NET: AP setup active ssid=KONTAKTS-6C693D
DHCP server started ... 192.168.4.1
APP05_NET: Open http://192.168.4.1/
```

A client joined the setup AP and received `192.168.4.2`.

The setup page saved credentials for `TECNO CAMON 50` without exposing the password in logs:

```text
APP05_WEB: Credentials saved for ssid=TECNO CAMON 50 (password hidden)
APP05_NET: STA connecting ssid=TECNO CAMON 50 (setup AP remains during attempt)
```

Association succeeded:

```text
wifi:connected with TECNO CAMON 50
wifi:security: WPA2-PSK
rssi: -34
```

DHCP then assigned:

```text
sta ip: 10.113.29.119
APP05_NET: STA online ip=10.113.29.119
```

Immediately after success, the setup AP was shut down and Wi-Fi returned to STA-only mode:

```text
wifi:mode : sta (84:fc:e6:6c:69:3c)
APP05_NET: Provisioning AP disabled; STA-only mode active
```

This is the primary v0.1.1 stability acceptance condition.

## Post-connect behavior

No `Guru Meditation`, `StoreProhibited`, panic, reset loop, or unexpected reboot was observed after the repaired AP shutdown sequence.

The setup client was disconnected when the AP was intentionally disabled. The HTTP server then logged socket receive errors `113` for stale AP-side connections. These are expected teardown artifacts and do not indicate a runtime crash.

One separate warning, `httpd_uri: uri handler execution failed`, was traced to the `/scan` handler returning `ESP_ERR_INVALID_STATE` after already sending a valid `409 Conflict` response. A post-pass cosmetic fix changes that handler to return `ESP_OK` after sending the 409. This does not modify the Wi-Fi state machine that was physically tested.

## Result

```text
BUILD PASS                  PASS
FULL MERGED FLASH           PASS
AP SETUP                    PASS
AP DHCP                     PASS
HTTP SETUP                  PASS
SSID SCAN                   PASS
CREDENTIAL SAVE             PASS
PASSWORD LOG HYGIENE        PASS
WPA2 ASSOCIATION            PASS
STA DHCP                    PASS
STA ONLINE                  PASS
AUTO SETUP-AP SHUTDOWN      PASS
STA-ONLY POST-CONNECT MODE  PASS
PANIC / RESET LOOP          NOT OBSERVED
```

**Overall:** `PHYSICAL PASS / WEB FLASHER CANDIDATE`

The remaining acceptance gate is a browser-to-board installation of the v0.1.1 Web Flasher build after merge to `main` and GitHub Pages deployment.
