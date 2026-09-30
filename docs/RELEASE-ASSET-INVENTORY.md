# Current release asset inventory — 2026-09-30

This file is the checked public-asset snapshot for the current KONTAKTS platform, SD library and Android client.

The GitHub Release asset digest is authoritative. Documentation and Web Flasher entries must match it.

## KONTAKTS Platform 0.4.0

Release tag:

```text
app06-v0.4.0
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `app06-factory-recovery-v0.1.0-full.bin` | 1,126,656 | `c241682145ff84db8f930d7a80997ea9a60583793456ab3a5cef4a457b01787d` |
| `app06-ota-recovery-v0.4.0-full.bin` | 2,030,128 | `f2787661fd55a8a6da1cd8131b644d3c874d89182d1cc5a7f057255a777893b0` |
| `app06-ota.bin` | 1,899,056 | `f5a494ab887ba010c130c5afb29f1478fe42bb4d8384b5f136a414842dd554b6` |
| `app06-ota.json` | 319 | `1ac83a7dc948af10375bd03034034dd1d81b9413a45984dae5682e52d1f747ba` |

Acceptance note:

- app-only image came from CI run `36713194827`;
- flash digest verified on Sample A;
- booted through NEW / PENDING_VERIFY;
- Climate Controller + local Web page physically checked;
- image then confirmed VALID.

The full image is published for first install / Web Flasher. Browser fresh-install of that complete image remains a separate installation-path retest from the already accepted app-only OTA path.

## Previous platform 0.3.9

Release tag:

```text
app06-v0.3.9
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `app06-factory-recovery-v0.1.0-full.bin` | 1,126,656 | `c241682145ff84db8f930d7a80997ea9a60583793456ab3a5cef4a457b01787d` |
| `app06-ota-recovery-v0.3.9-full.bin` | 2,014,912 | `20d3cd2675e49fa84e1be6ff9dbf0c01a4d3786234138cf4511014b9b4fdbc76` |
| `app06-ota.bin` | 1,883,840 | `061630b7871e045ebcded0837893cece455db95d093ed70fdf3dfba9ed8ccb26` |

0.3.9 remains useful as the previous accepted integrated platform / rollback reference, but it is no longer the current platform.

## KONTAKTS SD Application Library — current

Mutable prerelease tag:

```text
app09-sd-current
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `kontakts-sd-library.json` | 399 | `c3a616dcb58df61b497355fd0d49c23bfef508f0a6630de316afb393d9cc7aac` |
| `kontakts-sd-library.zip` | 41,909 | `73bee9c2c744569d05880ef351262d434a5b486048633b5dc63ef9a3733b1ed8` |
| `widget-climate-controller.zip` | 2,016 | `7b75d5859a34c763f8bb14ce76ddfc043d423ea455b1d0425fef577d2bf5eac3` |
| `widget-clock.zip` | 1,795 | `21544faab9cc4adefcbd9ff802efe5ce7ac3ba44f9f2fd017b6d48d86394d50b` |
| `widget-modbus-controller.zip` | 3,775 | `7de0827262071708391b7c155ae59e822a57cd53b12227d7c667e406a00ef214` |
| `widget-usb-serial-terminal.zip` | 6,672 | `32e0cbc33c2017030e41cfaee5dea08b1b3877c1e8f71f4d0e945d8009ad7487` |
| `widget-weather.zip` | 3,254 | `2e79b215066f0871b8dbf230d40562d14fa42a26946d9ae32b6277178d63f2f5` |
| `widget-youtube-led.zip` | 1,875 | `9635af2a0060ffd95c9ae9e0357e3ab2a130c8509ef6961cb2029cf89dcc92d9` |
| `widget-youtube.zip` | 3,212 | `02c76d9e158735965a537af607833d05173b2c07b7ea7c60ae04e3e2157bb49f` |

Because `app09-sd-current` is intentionally mutable, re-check these values after every SD republish.

## KONTAKTS Mobile App17 0.1.1

Release tag:

```text
app17-v0.1.1
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `kontakts-mobile-app17-v0.1.1-debug.apk` | 832,235 | `5de40bc802438a35285077241da603b0441ead28179928fad0e15ea0285868d0` |
| `kontakts-mobile-app17-v0.1.1-debug.apk.sha256` | 119 | `99c0545a2f9cd5554a7198bac25ec208be8fb062231ce0e7b2dec2f45b658ab3` |

The APK is the exact physically tested CI artifact from workflow run `36507403489`.

## Audit result

```text
Platform 0.4.0 OTA image        MATCH / PHYSICAL + WEB PASS
Platform 0.4.0 full image       MATCH / PUBLISHED
Previous Platform 0.3.9         MATCH
SD current bundle               MATCH
Climate Controller SD package   MATCH
App17 APK                       MATCH
```
