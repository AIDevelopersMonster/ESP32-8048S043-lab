# Platform 0.3.7 -> 0.3.9 GitHub OTA regression — physical pass

Date: 2026-09-29  
Board: ESP32-8048S043 Sample A  
Classification: **PHYSICAL PASS**

## Purpose

Regression-check the public Web Flasher + GitHub OTA upgrade path using an older accepted platform image rather than only checking OTA from the current firmware.

## Physical sequence

1. Platform 0.3.7 was installed through the public Web Flasher.
2. `CHECK GITHUB` initially returned `ESP_ERR_NOT_FOUND`.
3. Root cause was identified as GitHub's repository-wide `releases/latest` pointer having been taken over by the unrelated App17 Android release, while Platform OTA resolves:
   `https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json`
4. App17 release policy was corrected so the Android MVP is published as a prerelease and does not own the firmware `latest` pointer.
5. GitHub `releases/latest` returned to `app06-v0.3.9`.
6. Without reflashing 0.3.7 again, `CHECK GITHUB` succeeded and detected Platform 0.3.9.
7. The board updated over GitHub OTA from 0.3.7 to 0.3.9 successfully.
8. After booting Platform 0.3.9, `CHECK GITHUB` was repeated and again resolved the current GitHub firmware release successfully.

## Result

```text
Web Flasher -> Platform 0.3.7        PASS
0.3.7 CHECK GITHUB -> 0.3.9          PASS
0.3.7 -> 0.3.9 OTA download/install PASS
boot into Platform 0.3.9             PASS
0.3.9 CHECK GITHUB current release   PASS
```

## Release-channel invariant

The firmware OTA contract depends on GitHub's repository-wide non-prerelease `latest` release containing `app06-ota.json`.

Therefore:

- normal/non-prerelease `latest` is reserved for the current KONTAKTS Platform firmware release;
- unrelated release channels such as Android App17 must not replace it;
- App17 v0.1.1 is published as a prerelease;
- mutable SD channel `app09-sd-current` is also published as a prerelease.

This preserves the existing Platform 0.3.7/0.3.9 OTA URL without changing firmware already deployed in the field.

## Related fix

```text
13a2a5300b4b3fafc1cf0ca50aab9ae1d767ce5d
fix(release): keep App17 from overriding platform OTA latest
```

Workflow:

```text
36521435425
KONTAKTS Mobile - Android Release
SUCCESS
```

At the verification checkpoint, GitHub `releases/latest` resolves to:

```text
app06-v0.3.9
```
