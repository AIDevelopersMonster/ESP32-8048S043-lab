#!/usr/bin/env python3
"""Create a reviewed keyword summary from firmware strings.txt.

The raw strings file may contain local paths, user names or device-specific data.
This tool reads the local strings report and writes a safer Markdown summary with
sanitized paths, grouped keyword hits and interpretation boundaries.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

Entry = Tuple[int, str]

CATEGORIES: Dict[str, List[str]] = {
    "application_identity": [
        r"LVGL Widgets Demo",
        r"LVGL v\d+",
        r"Arduino_GFX",
        r"Arduino_ESP32RGBPanel",
        r"Arduino15",
        r"esp32\\hardware\\esp32\\2\.0\.3",
    ],
    "display_rgb_panel": [
        r"esp_lcd_new_rgb_panel",
        r"esp_lcd_panel_reset",
        r"esp_lcd_panel_init",
        r"Arduino_ESP32RGBPanel",
        r"getFrameBuffer",
        r"rgb",
        r"display",
        r"lcd",
    ],
    "touch_i2c": [
        r"touch",
        r"gt911",
        r"goodix",
        r"i2c",
        r"sda gpio",
        r"scl gpio",
    ],
    "uart_usb": [
        r"uart",
        r"/dev/uart",
        r"usb_serial_jtag",
        r"usb",
    ],
    "gpio_backlight_pwm": [
        r"gpio",
        r"backlight",
        r"pwm",
    ],
    "ota_storage_fs": [
        r"otadata",
        r"ota",
        r"spiffs",
        r"sdcard",
        r"sd",
        r"nvs",
    ],
    "factory_test_words": [
        r"factory",
        r"selftest",
        r"diagnostic",
        r"\btest\b",
        r"pass",
        r"fail",
    ],
}

STRONG_PATTERNS = [
    r"LVGL Widgets Demo",
    r"LVGL v\d+",
    r"Arduino_ESP32RGBPanel",
    r"Arduino_GFX",
    r"esp_lcd_new_rgb_panel",
    r"esp_lcd_panel_init",
    r"esp_lcd_panel_reset",
    r"esp32\\hardware\\esp32\\2\.0\.3",
]

WINDOWS_PATH_RE = re.compile(r"[A-Za-z]:\\[^\s`|]+")


def parse_strings(path: Path) -> List[Entry]:
    entries: List[Entry] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"\s*0x([0-9A-Fa-f]+):\s*(.*)$", line)
        if not m:
            # Also accept Select-String-style lines ending with :0x1234: text
            m = re.search(r":0x([0-9A-Fa-f]+):\s*(.*)$", line)
        if not m:
            continue
        entries.append((int(m.group(1), 16), m.group(2)))
    return entries


def sanitize(s: str) -> str:
    def repl(match: re.Match[str]) -> str:
        path = match.group(0)
        # Keep useful library/file tail, hide host/user prefix.
        parts = re.split(r"\\+", path)
        if len(parts) >= 4:
            tail = "\\".join(parts[-4:])
            return f"<WINDOWS_PATH>\\{tail}"
        return "<WINDOWS_PATH>"

    return WINDOWS_PATH_RE.sub(repl, s).replace("|", "\\|")


def matched(entries: Iterable[Entry], patterns: List[str]) -> List[Entry]:
    compiled = [re.compile(p, re.IGNORECASE) for p in patterns]
    out: List[Entry] = []
    seen = set()
    for off, s in entries:
        if any(rx.search(s) for rx in compiled):
            key = (off, s)
            if key not in seen:
                out.append((off, s))
                seen.add(key)
    return out


def write_summary(entries: List[Entry], out: Path, max_per_category: int) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    strong = matched(entries, STRONG_PATTERNS)

    with out.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# Factory firmware keyword summary\n\n")
        f.write("This report is generated from the local `strings.txt` output. Raw strings are not published by default.\n\n")
        f.write("## High-confidence application leads\n\n")
        if strong:
            f.write("| Offset | String |\n")
            f.write("|---:|---|\n")
            for off, s in strong[:max_per_category]:
                f.write(f"| 0x{off:08X} | `{sanitize(s)}` |\n")
            if len(strong) > max_per_category:
                f.write(f"\nTruncated: {len(strong) - max_per_category} additional strong leads.\n")
        else:
            f.write("No strong application-identity leads found.\n")
        f.write("\n")

        f.write("## Grouped keyword hits\n\n")
        for name, patterns in CATEGORIES.items():
            hits = matched(entries, patterns)
            f.write(f"### {name}\n\n")
            f.write(f"Matches: `{len(hits)}`\n\n")
            if hits:
                f.write("| Offset | String |\n")
                f.write("|---:|---|\n")
                for off, s in hits[:max_per_category]:
                    f.write(f"| 0x{off:08X} | `{sanitize(s)}` |\n")
                if len(hits) > max_per_category:
                    f.write(f"\nTruncated: {len(hits) - max_per_category} additional matches.\n")
            else:
                f.write("No matches.\n")
            f.write("\n")

        f.write("## Interpretation boundary\n\n")
        f.write("String hits are static evidence only. They can identify likely frameworks, libraries and app labels, but they do not prove runtime behavior, pin mapping, touch controller identity or a factory-test entry path.\n")
        f.write("\n")
        f.write("Raw paths and host/user-specific fragments are sanitized before publishing. Review the output before committing it.\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("strings", type=Path, help="Path to analysis/strings.txt")
    ap.add_argument("--out", type=Path, required=True, help="Markdown summary output path")
    ap.add_argument("--max-per-category", type=int, default=80)
    args = ap.parse_args()

    entries = parse_strings(args.strings)
    print(f"Input strings: {args.strings}")
    print(f"Parsed entries: {len(entries)}")
    for name, patterns in CATEGORIES.items():
        print(f"{name}: {len(matched(entries, patterns))}")
    write_summary(entries, args.out, args.max_per_category)
    print(f"Wrote: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
