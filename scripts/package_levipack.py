#!/usr/bin/env python3

from pathlib import Path
import json
import shutil
import sys
import zipfile


# ============================================================
# PROJECT
# ============================================================

ROOT = Path(__file__).resolve().parent.parent

BUILD_DIR = ROOT / "build"
DIST_DIR = ROOT / "dist"

OUTPUT = DIST_DIR / "OutlineRGB.levipack"


# ============================================================
# CONSTANTS
# ============================================================

PACKAGE_SO_PATH = "lib/arm64-v8a/libOutlineRGB.so"


# ============================================================
# HELPERS
# ============================================================

def header(title):
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
# MANIFEST
# ============================================================

def find_manifest():

    root_manifest = ROOT / "manifest.json"

    if root_manifest.is_file():
        return root_manifest

    manifests = sorted(
        p
        for p in ROOT.rglob("manifest.json")
        if p.is_file()
        and ".git" not in p.parts
        and "build" not in p.parts
        and "dist" not in p.parts
    )

    if not manifests:
        fail(
            "manifest.json was not found.\n\n"
            f"Repository root:\n{ROOT}"
        )

    if len(manifests) > 1:

        print("Multiple manifest.json files found:")

        for manifest in manifests:
            print(
                f"  {manifest.relative_to(ROOT)}"
            )

        print()

    return manifests[0]


def validate_manifest(path):

    try:

        with path.open(
            "r",
            encoding="utf-8"
        ) as file:

            manifest = json.load(file)

    except json.JSONDecodeError as error:

        fail(
            "manifest.json contains invalid JSON:\n"
            f"{error}"
        )

    except OSError as error:

        fail(
            "Unable to read manifest.json:\n"
            f"{error}"
        )

    if not isinstance(manifest, dict):

        fail(
            "manifest.json root must be a JSON object."
        )

    print(
        f"Manifest: {path.relative_to(ROOT)}"
    )

    if "name" in manifest:
        print(
            f"Name   : {manifest['name']}"
        )

    if "version" in manifest:
        print(
            f"Version: {manifest['version']}"
        )

    return manifest


# ============================================================
# NATIVE LIBRARY
# ============================================================

def find_native_library():

    if not BUILD_DIR.exists():

        fail(
            "Build directory does not exist:\n"
            f"{BUILD_DIR}"
        )

    # Case-insensitive search.
    libraries = sorted(
        p
        for p in BUILD_DIR.rglob("*.so")
        if p.is_file()
        and p.name.lower() == "liboutlinergb.so"
    )

    if libraries:
        return libraries[0]

    # Fallback for unusual target naming.
    libraries = sorted(
        p
        for p in BUILD_DIR.rglob("*.so")
        if p.is_file()
        and "outlinergb" in p.name.lower()
    )

    if libraries:
        return libraries[0]

    # Print everything useful before failing.
    all_libraries = sorted(
        p
        for p in BUILD_DIR.rglob("*.so")
        if p.is_file()
    )

    if all_libraries:

        available = "\n".join(
            f"  {p.relative_to(ROOT)}"
            for p in all_libraries
        )

        fail(
            "OutlineRGB library was not found.\n\n"
            "Available .so files:\n"
            f"{available}"
        )

    fail(
        "No .so files were found under build/."
    )


# ============================================================
# DIST
# ============================================================

def prepare_dist():

    DIST_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    if OUTPUT.exists():

        print(
            f"Removing old package:\n"
            f"  {OUTPUT}"
        )

        OUTPUT.unlink()


# ============================================================
# CREATE PACKAGE
# ============================================================

def create_package(
    manifest_path,
    library_path
):

    header(
        "Creating LeviPack"
    )

    prepare_dist()

    with zipfile.ZipFile(
        OUTPUT,
        mode="w",
        compression=zipfile.ZIP_DEFLATED
    ) as archive:

        # ----------------------------------------------------
        # MANIFEST
        # ----------------------------------------------------

        archive.write(
            manifest_path,
            arcname="manifest.json"
        )

        print(
            "Added:"
        )

        print(
            "  manifest.json"
        )

        # ----------------------------------------------------
        # LIBRARY
        # ----------------------------------------------------

        archive.write(
            library_path,
            arcname=PACKAGE_SO_PATH
        )

        print(
            "Added:"
        )

        print(
            f"  {PACKAGE_SO_PATH}"
        )


# ============================================================
# VERIFY
# ============================================================

def verify_package():

    header(
        "Verifying LeviPack"
    )

    if not OUTPUT.is_file():

        fail(
            "LeviPack was not created:\n"
            f"{OUTPUT}"
        )

    with zipfile.ZipFile(
        OUTPUT,
        mode="r"
    ) as archive:

        # ZIP integrity.
        bad_file = archive.testzip()

        if bad_file is not None:

            fail(
                "ZIP integrity check failed:\n"
                f"{bad_file}"
            )

        names = set(
            archive.namelist()
        )

        required = {
            "manifest.json",
            PACKAGE_SO_PATH,
        }

        missing = required - names

        if missing:

            fail(
                "Required package files are missing:\n"
                + "\n".join(
                    f"  {item}"
                    for item in sorted(missing)
                )
            )

        # ----------------------------------------------------
        # Manifest validation inside package.
        # ----------------------------------------------------

        try:

            manifest_data = archive.read(
                "manifest.json"
            )

            json.loads(
                manifest_data.decode("utf-8")
            )

        except Exception as error:

            fail(
                "Packaged manifest.json is invalid:\n"
                f"{error}"
            )

        # ----------------------------------------------------
        # Output
        # ----------------------------------------------------

        print(
            "Package contents:"
        )

        for name in sorted(names):

            print(
                f"  {name}"
            )

    print()
    print(
        "ZIP integrity : OK"
    )

    print(
        "Manifest      : OK"
    )

    print(
        "ARM64 library : OK"
    )


# ============================================================
# MAIN
# ============================================================

def main():

    header(
        "OutlineRGB LeviPack Builder"
    )

    print(
        f"Project root : {ROOT}"
    )

    print(
        f"Build dir    : {BUILD_DIR}"
    )

    print(
        f"Output       : {OUTPUT}"
    )

    # --------------------------------------------------------
    # Manifest
    # --------------------------------------------------------

    header(
        "Checking manifest"
    )

    manifest_path = find_manifest()

    validate_manifest(
        manifest_path
    )

    # --------------------------------------------------------
    # Native library
    # --------------------------------------------------------

    header(
        "Checking native library"
    )

    library_path = find_native_library()

    print(
        "Selected library:"
    )

    print(
        f"  {library_path.relative_to(ROOT)}"
    )

    # --------------------------------------------------------
    # Package
    # --------------------------------------------------------

    create_package(
        manifest_path,
        library_path
    )

    # --------------------------------------------------------
    # Verify
    # --------------------------------------------------------

    verify_package()

    # --------------------------------------------------------
    # Final
    # --------------------------------------------------------

    size = OUTPUT.stat().st_size

    header(
        "SUCCESS"
    )

    print(
        f"LeviPack : {OUTPUT}"
    )

    print(
        f"Size     : {size:,} bytes"
    )


if __name__ == "__main__":
    main()
