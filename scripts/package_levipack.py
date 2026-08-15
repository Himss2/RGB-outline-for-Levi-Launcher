#!/usr/bin/env python3

from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import json
import shutil
import sys


# ============================================================
# Project configuration
# ============================================================

ROOT = Path(__file__).resolve().parent.parent

BUILD_DIR = ROOT / "build"
DIST_DIR = ROOT / "dist"

PACKAGE_NAME = "OutlineRGB.levipack"
PACKAGE_PATH = DIST_DIR / PACKAGE_NAME

PACKAGE_SO_NAME = "libOutlineRGB.so"

PACKAGE_LIB_DIR = Path("lib") / "arm64-v8a"


# ============================================================
# Output helpers
# ============================================================

def info(message=""):
    print(message)


def section(title):
    print()
    print("=" * 40)
    print(title)
    print("=" * 40)


def fail(message):
    print()
    print("ERROR:")
    print(message)
    print()
    sys.exit(1)


# ============================================================
# Manifest discovery
# ============================================================

def find_manifest():
    """
    Find manifest.json automatically.

    We do not assume that manifest.json is located at the
    repository root.

    Candidate locations are checked first, followed by a
    recursive search.
    """

    section("Searching manifest.json")

    candidates = [
        ROOT / "manifest.json",
        ROOT / "assets" / "manifest.json",
        ROOT / "asset" / "manifest.json",
        ROOT / "levipack" / "manifest.json",
        ROOT / "pack" / "manifest.json",
        ROOT / "package" / "manifest.json",
        ROOT / "packages" / "manifest.json",
        ROOT / "resources" / "manifest.json",
    ]

    checked = set()

    for candidate in candidates:
        candidate = candidate.resolve()

        if candidate in checked:
            continue

        checked.add(candidate)

        if candidate.is_file():
            info(f"Found manifest:")
            info(f"  {candidate.relative_to(ROOT)}")
            return candidate

    # Recursive search as fallback.
    recursive_matches = sorted(
        p for p in ROOT.rglob("manifest.json")
        if p.is_file()
        and ".git" not in p.parts
        and "build" not in p.parts
        and "dist" not in p.parts
    )

    if not recursive_matches:
        info("No manifest.json found.")
        info()
        info("Repository files at root:")

        for item in sorted(ROOT.iterdir()):
            info(f"  {item.name}")

        fail(
            "manifest.json could not be located.\n"
            "Make sure the repository contains a LeviPack manifest."
        )

    if len(recursive_matches) == 1:
        manifest = recursive_matches[0]

        info("Found manifest:")
        info(f"  {manifest.relative_to(ROOT)}")

        return manifest

    info("Multiple manifest.json files were found:")

    for manifest in recursive_matches:
        info(f"  - {manifest.relative_to(ROOT)}")

    # Prefer a manifest outside examples/tests/templates.
    preferred = [
        p for p in recursive_matches
        if not any(
            part.lower() in {
                "example",
                "examples",
                "test",
                "tests",
                "template",
                "templates",
            }
            for part in p.parts
        )
    ]

    if len(preferred) == 1:
        manifest = preferred[0]

        info()
        info("Selected:")
        info(f"  {manifest.relative_to(ROOT)}")

        return manifest

    fail(
        "Multiple manifest.json files were found and the correct "
        "one could not be determined automatically."
    )


# ============================================================
# Manifest validation
# ============================================================

def load_manifest(manifest_path):
    try:
        with manifest_path.open(
            "r",
            encoding="utf-8",
        ) as file:
            manifest = json.load(file)

    except UnicodeDecodeError as exc:
        fail(
            f"Could not decode manifest.json as UTF-8:\n{exc}"
        )

    except json.JSONDecodeError as exc:
        fail(
            "manifest.json contains invalid JSON:\n"
            f"{exc}"
        )

    if not isinstance(manifest, dict):
        fail(
            "manifest.json must contain a JSON object."
        )

    return manifest


def validate_manifest(manifest):
    section("Validating manifest")

    if "name" in manifest:
        info(f"name    : {manifest['name']}")
    else:
        info("Warning: manifest has no 'name' field.")

    if "version" in manifest:
        info(f"version : {manifest['version']}")
    else:
        info("Warning: manifest has no 'version' field.")

    info("Manifest JSON: OK")


# ============================================================
# Shared library discovery
# ============================================================

def find_shared_libraries():
    section("Searching Android ARM64 libraries")

    if not BUILD_DIR.exists():
        fail(
            f"Build directory does not exist:\n"
            f"{BUILD_DIR}"
        )

    libraries = sorted(
        p for p in BUILD_DIR.rglob("*.so")
        if p.is_file()
    )

    if not libraries:
        fail(
            "No .so files were found under build/.\n"
            "The Android ARM64 build probably did not produce "
            "a shared library."
        )

    info("Found .so files:")

    for library in libraries:
        info(
            f"  - {library.relative_to(ROOT)}"
        )

    return libraries


