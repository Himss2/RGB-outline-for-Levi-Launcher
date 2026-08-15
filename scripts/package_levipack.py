#!/usr/bin/env python3

from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import json
import shutil
import sys


# ============================================================
# Project paths
# ============================================================

ROOT = Path(__file__).resolve().parent.parent

DIST_DIR = ROOT / "dist"
PACKAGE_PATH = DIST_DIR / "OutlineRGB.levipack"

MANIFEST_PATH = ROOT / "manifest.json"

BUILD_DIR = ROOT / "build"


# ============================================================
# Configuration
# ============================================================

PACKAGE_LIB_DIR = Path("lib") / "arm64-v8a"

# Expected output name inside the LeviPack.
PACKAGE_SO_NAME = "libOutlineRGB.so"


# ============================================================
# Helpers
# ============================================================

def fail(message: str) -> None:
    print()
    print("ERROR:")
    print(message)
    print()
    sys.exit(1)


def find_shared_library() -> Path:
    """
    Locate the ARM64 shared library produced by Xmake.

    We search recursively inside build/ instead of hardcoding
    Xmake's generated directory structure because that structure
    can vary between configurations.
    """

    if not BUILD_DIR.exists():
        fail(f"Build directory does not exist: {BUILD_DIR}")

    candidates = sorted(
        p for p in BUILD_DIR.rglob("*.so")
        if p.is_file()
    )

    if not candidates:
        fail(
            "No .so file was found under build/.\n"
            "Make sure the Android ARM64 build completed successfully."
        )

    print("Found shared libraries:")
    for candidate in candidates:
        print(f"  - {candidate.relative_to(ROOT)}")

    # Prefer an already correctly named library.
    preferred_names = [
        "libOutlineRGB.so",
        "OutlineRGB.so",
    ]

    for preferred_name in preferred_names:
        for candidate in candidates:
            if candidate.name == preferred_name:
                return candidate

    # If there is only one .so, use it.
    if len(candidates) == 1:
        return candidates[0]

    # Otherwise prefer names containing OutlineRGB / outline.
    outline_candidates = [
        p for p in candidates
        if "outlinergb" in p.name.lower()
        or "outline" in p.name.lower()
    ]

    if len(outline_candidates) == 1:
        return outline_candidates[0]

    fail(
        "Multiple .so files were found, but the OutlineRGB library "
        "could not be identified automatically.\n\n"
        + "\n".join(
            f"  - {p.relative_to(ROOT)}"
            for p in candidates
        )
    )


def load_manifest() -> dict:
    if not MANIFEST_PATH.exists():
        fail(f"manifest.json not found: {MANIFEST_PATH}")

    try:
        with MANIFEST_PATH.open("r", encoding="utf-8") as f:
            manifest = json.load(f)
    except json.JSONDecodeError as exc:
        fail(
            f"manifest.json contains invalid JSON:\n"
            f"{exc}"
        )

    if not isinstance(manifest, dict):
        fail("manifest.json root must be a JSON object.")

    return manifest


def validate_manifest(manifest: dict) -> None:
    """
    Basic validation only.

    We intentionally do not rewrite the user's manifest here.
    """

    if "name" not in manifest:
        print("Warning: manifest.json has no 'name' field.")

    if "version" not in manifest:
        print("Warning: manifest.json has no 'version' field.")


def prepare_package_directory() -> Path:
    """
    Create a temporary directory containing exactly the files
    that will be packed into the LeviPack.
    """

    package_root = DIST_DIR / "_package"

    if package_root.exists():
        shutil.rmtree(package_root)

    package_root.mkdir(parents=True)

    return package_root


def copy_manifest(package_root: Path) -> None:
    destination = package_root / "manifest.json"

    shutil.copy2(
        MANIFEST_PATH,
        destination
    )


def copy_shared_library(
    package_root: Path,
    source_so: Path
) -> Path:

    lib_dir = package_root / PACKAGE_LIB_DIR
    lib_dir.mkdir(parents=True, exist_ok=True)

    destination = lib_dir / PACKAGE_SO_NAME

    shutil.copy2(
        source_so,
        destination
    )

    return destination


def create_levipack(package_root: Path) -> None:
    if DIST_DIR.exists() is False:
        DIST_DIR.mkdir(parents=True)

    if PACKAGE_PATH.exists():
        print(f"Removing old package: {PACKAGE_PATH}")
        PACKAGE_PATH.unlink()

    print()
    print("Creating LeviPack:")
    print(f"  {PACKAGE_PATH}")

    with ZipFile(
        PACKAGE_PATH,
        mode="w",
        compression=ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:

        for file_path in sorted(package_root.rglob("*")):
            if not file_path.is_file():
                continue

            archive_name = file_path.relative_to(package_root).as_posix()

            print(f"  + {archive_name}")

            archive.write(
                file_path,
                arcname=archive_name,
            )


def verify_package() -> None:
    if not PACKAGE_PATH.exists():
        fail(
            f"LeviPack was not created:\n"
            f"{PACKAGE_PATH}"
        )

    print()
    print("Verifying LeviPack...")

    with ZipFile(PACKAGE_PATH, "r") as archive:

        if archive.testzip() is not None:
            fail("ZIP CRC verification failed.")

        names = archive.namelist()

        required_files = [
            "manifest.json",
            f"{PACKAGE_LIB_DIR.as_posix()}/{PACKAGE_SO_NAME}",
        ]

        print()
        print("Package contents:")

        for name in names:
            print(f"  {name}")

        print()

        for required in required_files:
            if required not in names:
                fail(
                    "Required file is missing from LeviPack:\n"
                    f"  {required}"
                )

    print("LeviPack verification: OK")


# ============================================================
# Main
# ============================================================

def main() -> None:

    print("=" * 40)
    print("OutlineRGB LeviPack Builder")
    print("=" * 40)

    print()
    print(f"Project root : {ROOT}")
    print(f"Build dir    : {BUILD_DIR}")
    print(f"Manifest     : {MANIFEST_PATH}")
    print(f"Output       : {PACKAGE_PATH}")

    print()
    print("=" * 40)
    print("Checking manifest")
    print("=" * 40)

    manifest = load_manifest()
    validate_manifest(manifest)

    print("manifest.json: OK")

    print()
    print("=" * 40)
    print("Finding Android ARM64 library")
    print("=" * 40)

    source_so = find_shared_library()

    print()
    print(f"Selected library:")
    print(f"  {source_so.relative_to(ROOT)}")

    print()
    print("=" * 40)
    print("Preparing package")
    print("=" * 40)

    package_root = prepare_package_directory()

    copy_manifest(package_root)

    packaged_so = copy_shared_library(
        package_root,
        source_so
    )

    print()
    print("Package staging directory:")
    print(f"  {package_root}")

    print()
    print("Packaged native library:")
    print(f"  {packaged_so.relative_to(package_root)}")

    print()
    print("=" * 40)
    print("Creating LeviPack")
    print("=" * 40)

    create_levipack(package_root)

    print()
    print("=" * 40)
    print("Verifying LeviPack")
    print("=" * 40)

    verify_package()

    # Cleanup temporary staging directory.
    if package_root.exists():
        shutil.rmtree(package_root)

    print()
    print("=" * 40)
    print("BUILD SUCCESS")
    print("=" * 40)
    print()
    print(f"LeviPack:")
    print(f"  {PACKAGE_PATH}")
    print()


if __name__ == "__main__":
    main()
