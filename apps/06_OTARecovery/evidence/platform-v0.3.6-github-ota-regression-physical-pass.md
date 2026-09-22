# Platform 0.3.6 GitHub OTA regression physical pass

Date: 2026-09-22  
Device: ESP32-8048S043 / Sample A  
Source platform: KONTAKTS Platform 0.3.5  
Target platform: KONTAKTS Platform 0.3.6  
Release: `app06-v0.3.6`

## Test purpose

Re-check the current GitHub OTA path after the Web Flasher and USB Serial Terminal work by upgrading the running platform from 0.3.5 to the released 0.3.6 application image.

The 0.3.6 release is intentionally a narrow OTA-validation release: relative to the 0.3.5 source line, the release commit changes the platform version so that the complete OTA update path can be exercised without mixing the test with unrelated functional changes.

The persistence boundary is:

```text
full Web Flasher install -> full-image path; Wi-Fi setup may be required again
GitHub OTA app image     -> writes an OTA application slot; NVS and saved Wi-Fi remain intact
```

## Physical result

Operator acceptance on Sample A:

```text
0.3.5 -> GitHub manifest check       PASS
newer 0.3.6 release detection        PASS
OTA application download             PASS
application verification/install     PASS
reboot into OTA candidate            PASS
PENDING_VERIFY handling              PASS
on-device OTA controls               PASS
saved Wi-Fi / NVS persistence        PASS
automatic Wi-Fi reconnect            PASS
post-update Platform 0.3.6           PASS
rollback / confirmation workflow     PASS
```

Classification:

`PLATFORM 0.3.6 GITHUB OTA REGRESSION PHYSICAL PASS`

## Video evidence

Physical demonstration of the 0.3.5 -> 0.3.6 OTA update:

https://youtube.com/shorts/70_Huyt8igQ

## Evidence boundary

This record is based on the operator's physical acceptance report and published video from 2026-09-22. A raw serial transcript for this particular 0.3.6 regression run is not archived here.

Earlier App06 evidence files contain detailed serial transcripts for the same OTA mechanisms: verified HTTPS manifest/download, SHA-256 verification, PENDING_VERIFY, Wi-Fi/NVS persistence, rollback and factory recovery.

The 0.3.6 run confirms that those mechanisms still operate correctly after the later Widget Runtime, SD library, Help Center and serial-service additions.
