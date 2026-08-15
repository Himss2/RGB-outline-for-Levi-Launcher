#!/usr/bin/env python3

from pathlib import Path
import sys
import zipfile


# ============================================================
# Configuration
# ============================================================

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_ROOT / "build"
DIST_DIR = PROJECT_ROOT / "dist"

LEVIPACK_NAME = "OutlineRGB.levipack"
LEVIPACK_PATH = DIST_DIR / LEVIPACK_NAME

TARGET_ABI = "arm64-v8a"


# ============================================================
# Helpers
# ============================================================

def fail(message: str) -> None:
    print(f"ERROR: {message}", file=sys.stderr)
    sys.exit(1)


def print_tree(path: Path) -> None:
    if not path.exists():
        print(f"{path}: does not exist")
        return

    print(f"\n=== {path} ===")

    for item in sorted(path.rglob("*")):
        if item.is_file():
            try:
                relative = item.relative_to(path)
            except ValueError:
                relative = item

            print(f"  {relative}")


def find_shared_library() -> Path:
    if not BUILD_DIR.exists():
        fail(f"build directory does not exist: {BUILD_DIR}")

    candidates = sorted(BUILD_DIR.rglob("*.so"))

    if not candidates:
        fail(
            "no shared library (*.so) was found inside build/\n"
            "Make sure the Xmake Android ARM64 build actually produced "
            "the native library."
        )

    print("\n=== Shared libraries found ===")

    for candidate in candidates:
        print(f"  {candidate.relative_to(PROJECT_ROOT)}")

    # Prefer a library whose filename starts with "lib".
    preferred = [
        candidate
        for candidate in candidates
        if candidate.name.startswith("lib")
    ]

    if preferred:
        # If there is more than one, use the first deterministic result.
        selected = preferred[0]
    else:
        selected = candidates[0]

    print(f"\nSelected library:")
    print(f"  {selected.relative_to(PROJECT_ROOT)}")

    return selected


def validate_elf(path: Path) -> None:
    if not path.exists():
        fail(f"selected library disappeared: {path}")

    if path.stat().st_size == 0:
        fail(f"selected library is empty: {path}")

    # Basic ELF magic check.
    with path.open("rb") as file:
        magic = file.read(4)

    if magic != b"\x7fELF":
        fail(
            f"{path.name} does not appear to be an ELF shared library "
            "(invalid ELF magic)"
        )

    print(f"ELF validation: OK ({path.name})")


def create_levipack(shared_library: Path) -> None:
    DIST_DIR.mkdir(parents=True, exist_ok=True)

    # Remove stale output from a previous run.
    if LEVIPACK_PATH.exists():
        print(f"Removing old package: {LEV​​IPACK_PATH}")
        LEVIPACK_PATH.unlink()

    # LeviPack is packaged as a ZIP-compatible container.
    internal_path = f"lib/{TARGET_ABI}/{shared_library.name}"

    print("\n=== Creating LeviPack ===")
    print(f"Output : {LEV​​IPACK_PATH}")
    print(f"Library: {shared_library}")
    print(f"Inside : {internal_path}")

    with zipfile.ZipFile(
        LEVIPACK_PATH,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
    ) as archive:
        archive.write(
            shared_library,
            arcname=internal_path,
        )

    if not LEVIPACK_PATH.exists():
        fail("LeviPack was not created")

    if LEVIPACK_PATH.stat().st_size == 0:
        fail("LeviPack was created but is empty")

    print(
        f"\nSuccessfully created "
        f"{LEV​​IPACK_PATH.relative_to(PROJECT_ROOT)}"
    )


def verify_levipack() -> None:
    print("\n=== Verifying LeviPack ===")

    if not LEVIPACK_PATH.exists():
        fail(
            f"expected LeviPack does not exist:\n"
            f"  {LEV​​IPACK_PATH}"
        )

    print(f"Package exists:")
    print(f"  {LEV​​IPACK_PATH.relative_to(PROJECT_ROOT)}")

    with zipfile.ZipFile(LEV​​IPACK_PATH, "r") as archive:
        entries = archive.namelist()

        if not entries:
            fail("LeviPack contains no files")

        print("\nPackage contents:")

        for entry in entries:
            info = archive.getinfo(entry)

            print(
                f"  {entry} "
                f"({info.file_size} bytes)"
            )

        expected_prefix = f"lib/{TARGET_ABI}/"

        libraries = [
            entry
            for entry in entries
            if entry.startswith(expected_prefix)
            and entry.endswith(".so")
        ]

        if not libraries:
            fail(
                "no ARM64 shared library was found inside the package.\n"
                f"Expected something under: {expected_prefix}"
            )

        print("\nARM64 library found:")
        for library in libraries:
            print(f"  {library}")

    print("\nLeviPack verification: OK")


# ============================================================
# Main
# ============================================================

def main() -> None:
    print("========================================")
    print("       RGB Outline LeviPack Builder")
    print("========================================")

    print(f"\nProject root : {PROJECT_ROOT}")
    print(f"Build dir    : {BUILD_DIR}")
    print(f"Output dir   : {DIST_DIR}")
    print(f"Package      : {LEV​​IPACK_NAME}")
    print(f"Target ABI   : {TARGET_ABI}")

    # Show build tree for debugging.
    print_tree(BUILD_DIR)

    # Find and validate native library.
    shared_library = find_shared_library()
    validate_elf(shared_library)

    # Build package.
    create_levipack(shared_library)

    # Verify package immediately.
    verify_levipack()

    print("\n========================================")
    print("              SUCCESS")
    print("========================================")
    print(
        f"\nFinal package:\n"
        f"  {LEV​​IPACK_PATH.relative_to(PROJECT_ROOT)}"
    )


if __name__ == "__main__":
    main()
