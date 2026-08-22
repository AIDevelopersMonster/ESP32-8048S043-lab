#!/usr/bin/env python3
"""Placeholder web flasher packager.

The Web Flasher will be enabled only after safe, non-destructive firmware targets
are physically validated on a named specimen.
"""

from pathlib import Path

out = Path("web-site")
out.mkdir(exist_ok=True)
(out / "index.html").write_text("<h1>ESP32-8048S043 Lab Web Flasher</h1><p>Not enabled yet.</p>\n", encoding="utf-8")
print(f"prepared placeholder site at {out}")
