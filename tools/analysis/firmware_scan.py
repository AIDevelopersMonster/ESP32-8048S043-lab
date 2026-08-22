#!/usr/bin/env python3
"""Basic offline scanner for full ESP32-S3 factory flash dumps.

This tool is intentionally dependency-free. It does not modify the input dump.
It produces a conservative first-pass report: hashes, ESP image headers,
partition table candidates and strings that may point to factory tests.
"""

from __future__ import annotations

import argparse
import hashlib
import math
import os
import re
import struct
from pathlib import Path
from typing import Iterable, List, Tuple

KEYWORDS = [
    "factory", "test", "selftest", "diagnostic", "diag", "pass", "fail",
    "lcd", "display", "rgb", "st7262", "ili9485", "touch", "gt911",
    "i2c", "sd", "sdcard", "tf", "wifi", "ble", "bt", "usb",
    "gpio", "psram", "flash", "backlight", "pwm", "lvgl", "ota",
    "uart", "rs485", "audio", "buzzer", "speaker", "adc", "rtc",
]

PARTITION_TYPES = {
    0x00: "app",
    0x01: "data",
}

APP_SUBTYPES = {
    0x00: "factory",
    0x10: "ota_0",
    0x11: "ota_1",
    0x12: "ota_2",
    0x13: "ota_3",
    0x14: "ota_4",
    0x15: "ota_5",
    0x16: "ota_6",
    0x17: "ota_7",
    0x18: "ota_8",
    0x19: "ota_9",
    0x20: "test",
}

DATA_SUBTYPES = {
    0x00: "ota",
    0x01: "phy",
    0x02: "nvs",
    0x03: "coredump",
    0x04: "nvs_keys",
    0x05: "efuse",
    0x81: "fat",
    0x82: "spiffs",
}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def printable_strings(data: bytes, min_len: int = 5) -> List[Tuple[int, str]]:
    out: List[Tuple[int, str]] = []
    start = None
    buf = []
    for i, b in enumerate(data):
        if 32 <= b <= 126:
            if start is None:
                start = i
            buf.append(chr(b))
        else:
            if start is not None and len(buf) >= min_len:
                out.append((start, "".join(buf)))
            start = None
            buf = []
    if start is not None and len(buf) >= min_len:
        out.append((start, "".join(buf)))
    return out


def image_headers(data: bytes) -> List[dict]:
    hits = []
    # ESP image headers are commonly at 0x0000 for legacy images or app offsets.
    # Scan 4 KiB boundaries to avoid over-reporting random 0xE9 bytes.
    for off in range(0, len(data) - 24, 0x1000):
        if data[off] != 0xE9:
            continue
        segments = data[off + 1]
        if segments == 0 or segments > 16:
            continue
        spi_mode = data[off + 2]
        spi_size_freq = data[off + 3]
        entry = struct.unpack_from("<I", data, off + 4)[0]
        hits.append({
            "offset": off,
            "segments": segments,
            "spi_mode": spi_mode,
            "spi_size_freq": spi_size_freq,
            "entry": entry,
        })
    return hits


def parse_partitions_at(data: bytes, table_off: int) -> List[dict]:
    entries = []
    for idx in range(0, 0xC00, 32):
        off = table_off + idx
        if off + 32 > len(data):
            break
        chunk = data[off:off + 32]
        if chunk == b"\xff" * 32:
            break
        if chunk[:2] != b"\xaa\x50":
            if idx == 0:
                return []
            break
        ptype = chunk[2]
        subtype = chunk[3]
        poff, psize = struct.unpack_from("<II", chunk, 4)
        label_raw = chunk[12:28]
        label = label_raw.split(b"\x00", 1)[0].decode("ascii", "replace")
        flags = struct.unpack_from("<I", chunk, 28)[0]
        entries.append({
            "index": len(entries),
            "type": ptype,
            "type_name": PARTITION_TYPES.get(ptype, f"0x{ptype:02x}"),
            "subtype": subtype,
            "subtype_name": subtype_name(ptype, subtype),
            "offset": poff,
            "size": psize,
            "label": label,
            "flags": flags,
        })
    return entries


def subtype_name(ptype: int, subtype: int) -> str:
    if ptype == 0x00:
        return APP_SUBTYPES.get(subtype, f"0x{subtype:02x}")
    if ptype == 0x01:
        return DATA_SUBTYPES.get(subtype, f"0x{subtype:02x}")
    return f"0x{subtype:02x}"


def find_partition_tables(data: bytes) -> List[Tuple[int, List[dict]]]:
    candidates = []
    for off in [0x8000, 0x9000, 0x10000, 0xE000, 0x7000]:
        entries = parse_partitions_at(data, off)
        if len(entries) >= 2:
            candidates.append((off, entries))
    return candidates


def entropy(block: bytes) -> float:
    if not block:
        return 0.0
    counts = [0] * 256
    for b in block:
        counts[b] += 1
    total = len(block)
    return -sum((c / total) * math.log2(c / total) for c in counts if c)


