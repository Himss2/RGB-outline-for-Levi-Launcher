#!/usr/bin/env python3

import argparse
import json
import sys
import zipfile
from pathlib import Path


LIBRARY_NAME = "libOutlineRGB.so"
MANIFEST_NAME = "manifest.json"

SUPPORTED_VERSION = "1.26.4*"


def fail(message: str) -> None:
    print()
    print("ERROR:")
    print(message)
    print()
    raise SystemExit(1)


def load_manifest(path: Path) -> dict:
    if not path.is_file():
        fail(f"Manifest not found: {path}")

    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except UnicodeDecodeError as exc:
        fail(f"Manifest is not valid UTF-8: {exc}")
    except json.JSONDecodeError as exc:
        fail(f"Invalid manifest JSON: {exc}")

    if not isinstance(data, dict):
        fail("manifest.json must contain a JSON object.")

    return data


def validate_manifest(manifest: dict) -> None:
    required = [
        "type",
        "name",
        "author",
        "version",
        "entry",
    ]

    missing = [
        key
        for key in required
        if key not in manifest
    ]

    if missing:
        fail(
            "Missing manifest fields: "
            + ", ".join(missing)
        )

    if manifest["type"] != "preload-native":
        fail(
            'manifest "type" must be '
            '"preload-native".'
        )

    if manifest["entry"] != LIBRARY_NAME:
        fail(
            'manifest "entry" must be '
            f'"{LIBRARY_NAME}".'
        )

    versions = manifest.get("minecraft_versions", [])

    if not isinstance(versions, list):
        fail(
            '"minecraft_versions" must be an array.'
        )

    if SUPPORTED_VERSION not in versions:
        fail(
            "manifest must contain "
            f'"{SUPPORTED_VERSION}" '
            "in minecraft_versions."
        )


def normalize_manifest(manifest: dict) -> dict:
    result = dict(manifest)

    result["type"] = "preload-native"
    result["entry"] = LIBRARY_NAME

    result["minecraft_versions"] = [
        SUPPORTED_VERSION
    ]

    result["icon"] = ""

    result["overwrite_files"] = []
    result["overwrite_folders"] = []

    return result


def make_manifest_bytes(manifest: dict) -> bytes:
    return (
        json.dumps(
            manifest,
            indent=2,
            ensure_ascii=False,
        )
        + "\n"
    ).encode("utf-8")


