# App06 v0.1.1 GitHub update-check physical evidence

Date: 2026-09-07
Device: ESP32-8048S043 / ESP32-S3
Running image before OTA: factory v0.1.0
Branch: `agent/app06-ota-recovery`
Release: `app06-v0.1.1`

## Physical serial evidence

```text
APP06_OTA: Checking GitHub manifest: https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
esp-x509-crt-bundle: Certificate validated
esp-x509-crt-bundle: Certificate validated
APP06_OTA: Update available: installed=0.1.0 available=0.1.1
```

## Result

```text
GitHub manifest fetch over HTTPS      PASS
CA bundle certificate validation      PASS
GitHub redirect path                  PASS
manifest parsing                      PASS
semantic version comparison           PASS
factory 0.1.0 -> available 0.1.1      PASS
```

Classification:

`GITHUB UPDATE CHECK PHYSICAL PASS`

Not yet claimed here: firmware download, streaming SHA-256, byte-count verification, OTA write/finalize, boot into OTA slot, PENDING_VERIFY, NVS persistence, confirm, rollback, or factory recovery.