def select_shared_library(libraries):
    """
    Select the OutlineRGB shared library.

    Priority:
      1. exact libOutlineRGB.so
      2. exact OutlineRGB.so
      3. filename containing OutlineRGB
      4. only .so available
    """

    preferred_names = [
        "libOutlineRGB.so",
        "OutlineRGB.so",
    ]

    for preferred in preferred_names:
        matches = [
            p for p in libraries
            if p.name == preferred
        ]

        if len(matches) == 1:
            return matches[0]

    outline_matches = [
        p for p in libraries
        if "outlinergb" in p.name.lower()
    ]

    if len(outline_matches) == 1:
        return outline_matches[0]

    if len(libraries) == 1:
        return libraries[0]

    fail(
        "Multiple .so files were found, but the OutlineRGB "
        "library could not be identified.\n\n"
        + "\n".join(
            f"  - {p.relative_to(ROOT)}"
            for p in libraries
        )
    )


# ============================================================
# Package staging
# ============================================================

def prepare_staging_directory():
    section("Preparing LeviPack")

    staging = DIST_DIR / "_package"

    if staging.exists():
        info("Removing old staging directory...")
        shutil.rmtree(staging)

    staging.mkdir(
        parents=True,
        exist_ok=True,
    )

    info(f"Staging directory:")
    info(f"  {staging}")

    return staging


def copy_manifest(
    manifest_path,
    staging,
):
    destination = staging / "manifest.json"

    shutil.copy2(
        manifest_path,
        destination,
    )

    info(
        f"Added: manifest.json"
    )


def copy_shared_library(
    source,
    staging,
):
    destination_dir = (
        staging
        / PACKAGE_LIB_DIR
    )

    destination_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    destination = (
        destination_dir
        / PACKAGE_SO_NAME
    )

    shutil.copy2(
        source,
        destination,
    )

    info(
        "Added: "
        f"{PACKAGE_LIB_DIR.as_posix()}/"
        f"{PACKAGE_SO_NAME}"
    )

    return destination


# ============================================================
# LeviPack creation
# ============================================================

def create_package(staging):
    section("Creating LeviPack")

    DIST_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    if PACKAGE_PATH.exists():
        info(
            f"Removing old package: "
            f"{PACKAGE_PATH}"
        )

        PACKAGE_PATH.unlink()

    with ZipFile(
        PACKAGE_PATH,
        mode="w",
        compression=ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:

        files = sorted(
            p for p in staging.rglob("*")
            if p.is_file()
        )

        if not files:
            fail(
                "The package staging directory is empty."
            )

        for file_path in files:
            relative_path = (
                file_path
                .relative_to(staging)
                .as_posix()
            )

            info(
                f"  + {relative_path}"
            )

            archive.write(
                file_path,
                arcname=relative_path,
            )

    info()
    info(
        f"Created: {PACKAGE_PATH}"
    )


# ============================================================
# Package verification
# ============================================================

def verify_package():
    section("Verifying LeviPack")

    if not PACKAGE_PATH.is_file():
        fail(
            "LeviPack was not created:\n"
            f"{PACKAGE_PATH}"
        )

    with ZipFile(
        PACKAGE_PATH,
        "r",
    ) as archive:

        bad_file = archive.testzip()

        if bad_file is not None:
            fail(
                "ZIP integrity check failed at:\n"
                f"{bad_file}"
            )

        names = archive.namelist()

    info("Package contents:")

    for name in names:
        info(f"  {name}")

    required = [
        "manifest.json",
        (
            f"{PACKAGE_LIB_DIR.as_posix()}/"
            f"{PACKAGE_SO_NAME}"
        ),
    ]

    info()

    for required_file in required:
        if required_file not in names:
            fail(
                "Required file is missing from LeviPack:\n"
                f"  {required_file}"
            )

    info("ZIP integrity: OK")
    info("Required files: OK")


# ============================================================
# Main
# ============================================================

def main():
    section("OutlineRGB LeviPack Builder")

    info(
        f"Project root : {ROOT}"
    )

    info(
        f"Build dir    : {BUILD_DIR}"
    )

    info(
        f"Output       : {PACKAGE_PATH}"
    )

    # --------------------------------------------------------
    # Manifest
    # --------------------------------------------------------

    manifest_path = find_manifest()

    manifest = load_manifest(
        manifest_path
    )

    validate_manifest(
        manifest
    )

    # --------------------------------------------------------
    # Native library
    # --------------------------------------------------------

    libraries = find_shared_libraries()

    selected_library = select_shared_library(
        libraries
    )

    info()
    info("Selected native library:")
    info(
        f"  {selected_library.relative_to(ROOT)}"
    )

    # --------------------------------------------------------
    # Staging
    # --------------------------------------------------------

    staging = prepare_staging_directory()

    copy_manifest(
        manifest_path,
        staging,
    )

    copy_shared_library(
        selected_library,
        staging,
    )

    # --------------------------------------------------------
    # Package
    # --------------------------------------------------------

    create_package(
        staging
    )

    # --------------------------------------------------------
    # Verify
    # --------------------------------------------------------

    verify_package()

    # --------------------------------------------------------
    # Cleanup
    # --------------------------------------------------------

    if staging.exists():
        shutil.rmtree(staging)

    section("BUILD SUCCESS")

    info(
        f"LeviPack:"
    )

    info(
        f"  {PACKAGE_PATH}"
    )


if __name__ == "__main__":
    main()
