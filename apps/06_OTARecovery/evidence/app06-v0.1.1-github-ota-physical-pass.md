# App06 physical GitHub OTA: factory v0.1.0 -> ota_0 v0.1.1

Date: 2026-09-07
Device: ESP32-8048S043 / ESP32-S3
Source image: factory v0.1.0
Target release: app06-v0.1.1
Target OTA slot: ota_0 @ 0x320000

## Physical serial evidence

```text
APP06_OTA: Checking GitHub manifest: https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
esp-x509-crt-bundle: Certificate validated
APP06_OTA: Update available: installed=0.1.0 available=0.1.1
APP06_OTA: OTA target=ota_0 offset=0x320000 size=3145728
esp-x509-crt-bundle: Certificate validated
APP06_OTA: SHA-256 PASS: 45cf518fee46fa0e0470f1d6ceaf29183dd6515702c5677810b265e1dbc7647f
APP06_OTA: OTA verified; next boot partition=ota_0 version=0.1.1
```

After reboot:

```text
boot: Loaded app from partition at offset 0x320000
app_init: Project name:     app06_ota_recovery
app_init: App version:      0.1.1
APP06_OTA: Running partition=ota_0 version=0.1.1 state=PENDING_VERIFY
APP06_OTA: OTA candidate is PENDING_VERIFY: confirm it or rollback before reset
APP05_NET: Saved Wi-Fi credentials found for ssid=TECNO CAMON 50 (password hidden)
APP05_NET: STA connecting ssid=TECNO CAMON 50
APP06: APP06:OTA:READY version=0.1.1 running=ota_0 image_state=PENDING_VERIFY
wifi:connected with TECNO CAMON 50
esp_netif_handlers: sta ip: 10.113.29.119
APP05_NET: STA online ip=10.113.29.119
APP05_NET: Provisioning AP disabled; STA-only mode active
```

## Result

```text
manifest re-check before install        PASS
verified HTTPS firmware download        PASS
OTA target selection (ota_0)            PASS
streaming SHA-256                        PASS
ESP image validation                     PASS
boot partition selection                 PASS
reboot into ota_0                        PASS
running version 0.1.1                    PASS
PENDING_VERIFY state                     PASS
NVS / saved Wi-Fi persistence            PASS
automatic STA reconnect                  PASS
same DHCP IP observed (10.113.29.119)    PASS
provisioning AP disabled in STA mode     PASS
```

Classification:

`GITHUB OTA INSTALL PHYSICAL PASS`

## Non-fatal shutdown race observed

During the intentional reboot after OTA verification, the Wi-Fi disconnect handler treated Wi-Fi shutdown as an ordinary link loss and attempted bounded reconnect/AP fallback while the driver was stopping:

```text
APP05_NET: STA link lost; starting bounded reconnect
APP05_NET: STA disconnected; retry 1/5
APP05_NET: esp_wifi_connect failed: ESP_ERR_WIFI_NOT_STARTED
APP05_NET: STA unavailable after bounded retries; entering AP setup
APP05_NET: network_manager_enter_ap_setup(212): set APSTA failed
APP05_NET: Failed to enter AP setup: ESP_ERR_WIFI_STOP_STATE
```

The board immediately rebooted normally and OTA validity was unaffected. This should be cleaned up in the next OTA candidate by suppressing reconnect/AP fallback during an intentional reboot.

Still open after this evidence: explicit confirm to VALID, second-generation OTA rollback test, reset-before-confirm rollback test if desired, and explicit factory recovery.