def build_package(
    library: Path,
    manifest_path: Path,
    output: Path,
) -> None:

    print("=" * 40)
    print("OutlineRGB LeviPack Builder")
    print("=" * 40)

    print()
    print(f"Library : {library}")
    print(f"Manifest: {manifest_path}")
    print(f"Output  : {output}")
    print()

    # --------------------------------------------------
    # Check native library
    # --------------------------------------------------

    if not library.is_file():
        fail(
            "Shared library not found:\n"
            f"{library}"
        )

    if library.stat().st_size == 0:
        fail(
            "Shared library is empty:\n"
            f"{library}"
        )

    # --------------------------------------------------
    # Load + normalize manifest
    # --------------------------------------------------

    manifest = load_manifest(manifest_path)

    manifest = normalize_manifest(manifest)

    validate_manifest(manifest)

    manifest_bytes = make_manifest_bytes(manifest)

    print("=" * 40)
    print("Manifest")
    print("=" * 40)

    print(
        json.dumps(
            manifest,
            indent=2,
            ensure_ascii=False,
        )
    )

    # --------------------------------------------------
    # Prepare output
    # --------------------------------------------------

    output.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    if output.exists():
        print()
        print(
            f"Removing old package: {output}"
        )
        output.unlink()

    # --------------------------------------------------
    # Create LeviPack
    #
    # IMPORTANT:
    #
    # Both files MUST be placed directly at
    # the root of the archive.
    #
    # No:
    #   assets/
    #   build/
    #   dist/
    #   lib/
    #
    # inside the LeviPack.
    # --------------------------------------------------

    print()
    print("=" * 40)
    print("Creating LeviPack")
    print("=" * 40)

    with zipfile.ZipFile(
        output,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:

        # Library first.
        archive.write(
            library,
            arcname=LIBRARY_NAME,
        )

        # Manifest second.
        archive.writestr(
            MANIFEST_NAME,
            manifest_bytes,
        )

    print()
    print(
        f"Created: {output}"
    )

    # --------------------------------------------------
    # Verify
    # --------------------------------------------------

    verify_package(
        output,
        manifest,
    )


def verify_package(
    output: Path,
    manifest: dict,
) -> None:

    print()
    print("=" * 40)
    print("Verifying LeviPack")
    print("=" * 40)

    if not output.is_file():
        fail(
            f"Package was not created:\n"
            f"{output}"
        )

    try:
        with zipfile.ZipFile(
            output,
            mode="r",
        ) as archive:

            # Check ZIP integrity first.
            bad_file = archive.testzip()

            if bad_file is not None:
                fail(
                    "Corrupted ZIP entry:\n"
                    f"{bad_file}"
                )

            names = archive.namelist()

            # --------------------------------------------------
            # We require exactly two files.
            # --------------------------------------------------

            expected_set = {
                LIBRARY_NAME,
                MANIFEST_NAME,
            }

            actual_set = set(names)

            if actual_set != expected_set:
                fail(
                    "Unexpected LeviPack structure.\n"
                    f"Expected files: "
                    f"{sorted(expected_set)}\n"
                    f"Actual files  : "
                    f"{sorted(actual_set)}"
                )

            if len(names) != 2:
                fail(
                    "LeviPack contains duplicate "
                    "or unexpected entries.\n"
                    f"Entries: {names}"
                )

            # --------------------------------------------------
            # Ensure files are at archive root.
            # --------------------------------------------------

            for name in names:
                if "/" in name or "\\" in name:
                    fail(
                        "LeviPack contains a nested path:\n"
                        f"{name}\n\n"
                        "Both libOutlineRGB.so and "
                        "manifest.json must be at the "
                        "archive root."
                    )

            # --------------------------------------------------
            # Read embedded manifest.
            # --------------------------------------------------

            try:
                embedded_bytes = archive.read(
                    MANIFEST_NAME
                )

                embedded = json.loads(
                    embedded_bytes.decode("utf-8")
                )

            except Exception as exc:
                fail(
                    "Unable to read embedded "
                    f"manifest.json: {exc}"
                )

            if embedded != manifest:
                fail(
                    "Embedded manifest does not "
                    "match normalized manifest."
                )

            # --------------------------------------------------
            # Check native library.
            # --------------------------------------------------

            library_info = archive.getinfo(
                LIBRARY_NAME
            )

            if library_info.file_size <= 0:
                fail(
                    "libOutlineRGB.so is empty."
                )

            # --------------------------------------------------
            # Check entry name.
            # --------------------------------------------------

            if manifest.get("entry") != LIBRARY_NAME:
                fail(
                    "Manifest entry does not point "
                    "to libOutlineRGB.so."
                )

    except zipfile.BadZipFile as exc:
        fail(
            f"Invalid LeviPack/ZIP archive: {exc}"
        )

    print()
    print("LeviPack structure: OK")

    print()
    print("Entries:")

    # Print in archive order for debugging.
    for name in names:
        print(f"  {name}")

    print()
    print("Manifest:")

    print(
        json.dumps(
            manifest,
            indent=2,
            ensure_ascii=False,
        )
    )

    print()
    print(
        "libOutlineRGB.so size: "
        f"{library_info.file_size} bytes"
    )

    print()
    print("Verification: OK")


def main() -> int:

    parser = argparse.ArgumentParser(
        description="Build OutlineRGB LeviPack"
    )

    parser.add_argument(
        "--library",
        required=True,
        type=Path,
        help="Path to libOutlineRGB.so",
    )

    parser.add_argument(
        "--manifest",
        required=True,
        type=Path,
        help="Path to manifest.json",
    )

    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Output .levipack path",
    )

    args = parser.parse_args()

    build_package(
        library=args.library,
        manifest_path=args.manifest,
        output=args.output,
    )

    return 0


if __name__ == "__main__":
    sys.exit(main())
