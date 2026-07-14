#!/usr/bin/env python3
"""Check the C++ device-range table against the canonical fixture JSON."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "slmp_high_level.cpp"
FIXTURE = ROOT / "tests" / "fixtures" / "slmp_device_range_rules.json"

PROFILE_ARRAYS = {
    "melsec:iq-r": "kIqRRangeRules",
    "melsec:iq-r:rj71en71": "kIqRRangeRules",
    "melsec:iq-l": "kIqLRangeRules",
    "melsec:mx-f": "kMxFRangeRules",
    "melsec:mx-r": "kMxRRangeRules",
    "melsec:mx-r:rj71en71": "kMxRRangeRules",
    "melsec:iq-f": "kIqFRangeRules",
    "melsec:qcpu": "kQCpuRangeRules",
    "melsec:qcpu:qj71e71-100": "kQCpuRangeRules",
    "melsec:lcpu": "kLCpuRangeRules",
    "melsec:lcpu:lj71e71-100": "kLCpuRangeRules",
    "melsec:qnu": "kQnURangeRules",
    "melsec:qnu:qj71e71-100": "kQnURangeRules",
    "melsec:qnudv": "kQnUDVRangeRules",
    "melsec:qnudv:qj71e71-100": "kQnUDVRangeRules",
}

KIND_FUNCTIONS = {
    "unsupported": "rangeUnsupported",
    "undefined": "rangeUndefined",
    "fixed": "rangeFixed",
    "word-register": "rangeWord",
    "dword-register": "rangeDWord",
    "word-register-clipped": "rangeWordClipped",
    "dword-register-clipped": "rangeDWordClipped",
}


def extract_array(source: str, name: str) -> str:
    marker = f"const DeviceRangeRuleSpec {name}[]"
    start = source.find(marker)
    if start < 0:
        raise AssertionError(f"missing array {name}")
    brace = source.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing opening brace for {name}")

    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : index]
    raise AssertionError(f"unterminated array {name}")


def parse_entries(array_body: str) -> dict[str, tuple[str, list[int]]]:
    entries: dict[str, tuple[str, list[int]]] = {}
    pattern = re.compile(r'\{\s*"([^"]+)"\s*,\s*(range\w+)\((.*)\)\s*\},')
    for line in array_body.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        item, function, args = match.groups()
        numbers = [int(value) for value in re.findall(r"\b(\d+)U?\b", args)]
        entries[item] = (function, numbers)
    return entries


def check_rule(profile: str, item: str, rule: dict[str, object], actual: tuple[str, list[int]]) -> list[str]:
    errors: list[str] = []
    kind = str(rule["kind"])
    expected_function = KIND_FUNCTIONS[kind]
    function, numbers = actual
    if function != expected_function:
        errors.append(f"{profile} {item}: expected {expected_function}, got {function}")
        return errors

    if kind in {"word-register", "dword-register", "word-register-clipped", "dword-register-clipped"}:
        expected_register = int(rule["register"])
        if not numbers or numbers[0] != expected_register:
            errors.append(f"{profile} {item}: expected register {expected_register}, got {numbers[:1]}")
    if kind in {"word-register-clipped", "dword-register-clipped"}:
        expected_clip = int(rule["clip_value"])
        if len(numbers) < 2 or numbers[1] != expected_clip:
            errors.append(f"{profile} {item}: expected clip {expected_clip}, got {numbers[1:2]}")
    if kind == "fixed":
        expected_fixed = int(rule["fixed_value"])
        if not numbers or numbers[0] != expected_fixed:
            errors.append(f"{profile} {item}: expected fixed {expected_fixed}, got {numbers[:1]}")
    return errors


def main() -> int:
    payload = json.loads(FIXTURE.read_text(encoding="utf-8"))
    source = SOURCE.read_text(encoding="utf-8")
    errors: list[str] = []

    for profile, profile_payload in payload["profiles"].items():
        array_name = PROFILE_ARRAYS.get(profile)
        if array_name is None:
            errors.append(f"{profile}: missing PROFILE_ARRAYS mapping")
            continue
        entries = parse_entries(extract_array(source, array_name))
        for item, rule in profile_payload["rules"].items():
            actual = entries.get(item)
            if actual is None:
                errors.append(f"{profile} {item}: missing rule")
                continue
            errors.extend(check_rule(profile, item, rule, actual))

    for profile in sorted(set(PROFILE_ARRAYS) - set(payload["profiles"])):
        errors.append(f"{profile}: stale PROFILE_ARRAYS mapping")

    if errors:
        print("device-range-catalog-parity-failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print("device-range-catalog-parity-ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