def write_report(path: Path, data: bytes, strings: List[Tuple[int, str]], out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    strings_file = out_dir / "strings.txt"
    with strings_file.open("w", encoding="utf-8", newline="\n") as f:
        for off, s in strings:
            f.write(f"0x{off:08X}: {s}\n")

    keyword_hits = []
    lowered = [(off, s, s.lower()) for off, s in strings]
    for kw in KEYWORDS:
        for off, s, sl in lowered:
            if kw in sl:
                keyword_hits.append((kw, off, s))

    md = out_dir / "factory-firmware-analysis.md"
    with md.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# Factory firmware first-pass analysis\n\n")
        f.write(f"Input: `{path}`\n\n")
        f.write(f"Size: {len(data)} bytes / 0x{len(data):X}\n\n")
        f.write(f"SHA-256: `{sha256_file(path)}`\n\n")

        f.write("## ESP image header candidates\n\n")
        headers = image_headers(data)
        if headers:
            f.write("| Offset | Segments | SPI mode | Size/freq | Entry |\n")
            f.write("|---:|---:|---:|---:|---:|\n")
            for h in headers:
                f.write(
                    f"| 0x{h['offset']:08X} | {h['segments']} | 0x{h['spi_mode']:02X} | "
                    f"0x{h['spi_size_freq']:02X} | 0x{h['entry']:08X} |\n"
                )
        else:
            f.write("No conservative 4 KiB-aligned ESP image headers found.\n")
        f.write("\n")

        f.write("## Partition table candidates\n\n")
        pts = find_partition_tables(data)
        if pts:
            for table_off, entries in pts:
                f.write(f"### Candidate at 0x{table_off:08X}\n\n")
                f.write("| # | Type | Subtype | Offset | Size | Label | Flags |\n")
                f.write("|---:|---|---|---:|---:|---|---:|\n")
                for e in entries:
                    f.write(
                        f"| {e['index']} | {e['type_name']} | {e['subtype_name']} | "
                        f"0x{e['offset']:08X} | 0x{e['size']:X} | `{e['label']}` | 0x{e['flags']:08X} |\n"
                    )
                f.write("\n")
        else:
            f.write("No standard partition table candidate found at common offsets.\n\n")

        f.write("## Keyword hits for possible factory tests\n\n")
        if keyword_hits:
            f.write("| Keyword | Offset | String |\n")
            f.write("|---|---:|---|\n")
            for kw, off, s in keyword_hits[:300]:
                safe = s.replace("|", "\\|")
                f.write(f"| `{kw}` | 0x{off:08X} | `{safe}` |\n")
            if len(keyword_hits) > 300:
                f.write(f"\nTruncated: {len(keyword_hits) - 300} additional hits. See `strings.txt`.\n")
        else:
            f.write("No keyword hits found in printable ASCII strings.\n")
        f.write("\n")

        f.write("## Entropy map, 64 KiB blocks\n\n")
        f.write("| Offset | Entropy | Note |\n")
        f.write("|---:|---:|---|\n")
        for off in range(0, len(data), 0x10000):
            block = data[off:off + 0x10000]
            ent = entropy(block)
            if block == b"\xff" * len(block):
                note = "erased"
            elif ent > 7.6:
                note = "high entropy / compressed or encrypted-like"
            elif ent < 1.0:
                note = "low entropy"
            else:
                note = "mixed"
            f.write(f"| 0x{off:08X} | {ent:.3f} | {note} |\n")

        f.write("\n## Interpretation boundary\n\n")
        f.write("This is a first-pass static scan only. It may identify strings, partitions and likely test labels, but it does not prove runtime behavior. Factory-test claims require a named specimen, logs/video and a reproducible entry path.\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("firmware", type=Path)
    ap.add_argument("--out", type=Path, default=None)
    args = ap.parse_args()

    path = args.firmware
    data = path.read_bytes()
    digest = sha256_file(path)
    print(f"Input: {path}")
    print(f"Size: {len(data)} bytes / 0x{len(data):X}")
    print(f"SHA-256: {digest}")

    headers = image_headers(data)
    print(f"ESP image header candidates: {len(headers)}")
    for h in headers:
        print(
            f"  0x{h['offset']:08X}: segments={h['segments']} "
            f"spi_mode=0x{h['spi_mode']:02X} size_freq=0x{h['spi_size_freq']:02X} "
            f"entry=0x{h['entry']:08X}"
        )

    pts = find_partition_tables(data)
    print(f"Partition table candidates: {len(pts)}")
    for off, entries in pts:
        print(f"  table @ 0x{off:08X}: {len(entries)} entries")
        for e in entries:
            print(
                f"    {e['index']:02d} {e['type_name']}/{e['subtype_name']} "
                f"off=0x{e['offset']:08X} size=0x{e['size']:X} label={e['label']}"
            )

    strings = printable_strings(data)
    print(f"Printable strings >=5 chars: {len(strings)}")

    if args.out:
        write_report(path, data, strings, args.out)
        print(f"Wrote analysis to: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
