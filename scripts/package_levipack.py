#!/usr/bin/env python3

from pathlib import Path
import json
import os
import shutil
import sys
import zipfile


# ============================================================
# PATHS
# ============================================================

ROOT = Path(__file__).resolve().parent.parent

BUILD_DIR = ROOT / "build"
DIST_DIR = ROOT / "dist"

OUTPUT = DIST_DIR / "OutlineRGB.levipack"


# ============================================================
# HELPERS
# ============================================================

def print_header(text: str):
    print()
    print("=" * 40)
    print(text)
    print("=" * 40)


def fail(message: str):
    print()
    print("ERROR:")
    print(message)
    print()
    sys.exit(1)


def find_manifest() -> Path:
    """
    Locate manifest.json anywhere in the repository.

    Prefer repository root, then search recursively.
    """

    root_manifest = ROOT / "manifest.json"

    if root_manifest.is_file():
        return root_manifest

    candidates = sorted(
        p for p in ROOT.rglob("manifest.json")
        if ".git" not in p.parts
        and "build" not in p.parts
        and "dist" not in p.parts
    )

    if not candidates:
        fail(
            "manifest.json not found.\n"
            f"Repository root: {ROOT}"
        )

    if len(candidates) > 1:
        print("Multiple manifest.json files found:")

        for candidate in candidates:
            print(f"  - {candidate}")

        print()
        print(f"Using: {candidates[0]}")

    return candidates[0]


def find_library() -> Path:
    """
    Locate the ARM64 OutlineRGB shared library.
    """

    candidates = []

    # Preferred exact name.
    candidates.extend(
        BUILD_DIR.rglob("libOutlineRGB.so")
    )

    if candidates:
        return sorted(candidates)[0]

    # Fallback: any OutlineRGB library.
    candidates.extend(
        BUILD_DIR.rglob("*OutlineRGB*.so")
    )

    if candidates:
        return sorted(candidates)[0]

    fail(
        "libOutlineRGB.so not found in build directory.\n"
        f"Build directory: {BUILD_DIR}"
    )


def validate_manifest(manifest_path: Path):
    try:
        with manifest_path.open(
            "r",
            encoding="utf-8"
        ) as f:
            manifest = json.load(f)

    except json.JSONDecodeError as exc:
        fail(
            f"Invalid manifest.json:\n{exc}"
        )

    except OSError as exc:
        fail(
            f"Unable to read manifest.json:\n{exc}"
        )

    if not isinstance(manifest, dict):
        fail(
            "manifest.json root must be a JSON object."
        )

    print("Manifest JSON: valid")

    # Print useful metadata when available.
    for key in (
        "name",
        "id",
        "version",
        "description",
    ):
        if key in manifest:
            print(f"{key}: {manifest[key]}")

    return manifest


def clean_dist():
    DIST_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    if OUTPUT.exists():
        print(
            f"Removing old package: {OUTPUT}"
        )

        OUTPUT.unlink()


# ============================================================
# MAIN
# ============================================================

def main():

    print_header(
        "OutlineRGB LeviPack Builder"
    )

    print(f"Project root : {ROOT}")
    print(f"Build dir    : {BUILD_DIR}")
    print(f"Output       : {OUTPUT}")

    # --------------------------------------------------------
    # Manifest
    # --------------------------------------------------------

    print_header(
        "Checking manifest"
    )

    manifest_path = find_manifest()

    print(
        f"Manifest found: {manifest_path}"
    )

    manifest = validate_manifest(
        manifest_path
    )

    # --------------------------------------------------------
    # Shared library
    # --------------------------------------------------------

    print_header(
        "Checking shared library"
    )

    library_path = find_library()

    print(
        f"Library found: {library_path}"
    )

    if not library_path.is_file():
        fail(
            "Shared library does not exist."
        )

    # --------------------------------------------------------
    # Clean output
    # --------------------------------------------------------

    print_header(
        "Preparing output"
    )

    clean_dist()

    # --------------------------------------------------------
    # Create package
    # --------------------------------------------------------

    print_header(
        "Creating LeviPack"
    )

    with zipfile.ZipFile(
        OUTPUT,
        "w",
        compression=zipfile.ZIP_DEFLATED
    ) as package:

        # ----------------------------------------------------
        # manifest.json
        # ----------------------------------------------------

        package.write(
            manifest_path,
            "manifest.json"
        )

        print(
            "Added: manifest.json"
        )

        # ----------------------------------------------------
        # ARM64 library
        # ----------------------------------------------------

        package.write(
            library_path,
            "lib/arm64-v8a/libOutlineRGB.so"
        )

        print(
            "Added: "
            "lib/arm64-v8a/libOutlineRGB.so"
        )

    # --------------------------------------------------------
    # Verify
    # --------------------------------------------------------

    print_header(
        "Verifying package"
    )

    if not OUTPUT.is_file():
        fail(
            "LeviPack was not created."
        )

    with zipfile.ZipFile(
        OUTPUT,
        "r"
    ) as package:

        names = package.namelist()

        required = {
            "manifest.json",
            "lib/arm64-v8a/libOutlineRGB.so",
        }

        missing = required.difference(
            names
        )

        if missing:
            fail(
                "LeviPack is missing:\n"
                + "\n".join(
                    f"  - {item}"
                    for item in sorted(missing)
                )
            )

        # Validate manifest inside archive.
        try:
            packaged_manifest = json.loads(
                package.read(
                    "manifest.json"
                ).decode("utf-8")
            )

        except Exception as exc:
            fail(
                "Unable to validate packaged "
                f"manifest.json:\n{exc}"
            )

        print(
            "Packaged manifest: valid"
        )

        print()
        print(
            "Package contents:"
        )

        for name in names:
            print(
                f"  {name}"
            )

    # --------------------------------------------------------
    # Size
    # --------------------------------------------------------

    size = OUTPUT.stat().st_size

    print()
    print(
        f"Package size: {size:,} bytes"
    )

    print_header(
        "SUCCESS"
    )

    print(
        f"Created: {OUTPUT}"
    )


if __name__ == "__main__":
    main()
