# Platform v0.2.0 - Widget Runtime physical test plan

Branch: `agent/app07-widget-runtime`
Release: `app06-v0.2.0`
Status: BUILD/RELEASE PASS - PHYSICAL VALIDATION PENDING

## Purpose

Physically close two platform questions:

1. persistent internal filesystem resources across reset and later firmware OTA;
2. live declarative widget install/replace from filesystem while firmware-resident STATUS/OTA/recovery remain available.

## Preconditions

Before installing v0.2.0, confirm the currently running v0.1.2 image so the rollback baseline is clean:

```text
0.1.2 -> CONFIRM -> image_state=VALID
```

Then install v0.2.0 through the existing GitHub OTA path.

Expected first v0.2.0 boot:

```text
version=0.2.0
image_state=PENDING_VERIFY
display=READY
FILESYSTEM mounted at /storage
widget=none
```

Confirm v0.2.0 before reset-based persistence tests.

## Test A - filesystem + first widget

Use:

`widgets/widget-demo-a.json`

1. Open the device web page.
2. Under `Filesystem Widget`, select `widget-demo-a.json`.
3. Press `UPLOAD & INSTALL WIDGET`.
4. Expect HTTP text `INSTALL PASS`.
5. On the display open `WIDGET`.
6. Expect `Platform Status` and live values for firmware, uptime, IP, heap, PSRAM and RSSI.
7. Verify `OPEN STATUS` and `OPEN OTA` return to firmware-resident system pages.

Expected serial evidence:

```text
APP07_WIDGET: WIDGET INSTALL PASS id=demo.platform-status
APP07_UI: WIDGET RENDER PASS id=demo.platform-status
```

## Test B - reset/autoload persistence

1. Reset the board normally.
2. Expect the same widget A without re-upload.

Expected serial evidence:

```text
APP07_WIDGET: WIDGET AUTOLOAD PASS id=demo.platform-status
APP07_UI: WIDGET RENDER PASS id=demo.platform-status
```

This closes filesystem persistence across reset.

## Test C - live replacement without firmware OTA

Use:

`widgets/widget-demo-b.json`

1. Upload `widget-demo-b.json` from the same web page.
2. Do not perform firmware OTA and do not reboot first.
3. Expect the WIDGET page to change live to `Second Widget`.
4. Verify its text explicitly states it was loaded without firmware OTA.
5. Verify STATUS and OTA buttons still navigate to firmware-resident pages.

Expected serial evidence:

```text
APP07_WIDGET: WIDGET INSTALL PASS id=demo.second-screen
APP07_UI: WIDGET RENDER PASS id=demo.second-screen
```

This closes live filesystem widget replacement independent of firmware update.

## Test D - second reset

Reset again.

Expected:

```text
APP07_WIDGET: WIDGET AUTOLOAD PASS id=demo.second-screen
```

and widget B is displayed again.

## Test E - bad widget isolation

Upload a deliberately invalid JSON or unsupported action.

Expected:

- HTTP 400 / validation reason;
- active `widget.json` remains unchanged;
- STATUS/OTA/recovery remain available;
- no crash or reset loop.

## Later OTA persistence gate

After v0.2.0 is validated, a later platform OTA must update only an app slot and must not erase the `storage` partition.

Expected after that later OTA:

```text
/storage/widget.json unchanged
same widget autoloads
saved Wi-Fi unchanged
```

## PASS boundary

```text
v0.2.0 GitHub OTA                         PASS required
SPIFFS mount                              PASS required
widget A live install/render              PASS required
widget A reset/autoload                   PASS required
widget B live replace without OTA         PASS required
widget B reset/autoload                   PASS required
invalid widget rejection                  PASS required
STATUS/OTA/recovery isolation             PASS required
later firmware OTA preserves widget       deferred to next release
```

The first seven items close App07 functional storage/widget runtime. The final later-OTA persistence item closes cross-firmware resource persistence.
