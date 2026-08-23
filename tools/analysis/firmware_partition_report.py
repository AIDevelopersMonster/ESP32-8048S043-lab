#!/usr/bin/env python3
"""Generate a reproducible partition-level report for an ESP32 flash dump.

This tool is dependency-free and read-only by default. It reads a full flash image,
locates the ESP-IDF partition table, calculates per-partition SHA-256 values and
summarizes ESP image headers for bootloader/app partitions.

It does not publish or require proprietary firmware binaries. Optional binary
partition extraction is local-only and should not be committed.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import struct
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

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

COMMON_PARTITION_TABLE_OFFSETS = [0x8000, 0x9000, 0xE000, 0x10000, 0x7000]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def subtype_name(ptype: int, subtype: int) -> str:
    if ptype == 0x00:
        return APP_SUBTYPES.get(subtype, f"0x{subtype:02x}")
    if ptype == 0x01:
        return DATA_SUBTYPES.get(subtype, f"0x{subtype:02x}")
    return f"0x{subtype:02x}"


def parse_partitions_at(data: bytes, table_off: int) -> List[dict]:
    entries: List[dict] = []
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


def find_partition_tables(data: bytes) -> List[Tuple[int, List[dict]]]:
    found: List[Tuple[int, List[dict]]] = []
    for off in COMMON_PARTITION_TABLE_OFFSETS:
        entries = parse_partitions_at(data, off)
        if len(entries) >= 2:
            found.append((off, entries))
    return found


def entropy(block: bytes) -> float:
    if not block:
        return 0.0
    counts = [0] * 256
    for b in block:
        counts[b] += 1
    total = len(block)
    return -sum((c / total) * math.log2(c / total) for c in counts if c)


def printable_count(data: bytes, min_len: int = 5) -> int:
    count = 0
    run = 0
    for b in data:
        if 32 <= b <= 126:
            run += 1
        else:
            if run >= min_len:
                count += 1
            run = 0
    if run >= min_len:
        count += 1
    return count


def image_summary(data: bytes, base_offset: int = 0) -> Optional[dict]:
    if len(data) < 24 or data[0] != 0xE9:
        return None
    segments = data[1]
    if segments == 0 or segments > 16:
        return None
    spi_mode = data[2]
    spi_size_freq = data[3]
    entry = struct.unpack_from("<I", data, 4)[0]
    wp_pin = data[8]
    drive = data[9:12]
    chip_id = struct.unpack_from("<H", data, 12)[0]
    min_chip_rev = data[14]
    max_chip_rev = data[15]
    hash_appended = data[23]

    cursor = 24
    segment_rows = []
    valid = True
    for idx in range(segments):
        if cursor + 8 > len(data):
            valid = False
            break
        load_addr, length = struct.unpack_from("<II", data, cursor)
        cursor += 8
        if cursor + length > len(data):
            valid = False
            break
        segment_rows.append({
            "index": idx,
            "load_addr": load_addr,
            "length": length,
            "file_offset": base_offset + cursor,
        })
        cursor += length

    return {
        "segments": segments,
        "spi_mode": spi_mode,
        "spi_size_freq": spi_size_freq,
        "entry": entry,
        "wp_pin": wp_pin,
        "drive": drive.hex(),
        "chip_id": chip_id,
        "min_chip_rev": min_chip_rev,
        "max_chip_rev": max_chip_rev,
        "hash_appended": hash_appended,
        "parsed_length": cursor,
        "valid_segments": valid,
        "segment_rows": segment_rows,
    }


def safe_name(label: str, index: int) -> str:
    clean = "".join(c if c.isalnum() or c in "._-" else "_" for c in label).strip("._-")
    if not clean:
        clean = f"partition_{index:02d}"
    return f"{index:02d}_{clean}"


def collect_partition_rows(data: bytes, entries: List[dict]) -> List[dict]:
    rows = []
    for e in entries:
        start = e["offset"]
        end = start + e["size"]
        part = data[start:end] if 0 <= start < len(data) and end <= len(data) else b""
        all_ff = bool(part) and part == b"\xff" * len(part)
        all_zero = bool(part) and part == b"\x00" * len(part)
        img = image_summary(part, start) if e["type_name"] == "app" else None
        rows.append({
            **e,
            "end": end,
            "inside_image": bool(part),
            "sha256": sha256_bytes(part) if part else "",
            "entropy": entropy(part) if part else 0.0,
            "printable_strings": printable_count(part) if part else 0,
            "all_ff": all_ff,
            "all_zero": all_zero,
            "image": img,
        })
    return rows


def write_csv(rows: List[dict], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "index", "type", "subtype", "label", "offset", "size", "end",
            "sha256", "entropy", "printable_strings", "all_ff", "all_zero",
        ])
        for r in rows:
            w.writerow([
                r["index"], r["type_name"], r["subtype_name"], r["label"],
                f"0x{r['offset']:08X}", f"0x{r['size']:X}", f"0x{r['end']:08X}",
                r["sha256"], f"{r['entropy']:.3f}", r["printable_strings"],
                str(r["all_ff"]).lower(), str(r["all_zero"]).lower(),
            ])


def write_hashes(rows: List[dict], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8", newline="\n") as f:
        for r in rows:
            f.write(f"{r['sha256']}  {r['index']:02d}_{r['label']}  offset=0x{r['offset']:08X} size=0x{r['size']:X}\n")


def write_markdown(firmware: Path, image_sha: str, image_size: int, table_off: int, rows: List[dict], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# Firmware partition report\n\n")
        f.write("This report is generated from a local factory flash dump. The dump itself is not committed.\n\n")
        f.write("## Input\n\n")
        f.write(f"- File: `{firmware}`\n")
        f.write(f"- Size: `{image_size}` bytes / `0x{image_size:X}`\n")
        f.write(f"- SHA-256: `{image_sha}`\n")
        f.write(f"- Partition table offset: `0x{table_off:08X}`\n\n")

        f.write("## Partition hashes\n\n")
        f.write("| # | Type | Subtype | Label | Offset | Size | End | SHA-256 | Entropy | Strings | Note |\n")
        f.write("|---:|---|---|---|---:|---:|---:|---|---:|---:|---|\n")
        for r in rows:
            note = []
            if r["all_ff"]:
                note.append("erased")
            if r["all_zero"]:
                note.append("zeroed")
            if r["image"]:
                note.append(f"ESP image, {r['image']['segments']} segments")
            f.write(
                f"| {r['index']} | {r['type_name']} | {r['subtype_name']} | `{r['label']}` | "
                f"0x{r['offset']:08X} | 0x{r['size']:X} | 0x{r['end']:08X} | "
                f"`{r['sha256']}` | {r['entropy']:.3f} | {r['printable_strings']} | {', '.join(note)} |\n"
            )

        apps = [r for r in rows if r["type_name"] == "app"]
        if len(apps) >= 2:
            f.write("\n## App-slot comparison\n\n")
            for i in range(len(apps)):
                for j in range(i + 1, len(apps)):
                    a = apps[i]
                    b = apps[j]
                    relation = "IDENTICAL" if a["sha256"] == b["sha256"] else "DIFFERENT"
                    f.write(f"- `{a['label']}` vs `{b['label']}`: **{relation}**\n")

        f.write("\n## ESP image summaries\n\n")
        boot = image_summary(Path(firmware).read_bytes()[:0x8000], 0)
        if boot:
            write_image_md(f, "Bootloader candidate at 0x00000000", boot)
        for r in rows:
            if r["image"]:
                write_image_md(f, f"Partition `{r['label']}` at 0x{r['offset']:08X}", r["image"])

        f.write("\n## Boundary\n\n")
        f.write("This report proves partition structure, per-partition hashes and ESP-image metadata. It does not prove runtime behavior or factory-test entry paths.\n")


def write_image_md(f, title: str, img: dict) -> None:
    f.write(f"### {title}\n\n")
    f.write("```text\n")
    f.write(f"segments       : {img['segments']}\n")
    f.write(f"spi_mode       : 0x{img['spi_mode']:02X}\n")
    f.write(f"spi_size_freq  : 0x{img['spi_size_freq']:02X}\n")
    f.write(f"entry          : 0x{img['entry']:08X}\n")
    f.write(f"wp_pin         : 0x{img['wp_pin']:02X}\n")
    f.write(f"drive          : {img['drive']}\n")
    f.write(f"chip_id        : 0x{img['chip_id']:04X}\n")
    f.write(f"min_chip_rev   : {img['min_chip_rev']}\n")
    f.write(f"max_chip_rev   : {img['max_chip_rev']}\n")
    f.write(f"hash_appended  : 0x{img['hash_appended']:02X}\n")
    f.write(f"parsed_length  : 0x{img['parsed_length']:X}\n")
    f.write(f"valid_segments : {img['valid_segments']}\n")
    f.write("```\n\n")
    if img["segment_rows"]:
        f.write("| # | File offset | Load address | Length |\n")
        f.write("|---:|---:|---:|---:|\n")
        for s in img["segment_rows"]:
            f.write(f"| {s['index']} | 0x{s['file_offset']:08X} | 0x{s['load_addr']:08X} | 0x{s['length']:X} |\n")
        f.write("\n")


def extract_partitions(data: bytes, rows: List[dict], out_dir: Path) -> None:
    extract_dir = out_dir / "partitions-local-only"
    extract_dir.mkdir(parents=True, exist_ok=True)
    for r in rows:
        name = safe_name(r["label"], r["index"])
        path = extract_dir / f"{name}_0x{r['offset']:08X}_0x{r['size']:X}.bin"
        path.write_bytes(data[r["offset"]:r["end"]])
    readme = extract_dir / "README.txt"
    readme.write_text(
        "Local-only extracted partitions. Do not commit these .bin files unless redistribution is explicitly permitted.\n",
        encoding="utf-8",
        newline="\n",
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("firmware", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--table-offset", type=lambda x: int(x, 0), default=None)
    ap.add_argument("--extract", action="store_true", help="also extract partition binaries to a local-only folder")
    args = ap.parse_args()

    data = args.firmware.read_bytes()
    image_sha = sha256_file(args.firmware)
    args.out.mkdir(parents=True, exist_ok=True)

    if args.table_offset is not None:
        entries = parse_partitions_at(data, args.table_offset)
        tables = [(args.table_offset, entries)] if entries else []
    else:
        tables = find_partition_tables(data)

    if not tables:
        raise SystemExit("No partition table found. Try --table-offset 0x8000 or inspect the dump manually.")

    table_off, entries = tables[0]
    rows = collect_partition_rows(data, entries)

    write_markdown(args.firmware, image_sha, len(data), table_off, rows, args.out / "partition-report.md")
    write_csv(rows, args.out / "partition-table.csv")
    write_hashes(rows, args.out / "partition-hashes.sha256.txt")

    if args.extract:
        extract_partitions(data, rows, args.out)

    print(f"Input: {args.firmware}")
    print(f"Image SHA-256: {image_sha}")
    print(f"Partition table: 0x{table_off:08X} entries={len(rows)}")
    for r in rows:
        note = ""
        if r["image"]:
            note = f" image_segments={r['image']['segments']}"
        print(
            f"  {r['index']:02d} {r['type_name']}/{r['subtype_name']} {r['label']} "
            f"off=0x{r['offset']:08X} size=0x{r['size']:X} sha256={r['sha256']}{note}"
        )
    print(f"Wrote: {args.out / 'partition-report.md'}")
    print(f"Wrote: {args.out / 'partition-table.csv'}")
    print(f"Wrote: {args.out / 'partition-hashes.sha256.txt'}")
    if args.extract:
        print(f"Extracted local-only binaries under: {args.out / 'partitions-local-only'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
