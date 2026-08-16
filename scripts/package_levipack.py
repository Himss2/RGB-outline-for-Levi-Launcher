#!/usr/bin/env python3

import argparse
import json
import sys
import zipfile
from pathlib import Path


PACKAGE_LIBRARY_NAME = "libOutlineRGB.so"
PACKAGE_MANIFEST_NAME = "manifest.json"

SUPPORTED_VERSION_PREFIX = "1.26.4"


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
        data = json.loads(
            path.read_text(encoding="utf-8")
        )
    except json.JSONDecodeError as exc:
        fail(
            f"Invalid JSON in manifest: {exc}"
        )

    if not isinstance(data, dict):
        fail(
            "manifest.json must contain a JSON object."
        )

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
            "Manifest is missing required fields: "
            + ", ".join(missing)
        )

    if manifest["type"] != "preload-native":
        fail(
            'manifest "type" must be "preload-native".'
        )

    if manifest["entry"] != PACKAGE_LIBRARY_NAME:
        fail(
            'manifest "entry" must be '
            f'"{PACKAGE_LIBRARY_NAME}".'
        )

    versions = manifest.get(
        "minecraft_versions",
        []
    )

    if not isinstance(versions, list):
        fail(
            '"minecraft_versions" must be an array.'
        )

    if SUPPORTED_VERSION_PREFIX + "*" not in versions:
        fail(
            'manifest must contain '
            f'"{SUPPORTED_VERSION_PREFIX}*"'
            " in minecraft_versions."
        )


def normalize_manifest(manifest: dict) -> dict:
    result = dict(manifest)

    result["type"] = "preload-native"
    result["entry"] = PACKAGE_LIBRARY_NAME

    result["minecraft_versions"] = [
        SUPPORTED_VERSION_PREFIX + "*"
    ]

    result["overwrite_files"] = []
    result["overwrite_folders"] = []

    result["icon"] = ""

    return result


def find_library(path: Path) -> Path:
    if not path.is_file():
        fail(
            f"Shared library not found: {path}"
        )

    return path


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

    library = find_library(library)

    print("=" * 40)
    print("Checking manifest")
    print("=" * 40)

    manifest = load_manifest(manifest_path)
    manifest = normalize_manifest(manifest)

    validate_manifest(manifest)

    print("Manifest OK")
    print()
    print(
        json.dumps(
            manifest,
            indent=2,
            ensure_ascii=False,
        )
    )
    print()

    output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    if output.exists():
        output.unlink()

    manifest_bytes = (
        json.dumps(
            manifest,
            indent=2,
            ensure_ascii=False,
        )
        + "\n"
    ).encode("utf-8")

    print("=" * 40)
    print("Creating LeviPack")
    print("=" * 40)

    with zipfile.ZipFile(
        output,
        "w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:

        # IMPORTANT:
        # Match the reference LeviPack layout:
        #
        #   libOutlineRGB.so
        #   manifest.json
        #
        # Both files are located directly at ZIP root.

        archive.write(
            library,
            PACKAGE_LIBRARY_NAME,
        )

        archive.writestr(
            PACKAGE_MANIFEST_NAME,
            manifest_bytes,
        )

    print()
    print(f"Created: {output}")
    print()

    verify_package(
        output=output,
        library=library,
        manifest=manifest,
    )


def verify_package(
    output: Path,
    library: Path,
    manifest: dict,
) -> None:

    print("=" * 40)
    print("Verifying LeviPack")
    print("=" * 40)

    if not output.is_file():
        fail(
            f"LeviPack was not created: {output}"
        )

    with zipfile.ZipFile(
        output,
        "r",
    ) as archive:

        names = archive.namelist()

        # Expected order intentionally matches
        # the known working reference LeviPack.
        expected = [
            PACKAGE_LIBRARY_NAME,
            PACKAGE_MANIFEST_NAME,
        ]

        if names != expected:
            fail(
                "Unexpected LeviPack structure.\n"
                f"Expected: {expected}\n"
                f"Actual  : {names}"
            )

        embedded_manifest = json.loads(
            archive.read(
                PACKAGE_MANIFEST_NAME
            ).decode("utf-8")
        )

        if embedded_manifest != manifest:
            fail(
                "Embedded manifest.json does not "
                "match the normalized manifest."
            )

        library_info = archive.getinfo(
            PACKAGE_LIBRARY_NAME
        )

        if (
            library_info.file_size
            != library.stat().st_size
        ):
            fail(
                "Embedded libOutlineRGB.so size "
                "does not match the built library."
            )

        if (
            embedded_manifest.get("type")
            != "preload-native"
        ):
            fail(
                'Embedded manifest has invalid "type".'
            )

        if (
            embedded_manifest.get("entry")
            != PACKAGE_LIBRARY_NAME
        ):
            fail(
                'Embedded manifest has invalid "entry".'
            )

        versions = embedded_manifest.get(
            "minecraft_versions",
            [],
        )

        if (
            SUPPORTED_VERSION_PREFIX + "*"
            not in versions
        ):
            fail(
                "Embedded manifest does not declare "
                f"Minecraft "
                f"{SUPPORTED_VERSION_PREFIX}* "
                "compatibility."
            )

    print("ZIP structure OK")
    print("Manifest OK")
    print("Library size OK")
    print("Minecraft compatibility OK")
    print()
    print(
        "LeviPack verification PASSED"
    )
    print()


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Build OutlineRGB Android LeviPack."
        )
    )

    parser.add_argument(
        "--library",
        required=True,
        type=Path,
        help=(
            "Path to the built ARM64 "
            "shared library."
        ),
    )

    parser.add_argument(
        "--manifest",
        required=True,
        type=Path,
        help=(
            "Path to source manifest.json."
        ),
    )

    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help=(
            "Output .levipack path."
        ),
    )

    args = parser.parse_args()

    try:
        build_package(
            library=args.library.resolve(),
            manifest_path=args.manifest.resolve(),
            output=args.output.resolve(),
        )

    except SystemExit:
        raise

    except Exception as exc:
        print(
            f"Unexpected packaging error: {exc}",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
