#!/usr/bin/env python3
"""Synchronize release metadata that mirrors library.json."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def replace_required(pattern: str, replacement: str, text: str, path: Path) -> str:
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise SystemExit(f"Could not update expected metadata in {path}")
    return updated


def read_library_json_version() -> str:
    manifest = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
    version = str(manifest.get("version", "")).strip()
    if not re.match(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$", version):
        raise SystemExit(f"library.json has an invalid version: {version!r}")
    return version


def synced_files(version: str) -> dict[Path, str]:
    files: dict[Path, str] = {}

    library_properties = ROOT / "library.properties"
    text = library_properties.read_text(encoding="utf-8")
    files[library_properties] = replace_required(r"^version=.*$", f"version={version}", text, library_properties)

    version_header = ROOT / "include" / "mcprotocol" / "serial" / "version.hpp"
    if version_header.exists():
        major, minor, patch = re.match(r"^(\d+)\.(\d+)\.(\d+)", version).groups()
        text = version_header.read_text(encoding="utf-8")
        text = replace_required(r"^#define MCPROTOCOL_SERIAL_VERSION_MAJOR .*$", f"#define MCPROTOCOL_SERIAL_VERSION_MAJOR {major}", text, version_header)
        text = replace_required(r"^#define MCPROTOCOL_SERIAL_VERSION_MINOR .*$", f"#define MCPROTOCOL_SERIAL_VERSION_MINOR {minor}", text, version_header)
        text = replace_required(r"^#define MCPROTOCOL_SERIAL_VERSION_PATCH .*$", f"#define MCPROTOCOL_SERIAL_VERSION_PATCH {patch}", text, version_header)
        text = replace_required(r'^#define MCPROTOCOL_SERIAL_VERSION_STRING ".*"$', f'#define MCPROTOCOL_SERIAL_VERSION_STRING "{version}"', text, version_header)
        files[version_header] = text

    cmake_lists = ROOT / "CMakeLists.txt"
    if cmake_lists.exists():
        text = cmake_lists.read_text(encoding="utf-8")
        files[cmake_lists] = replace_required(
            r"^(project\([^\n]*\bVERSION\s+)([0-9]+(?:\.[0-9]+){1,2}(?:[-+][^\s)]*)?)(.*)$",
            rf"\g<1>{version}\g<3>",
            text,
            cmake_lists,
        )

    return files


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if mirrored release metadata is not synchronized")
    parser.add_argument("--expected-version", help="also require library.json to match this version")
    args = parser.parse_args()

    version = read_library_json_version()
    if args.expected_version and version != args.expected_version:
        raise SystemExit(f"library.json version {version} does not match expected release {args.expected_version}")

    changed: list[Path] = []
    for path, updated in synced_files(version).items():
        current = path.read_text(encoding="utf-8")
        if current != updated:
            changed.append(path)
            if not args.check:
                path.write_text(updated, encoding="utf-8", newline="\n")

    if changed and args.check:
        names = ", ".join(str(path.relative_to(ROOT)) for path in changed)
        raise SystemExit(f"Release metadata is out of sync with library.json {version}: {names}")

    if changed:
        for path in changed:
            print(f"synced {path.relative_to(ROOT)} to {version}")
    else:
        print(f"release metadata already synchronized at {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
