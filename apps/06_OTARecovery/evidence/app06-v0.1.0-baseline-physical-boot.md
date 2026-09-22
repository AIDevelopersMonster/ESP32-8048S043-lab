# App06 v0.1.0 baseline physical boot evidence

Date: 2026-09-06  
Device: ESP32-8048S043 / ESP32-S3 revision v0.2  
Flash: 16 MB  
PSRAM: 8 MB  
Branch: `agent/app06-ota-recovery`  
CI source commit: `014af8e37a29a1f4806210230fd7562f9bd388bc`

## Flashed image

`app06-ota-recovery-v0.1.0-full.bin`

CI SHA-256:

`c241682145ff84db8f930d7a80997ea9a60583793456ab3a5cef4a457b01787d`

Physical esptool session:

```text
Connected to ESP32-S3 on COM12
Chip revision v0.2
Embedded PSRAM 8MB
Flash will be erased from 0x00000000 to 0x00113fff
Wrote 1126656 bytes at 0x00000000
Hash of data verified
Hard resetting via RTS pin
```

## First captured application boot

The first complete boot was captured before a later manual reset-button press.

```text
boot: ESP-IDF v5.5.5 2nd stage bootloader
boot.esp32s3: SPI Flash Size : 16MB
boot: Defaulting to factory image
boot: Loaded app from partition at offset 0x20000
app_init: Project name:     app06_ota_recovery
app_init: App version:      0.1.0
app_init: ESP-IDF:          v5.5.5
APP06: KONTAKTS App06 GitHub OTA / rollback / recovery start
APP06_OTA: Manifest URL: https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
APP06_OTA: Running partition=factory version=0.1.0 state=FACTORY
APP06_WEB: App06 HTTP control server started
APP05_NET: No saved Wi-Fi credentials
APP05_NET: AP setup active ssid=KONTAKTS-6C693D
APP05_NET: Open http://192.168.4.1/
APP06: APP06:OTA:READY version=0.1.0 running=factory image_state=FACTORY
```

A second nearly identical boot appears later in the serial transcript after the operator pressed the hardware reset button. That duplicate boot does not invalidate the first captured boot.

## Partition table observed on hardware

```text
nvs      0x00009000 0x00006000
otadata  0x0000f000 0x00002000
phy_init 0x00011000 0x00001000
factory  0x00020000 0x00300000
ota_0    0x00320000 0x00300000
ota_1    0x00620000 0x00300000
storage  0x00920000 0x00600000
coredump 0x00f20000 0x00010000
```

## Result

```text
full image write + host hash verify    PASS
bootloader / partition table           PASS
factory application v0.1.0             PASS
App06 OTA manager initialization        PASS
HTTP control server start               PASS
fresh-NVS AP setup fallback             PASS
no panic / Guru Meditation              PASS
```

Classification:

`BASELINE PHYSICAL BOOT PASS`

This is not yet `APP06 PHYSICAL PASS`. Wi-Fi provisioning, GitHub manifest check, OTA install, NVS preservation across OTA, `PENDING_VERIFY`, confirmation, rollback, and factory recovery remain open physical gates.
