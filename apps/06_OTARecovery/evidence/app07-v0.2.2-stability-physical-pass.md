# App07 v0.2.2 stability physical evidence

Branch: `agent/app07-widget-runtime`
Release: `app06-v0.2.2`
Date: 2026-09-07
Status: **PHYSICAL PASS - OTA/TLS/VALID/RESET/POWER-CYCLE STABILITY**

## Scope

This evidence closes the v0.2.2 stability regression gate that followed the v0.2.1 GitHub HTTPS timeout (`ESP_ERR_HTTP_EAGAIN`).

It does **not** yet close the full Widget Runtime physical test matrix; widget install/replace/autoload and invalid-widget isolation remain separate gates.

## Physical observations

The board was running `0.2.2` and repeated the GitHub manifest check multiple times without reset or TLS failure.

Observed serial evidence:

```text
I (117319) APP06_OTA: Checking GitHub manifest: https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
I (118539) esp-x509-crt-bundle: Certificate validated
I (120139) esp-x509-crt-bundle: Certificate validated
I (120799) APP06_OTA: GitHub up to date: installed=0.2.2 available=0.2.2
I (121459) APP07_UI: UI stack high-water=11348 largest_internal=31744
I (128179) APP06_OTA: Checking GitHub manifest: https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
I (129619) esp-x509-crt-bundle: Certificate validated
I (131419) esp-x509-crt-bundle: Certificate validated
I (131459) APP07_UI: UI stack high-water=11348 largest_internal=31744
I (132069) APP06_OTA: GitHub up to date: installed=0.2.2 available=0.2.2
I (145209) APP06_OTA: Running OTA candidate confirmed VALID
I (151459) APP07_UI: UI stack high-water=11348 largest_internal=31744
```

The user then physically tested both:

1. normal reset;
2. full power removal and power restoration.

In both cases v0.2.2 booted and remained operational with the previously confirmed OTA image. User report: `в этой части все тип-топ`.

## Result

```text
v0.2.2 boots and runs                    PASS
repeated GitHub manifest checks           PASS
TLS certificate validation                PASS
installed/available version comparison    PASS
largest internal block remains 31744 B    PASS
CONFIRM -> VALID                           PASS
VALID survives normal reset               PASS
VALID survives full power cycle           PASS
```

## Engineering conclusion

The v0.2.2 memory/TLS hardening resolves the physically observed v0.2.1 GitHub-check failure in this test path.

The reduction of the LVGL internal draw buffer and associated memory hardening provide a stable enough baseline for subsequent Widget Runtime and filesystem tests.

## Next physical gates

1. install `widget-demo-a.json` from the web UI and verify live render;
2. reset and verify widget A autoload;
3. install `widget-demo-b.json` without firmware OTA and verify live replacement;
4. reset and verify widget B autoload;
5. reject an invalid widget while preserving the active widget and firmware-resident STATUS/OTA/recovery;
6. later OTA must preserve `/storage/widget.json`.
