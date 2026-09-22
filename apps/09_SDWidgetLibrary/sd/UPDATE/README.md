# SD /UPDATE

Status: **DESIGN CONTRACT / NOT YET ENABLED FOR FLASH WRITES**.

This directory is reserved for explicit offline firmware update packages. Merely inserting an SD card must never start an update.

Recommended layout:

```text
UPDATE/
├── platform/
│   ├── update.json
│   └── firmware.bin
└── project-name/
    ├── update.json
    └── firmware.bin
```

Expected future flow:

```text
SYS or SD
  -> scan UPDATE
  -> show package metadata
  -> validate board/app/version/size/SHA-256
  -> explicit INSTALL action
  -> stream firmware.bin to inactive OTA partition
  -> verify ESP application descriptor/version
  -> set boot partition
  -> reboot PENDING_VERIFY
  -> CONFIRM or ROLLBACK from SYS
```

`firmware.bin` must be streamed in small chunks. It must not be loaded into RAM as one image.

No automatic formatting. No automatic flashing. No package may bypass the platform OTA manager.
