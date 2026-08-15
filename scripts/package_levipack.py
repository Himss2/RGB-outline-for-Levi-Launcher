#!/usr/bin/env python3

from pathlib import Path
import json
import shutil
import sys
import zipfile


ROOT = Path(__file__).resolve().parent.parent

BUILD_DIR = ROOT / "build"
ASSETS_DIR = ROOT / "assets"
DIST_DIR = ROOT / "dist"
PACKAGE_DIR = ROOT / ".package"

SO_NAME = "liboutlinergb.so"
PACK_NAME = "OutlineRGB.levipack"

MANIFEST = ASSETS_DIR / "manifest.json"
SO_OUTPUT = DIST_DIR / SO_NAME
PACKAGE_OUTPUT = DIST_DIR / PACK_NAME


def log(message):
    print(f"[OutlineRGB] {message}")


def find_shared_library():
    candidates = list(BUILD_DIR.rglob(SO_NAME))

    if not candidates:
        return None

    # Prefer release builds if multiple copies exist.
    candidates.sort(
        key=lambda p: (
            "release" not in str(p).lower(),
            len(p.parts),
        )
    )

    return candidates[0]


def validate_manifest():
    if not MANIFEST.is_file():
        raise RuntimeError(
            f"Manifest not found: {MANIFEST}"
        )

    try:
        data = json.loads(
            MANIFEST.read_text(encoding="utf-8")
        )
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"Invalid manifest.json: {exc}"
        )

    if not isinstance(data, dict):
        raise RuntimeError(
            "manifest.json must contain a JSON object"
        )

    required = [
        "type",
        "name",
        "author",
        "version",
        "entry",
        "minecraft_versions",
    ]

    missing = [
        key for key in required
        if key not in data
    ]

    if missing:
        raise RuntimeError(
            "manifest.json is missing: "
            + ", ".join(missing)
        )

    if data["type"] != "preload-native":
        raise RuntimeError(
            'manifest "type" must be "preload-native"'
        )

    if data["entry"] != SO_NAME:
        raise RuntimeError(
            f'Manifest entry must be "{SO_NAME}"'
        )

    return data


def prepare_package(so_path):
    if PACKAGE_DIR.exists():
        shutil.rmtree(PACKAGE_DIR)

    PACKAGE_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    DIST_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    # Copy native library.
    shutil.copy2(
        so_path,
        PACKAGE_DIR / SO_NAME
    )

    # Copy manifest.
    shutil.copy2(
        MANIFEST,
        PACKAGE_DIR / "manifest.json"
    )


def build_levipack():
    if PACKAGE_OUTPUT.exists():
        PACKAGE_OUTPUT.unlink()

    files = [
        PACKAGE_DIR / SO_NAME,
        PACKAGE_DIR / "manifest.json",
    ]

    with zipfile.ZipFile(
        PACKAGE_OUTPUT,
        "w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:

        for file in files:
            archive.write(
                file,
                arcname=file.name
            )


def verify_package():
    if not PACKAGE_OUTPUT.is_file():
        raise RuntimeError(
            "LeviPack was not created"
        )

    with zipfile.ZipFile(
        PACKAGE_OUTPUT,
        "r"
    ) as archive:

        names = archive.namelist()

        expected = {
            SO_NAME,
            "manifest.json",
        }

        actual = set(names)

        if actual != expected:
            raise RuntimeError(
                "Unexpected LeviPack contents:\n"
                f"Expected: {sorted(expected)}\n"
                f"Actual:   {sorted(actual)}"
            )

        with archive.open("manifest.json") as fp:
            manifest = json.load(fp)

        if manifest.get("type") != "preload-native":
            raise RuntimeError(
                'Packaged manifest does not use '
                '"preload-native"'
            )

        if manifest.get("entry") != SO_NAME:
            raise RuntimeError(
                "Packaged manifest entry does not "
                "match the native library"
            )


def main():
    log("Starting LeviPack packaging")

    if not BUILD_DIR.exists():
        print(
            f"ERROR: build directory not found: "
            f"{BUILD_DIR}",
            file=sys.stderr,
        )
        return 1

    manifest = validate_manifest()

    log(
        f"Manifest: "
        f"{manifest['name']} "
        f"v{manifest['version']}"
    )

    so_path = find_shared_library()

    if so_path is None:
        print(
            f"ERROR: {SO_NAME} was not found "
            f"inside {BUILD_DIR}",
            file=sys.stderr,
        )
        return 1

    log(f"Native library: {so_path}")

    prepare_package(so_path)

    log("Creating LeviPack")

    build_levipack()

    verify_package()

    log("LeviPack created successfully")
    log(f"Output: {PACKAGE_OUTPUT}")

    print()
    print("========================================")
    print(" OutlineRGB LeviPack")
    print("========================================")
    print(f"File: {PACKAGE_OUTPUT}")
    print()
    print("Contents:")

    with zipfile.ZipFile(
        PACKAGE_OUTPUT,
        "r"
    ) as archive:
        for info in archive.infolist():
            print(
                f"  {info.filename:<30} "
                f"{info.file_size:>10} bytes"
            )

    print("========================================")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
