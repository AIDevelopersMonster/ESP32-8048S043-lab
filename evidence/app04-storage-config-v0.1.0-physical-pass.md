# App04 Storage Config v0.1.0 - Physical Pass Evidence

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Firmware:** `app04-storage-config-v0.1.0.bin`  
**Target:** ESP32-S3 / 16 MB flash / 8 MB PSRAM sample  
**Result:** PHYSICAL PASS

## Flash evidence

User flashed the merged image at offset `0x0` with esptool 5.4.0 over COM12 at 921600 baud.

Observed device identity:

- ESP32-S3 QFN56 revision v0.2;
- 40 MHz crystal;
- embedded PSRAM 8 MB;
- image write and hash verification completed successfully.

## Boot / partition evidence

ESP-IDF v5.5.5 booted normally with the intended 16 MB layout:

- `nvs` at `0x9000`, size `0x6000`;
- `otadata` at `0xF000`, size `0x2000`;
- `phy_init` at `0x11000`, size `0x1000`;
- `factory` at `0x20000`, size `0x300000`;
- `ota_0` at `0x320000`, size `0x300000`;
- `ota_1` at `0x620000`, size `0x300000`;
- `storage` at `0x920000`, size `0x600000`;
- `coredump` at `0xF20000`, size `0x10000`.

The factory application loaded successfully from `0x20000`.

## Persistent NVS evidence

Repeated real-board boots reported:

- `boot_count=2`
- `boot_count=3`
- `boot_count=4`
- `boot_count=5`

Across those boots:

- `device_name=KONTAKTS-8048S043`
- `brightness=80`

remained stable.

The first monitored boot already showing `boot_count=2` is consistent with the board having completed the immediate post-flash boot before the serial monitor attached.

## Filesystem / streamed resource evidence

Every captured boot reported:

- `FILESYSTEM mounted at /storage`
- `FILESYSTEM total=5775761 used=1004`

Resource results were stable across repeated boots:

- `/storage/platform.cfg`: `bytes=121`, `chunks=1`, `fnv1a=0xfa9d74b4`
- `/storage/ui-settings-screen.cfg`: `bytes=121`, `chunks=1`, `fnv1a=0xf0acadc6`

Each boot reached:

`APP04:STORAGE:PASS-CANDIDATE`

## Pass conclusion

The captured serial evidence satisfies the App04 physical-pass boundary:

1. NVS initializes and persists state across resets;
2. boot counter increments across repeated boots;
3. durable settings remain readable;
4. SPIFFS mounts consistently;
5. both streamed resources read successfully;
6. byte/chunk/checksum values are stable for unchanged resources;
7. no reset loop, partition error, mount failure, NVS error or heap exhaustion is present in the captured runs.

**Final status:** `BUILD PASS / PHYSICAL PASS / CLOSED`.
