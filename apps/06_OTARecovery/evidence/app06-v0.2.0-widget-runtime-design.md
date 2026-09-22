# Platform firmware v0.2.0 - persistent filesystem widget runtime design

Branch: `agent/app07-widget-runtime`
Status: SOURCE IN PROGRESS - PHYSICAL VALIDATION REQUIRED

## Goal

Close two platform questions together:

1. can the already validated internal `storage` SPIFFS partition become a persistent application-resource store across firmware OTA;
2. can declarative UI widgets be installed/replaced from that filesystem and rendered without reflashing the firmware while firmware-resident STATUS / OTA / recovery controls always remain available.

## Proven predecessors

- ESP32-8048S043 App04 physically proved NVS + SPIFFS `storage` mount, persistence and chunked file access.
- ESP32-8048S043 App06 physically proved GitHub OTA, SHA-256/image validation, NVS preservation, LVGL9/GT911 shell and rollback.
- WT32-SC01-PLUS Example 21 physically proved `widget.tmp -> schema validation -> widget.json -> LVGL render -> reset/autoload` without reflashing native firmware.

## Safety boundary

External widget JSON is data, not executable code.

Firmware permanently owns:

```text
RGB/GT911 BSP
LVGL runtime
Wi-Fi provisioning
filesystem driver
Widget Runtime parser/validator
STATUS shell
OTA shell
rollback/recovery
allowed bindings/actions
```

Filesystem owns only declarative resources:

```text
/storage/widget.json
```

Temporary install path:

```text
/storage/widget.tmp
```

A rejected document never becomes the active widget. The active file is backed up during replacement and restored if the final rename fails.

## Widget schema v1 target

Maximum JSON size: 32 KiB.
Maximum objects: 24.

Allowed objects:

- `label` - static text or allow-listed runtime binding;
- `bar` - static 0..100 value;
- `button` - only safe navigation actions `show_status` / `show_ota`.

Allowed bindings:

- `system.uptime`
- `system.heap`
- `system.psram`
- `wifi.ip`
- `wifi.rssi`
- `firmware.version`
- `ota.state`

Unknown object types, bindings and actions are rejected.

## Physical acceptance sequence

1. OTA current platform firmware to v0.2.0.
2. Confirm firmware-resident STATUS/OTA shell still works with no widget file.
3. Upload widget A through the web control page.
4. Confirm widget A renders from `/storage/widget.json` without reboot/firmware flash.
5. Reset the board and confirm `WIDGET AUTOLOAD PASS` plus the same widget A.
6. Upload widget B without firmware OTA and confirm the display changes to B.
7. Reset and confirm widget B persists.
8. Perform a later firmware OTA and verify `/storage/widget.json` survives unchanged.
9. Delete/corrupt/reject a widget and verify STATUS/OTA/recovery remain reachable.

No physical PASS is claimed until this sequence is reproduced on the reference board.
