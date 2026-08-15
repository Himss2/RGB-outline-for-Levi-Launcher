#!/usr/bin/env python3

from pathlib import Path
import zipfile
import sys

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
DIST_DIR = ROOT / "dist"

DIST_DIR.mkdir(parents=True, exist_ok=True)

# Cari shared library hasil build.
candidates = list(BUILD_DIR.rglob("*.so"))

if not candidates:
    print("ERROR: no .so file found in build/")
    sys.exit(1)

# Ambil library utama.
so = next(
    (p for p in candidates if "lib" in p.name.lower()),
    candidates[0]
)

output = DIST_DIR / "RGBOutline.levipack"

print(f"Root : {ROOT}")
print(f"SO   : {so}")
print(f"Out  : {output}")

with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as z:
    z.write(so, arcname=f"lib/arm64-v8a/{so.name}")

print(f"Successfully created: {output}")
