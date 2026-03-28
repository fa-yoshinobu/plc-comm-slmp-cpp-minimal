#!/usr/bin/env python3
"""Extract a version section from CHANGELOG.md for GitHub Releases."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--changelog", type=Path, required=True)
    parser.add_argument("--version", required=True, help="Version text such as 0.1.0 or v0.1.0")
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def extract_section(text: str, version: str) -> str:
    clean_version = version[1:] if version.startswith("v") else version
    pattern = re.compile(r"^##\s+(.+)$", re.MULTILINE)
    matches = list(pattern.finditer(text))
    sections: dict[str, str] = {}
    for index, match in enumerate(matches):
        title = match.group(1).strip()
        start = match.end()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        sections[title] = text[start:end].strip()

    for key in (clean_version, f"v{clean_version}", "Unreleased"):
        if key in sections:
            body = sections[key].strip()
            header = f"## {key}"
            return f"{header}\n\n{body}\n"
    raise KeyError(f"version section not found: {version}")


def main() -> int:
    args = parse_args()
    text = args.changelog.read_text(encoding="utf-8")
    body = extract_section(text, args.version)
    args.output.write_text(body, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
