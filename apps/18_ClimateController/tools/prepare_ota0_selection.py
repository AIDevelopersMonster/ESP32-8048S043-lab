#!/usr/bin/env python3
"""Prepare (never flash) ota_0 NEW for the verified Sample A RTU fix build.

Requires sector 0 seq=1 VALID and sector 1 seq=2 VALID. Only sector 0 will
be replaced; sector 1 keeps the previous working ota_1 for rollback.
"""

import argparse
import hashlib
import os
from pathlib import Path
import struct
import zlib

from prepare_ota1_selection import SECTOR_SIZE, ota_entry, partitions

APP_SHA256 = "4092e66d550c56caea5269014aae49ea5a1305575527599ab5a0617f69625376"


def candidate_from_otadata(original: bytes) -> bytes:
    if len(original) != 2 * SECTOR_SIZE:
        raise ValueError("otadata readback must contain exactly 8192 bytes")
    for sector, expected_seq in ((0, 1), (1, 2)):
        seq, state, _ = ota_entry(original, sector)
        if (seq, state) != (expected_seq, 2):
            raise ValueError(f"expected sector {sector} seq={expected_seq} VALID, "
                             f"got seq={seq} state={state}")
    candidate = bytearray(b"\xff" * SECTOR_SIZE)
    # With two OTA slots seq=3 selects ota_0. NEW becomes PENDING_VERIFY.
    struct.pack_into("<I", candidate, 0, 3)
    struct.pack_into("<I", candidate, 24, 0)
    struct.pack_into("<I", candidate, 28,
                     zlib.crc32(struct.pack("<I", 3), 0xFFFFFFFF))
    ota_entry(candidate, 0)  # Check the generated CRC before writing a file.
    return bytes(candidate)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--partition-table", type=Path, required=True)
    parser.add_argument("--otadata", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    part = partitions(args.partition_table.read_bytes())
    candidate = candidate_from_otadata(args.otadata.read_bytes())
    image = args.image.read_bytes()
    digest = hashlib.sha256(image).hexdigest()
    if digest != APP_SHA256 or not image or image[0] != 0xE9:
        raise ValueError(f"RTU fix image signature or SHA-256 differs: {digest}")
    if len(image) > part["ota_0"][1]:
        raise ValueError("image exceeds ota_0 size")
    with os.fdopen(os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), "wb") as output:
        output.write(candidate)
    print("Source: ota_1 VALID seq=2; sector 0 seq=1 VALID")
    print(f"Verified RTU fix app SHA-256: {digest}")
    print("Prepared: ota_0 NEW, seq=3, CRC valid; 4096 bytes")
    print(f"Output: {args.output}")
    print(f"Output SHA-256: {hashlib.sha256(candidate).hexdigest()}")
    print("No flash was written. Target sector in the separate verified step: 0xF000")
    print("Keep sector 1 at 0x10000 untouched: ota_1 VALID rollback image.")


if __name__ == "__main__":
    main()
