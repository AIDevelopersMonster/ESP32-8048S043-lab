#!/usr/bin/env python3
"""Prepare, but never flash, the second otadata sector for Sample A App18.

Only the inactive ota_1 has been flashed with the verified App18 application.
The first 4 KiB otadata sector (ota_0 VALID) stays untouched on the device.
"""

import argparse
import hashlib
import os
from pathlib import Path
import struct
import zlib

APP_SHA256 = "ed75a33fb90e04d1bb21ebd50dec45a7eb4c793ab91e730fb93c5e6860064efe"
EXPECTED = {
    "nvs": (0x9000, 0x6000),
    "otadata": (0xF000, 0x2000),
    "factory": (0x20000, 0x300000),
    "ota_0": (0x320000, 0x300000),
    "ota_1": (0x620000, 0x300000),
    "storage": (0x920000, 0x600000),
}
SECTOR_SIZE = 4096


def partitions(raw: bytes) -> dict[str, tuple[int, int]]:
    if len(raw) != SECTOR_SIZE:
        raise ValueError("partition table readback must contain exactly 4096 bytes")
    found = {}
    for offset in range(0, len(raw), 32):
        magic, part_type, subtype, start, size, label, flags = struct.unpack_from(
            "<HBBII16sI", raw, offset
        )
        if magic != 0x50AA:
            break
        name = label.split(b"\0", 1)[0].decode("ascii")
        found[name] = (start, size)
        if name in ("ota_0", "ota_1") and (part_type != 0 or subtype != 0x10 + int(name[-1])):
            raise ValueError(f"unexpected application subtype for {name}")
    for name, expected in EXPECTED.items():
        if found.get(name) != expected:
            raise ValueError(f"{name} differs from the verified Sample A layout: {found.get(name)}")
    return found


def ota_entry(raw: bytes, sector: int) -> tuple[int, int, int]:
    seq, label, state, crc = struct.unpack_from("<I20sII", raw, sector * SECTOR_SIZE)
    expected_crc = zlib.crc32(struct.pack("<I", seq), 0xFFFFFFFF)
    if crc != expected_crc:
        raise ValueError(f"otadata sector {sector} has invalid CRC")
    return seq, state, crc


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--partition-table", type=Path, required=True)
    parser.add_argument("--otadata", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    part = partitions(args.partition_table.read_bytes())
    original = args.otadata.read_bytes()
    if len(original) != 2 * SECTOR_SIZE:
        raise ValueError("otadata readback must contain exactly 8192 bytes")
    seq, state, crc = ota_entry(original, 0)
    if (seq, state) != (1, 2):  # ota_0, ESP_OTA_IMG_VALID
        raise ValueError(f"expected ota_0 VALID (seq=1 state=2), got seq={seq} state={state}")
    if original[SECTOR_SIZE:] != bytes([0xFF]) * SECTOR_SIZE:
        raise ValueError("second otadata sector is not fully erased; stop and inspect")

    image = args.image.read_bytes()
    image_sha256 = hashlib.sha256(image).hexdigest()
    if image_sha256 != APP_SHA256 or not image or image[0] != 0xE9:
        raise ValueError(f"App18 image signature or SHA-256 differs: {image_sha256}")
    if len(image) > part["ota_1"][1]:
        raise ValueError("App18 image exceeds ota_1 size")

    # ESP-IDF v5.5.5 esp_ota_select_entry_t:
    # ota_seq [0:4], label [4:24], ota_state [24:28], CRC [28:32].
    # seq=2 -> ota_1 with two slots; state=0 -> ESP_OTA_IMG_NEW.
    # The bootloader changes NEW to PENDING_VERIFY on the first boot.
    candidate = bytearray(bytes([0xFF]) * SECTOR_SIZE)
    struct.pack_into("<I", candidate, 0, 2)
    struct.pack_into("<I", candidate, 24, 0)
    struct.pack_into("<I", candidate, 28, zlib.crc32(struct.pack("<I", 2), 0xFFFFFFFF))
    assert struct.unpack_from("<I20sII", candidate)[2] == 0
    assert zlib.crc32(candidate[:4], 0xFFFFFFFF) == struct.unpack_from("<I", candidate, 28)[0]

    # Refuse accidental overwrite of an existing, possibly previously used sector.
    with os.fdopen(os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), "wb") as output:
        output.write(candidate)

    print("Source: ota_0 VALID, seq=1; second sector erased")
    print(f"Verified App18 app SHA-256: {image_sha256}")
    print("Prepared: ota_1 NEW, seq=2, CRC valid; 4096 bytes")
    print(f"Output: {args.output}")
    print(f"Output SHA-256: {hashlib.sha256(candidate).hexdigest()}")
    print("No flash was written. Target sector in the separate verified step: 0x10000")


if __name__ == "__main__":
    main()
