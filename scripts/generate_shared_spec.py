#!/usr/bin/env python3
"""Generate the C++ shared-spec header from plc-comm-slmp-cross-verify."""

from __future__ import annotations

import argparse
import json
import os
import re
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parents[1]
DEFAULT_CROSS_VERIFY_DIR = PROJECT_DIR.parent / "plc-comm-slmp-cross-verify"
OUTPUT_PATH = PROJECT_DIR / "tests" / "generated_shared_spec.h"

HEX_DEVICE_CODES = {"X", "Y", "B", "W", "SB", "SW", "DX", "DY"}
BIT_DEVICE_CODES = {
    "SM",
    "X",
    "Y",
    "M",
    "L",
    "F",
    "V",
    "B",
    "TS",
    "TC",
    "LTS",
    "LTC",
    "STS",
    "STC",
    "LSTS",
    "LSTC",
    "CS",
    "CC",
    "LCS",
    "LCC",
    "SB",
    "DX",
    "DY",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--cross-verify-dir",
        default=os.environ.get("SLMP_CROSS_VERIFY_DIR", str(DEFAULT_CROSS_VERIFY_DIR)),
        help="Path to the plc-comm-slmp-cross-verify repository",
    )
    parser.add_argument(
        "--output",
        default=str(OUTPUT_PATH),
        help="Path to the generated C++ header",
    )
    return parser.parse_args()


def load_json(spec_dir: Path, name: str) -> dict:
    return json.loads((spec_dir / name).read_text(encoding="utf-8"))


def parse_device(text: str) -> tuple[str, int]:
    match = re.fullmatch(r"([A-Z]+)([0-9A-F]+)", text.strip().upper())
    if not match:
        raise ValueError(f"Unsupported device text for C++ generator: {text}")
    code, number_text = match.groups()
    base = 16 if code in HEX_DEVICE_CODES else 10
    return code, int(number_text, base)


def cpp_bool(value: bool) -> str:
    return "true" if value else "false"


def cpp_bytes(hex_text: str) -> list[str]:
    data = bytes.fromhex(hex_text)
    return [f"0x{byte:02X}" for byte in data]


def write_byte_array(lines: list[str], name: str, hex_text: str) -> str:
    values = cpp_bytes(hex_text)
    lines.append(f"static constexpr uint8_t {name}[] = {{")
    if values:
        for index in range(0, len(values), 12):
            chunk = ", ".join(values[index : index + 12])
            lines.append(f"    {chunk},")
    lines.append("};")
    lines.append("")
    return name


def build_header(spec_dir: Path) -> str:
    device_vectors = load_json(spec_dir, "device_spec_vectors.json")["vectors"]
    normalize_cases = load_json(spec_dir, "high_level_address_normalize_vectors.json")["cases"]
    cpp_parse_cases = load_json(spec_dir, "cpp_high_level_address_parse_vectors.json")["cases"]
    frame_vectors = load_json(spec_dir, "frame_golden_vectors.json")["cases"]

    lines: list[str] = []
    lines.append("#ifndef SLMP_TESTS_GENERATED_SHARED_SPEC_H")
    lines.append("#define SLMP_TESTS_GENERATED_SHARED_SPEC_H")
    lines.append("")
    lines.append("#include <stddef.h>")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("namespace shared_spec {")
    lines.append("")
    lines.append("namespace device_vectors {")
    lines.append("struct DeviceVector {")
    lines.append("    const char* id;")
    lines.append("    slmp::DeviceCode code;")
    lines.append("    uint32_t number;")
    lines.append("    slmp::CompatibilityMode mode;")
    lines.append("    bool bit_access;")
    lines.append("    const uint8_t* expected;")
    lines.append("    size_t expected_size;")
    lines.append("};")
    lines.append("")

    device_entries: list[str] = []
    for index, vector in enumerate(device_vectors):
        if "cpp" not in vector.get("implementations", []):
            continue
        code, number = parse_device(vector["device"])
        array_name = f"kDeviceVectorBytes{index}"
        write_byte_array(lines, array_name, vector["hex"])
        mode = "slmp::CompatibilityMode::iQR" if vector["series"] == "iqr" else "slmp::CompatibilityMode::Legacy"
        bit_access = code in BIT_DEVICE_CODES
        device_entries.append(
            "{"
            f"\"{vector['id']}\", "
            f"slmp::DeviceCode::{code}, "
            f"{number}U, "
            f"{mode}, "
            f"{cpp_bool(bit_access)}, "
            f"{array_name}, "
            f"sizeof({array_name})"
            "}"
        )
    lines.append("static constexpr DeviceVector kCases[] = {")
    for entry in device_entries:
        lines.append(f"    {entry},")
    lines.append("};")
    lines.append("")
    lines.append("}  // namespace device_vectors")
    lines.append("")

    lines.append("namespace normalize_cases {")
    lines.append("struct NormalizeCase {")
    lines.append("    const char* id;")
    lines.append("    const char* input;")
    lines.append("    const char* expected;")
    lines.append("};")
    lines.append("")
    lines.append("static constexpr NormalizeCase kCases[] = {")
    for case in normalize_cases:
        if "cpp" not in case.get("implementations", []):
            continue
        lines.append(f'    {{"{case["id"]}", "{case["input"]}", "{case["expected"]}"}},')
    lines.append("};")
    lines.append("")
    lines.append("}  // namespace normalize_cases")
    lines.append("")

    lines.append("namespace cpp_parse_cases {")
    lines.append("struct ParseCase {")
    lines.append("    const char* id;")
    lines.append("    const char* input;")
    lines.append("    slmp::Error expected_error;")
    lines.append("    slmp::DeviceCode code;")
    lines.append("    uint32_t number;")
    lines.append("    slmp::highlevel::ValueType value_type;")
    lines.append("    bool explicit_type;")
    lines.append("    int bit_index;")
    lines.append("    bool has_value_expectation;")
    lines.append("};")
    lines.append("")
    lines.append("static constexpr ParseCase kCases[] = {")
    for case in cpp_parse_cases:
        expected = case["expected"]
        has_value = expected["error"] == "Ok"
        code = expected.get("device_code", "D")
        number = expected.get("number", 0)
        value_type = expected.get("value_type", "U16")
        explicit_type = expected.get("explicit_type", False)
        bit_index = expected.get("bit_index", -1)
        lines.append(
            "    {"
            f"\"{case['id']}\", "
            f"\"{case['input']}\", "
            f"slmp::Error::{expected['error']}, "
            f"slmp::DeviceCode::{code}, "
            f"{number}U, "
            f"slmp::highlevel::ValueType::{value_type}, "
            f"{cpp_bool(explicit_type)}, "
            f"{bit_index}, "
            f"{cpp_bool(has_value)}"
            "},"
        )
    lines.append("};")
    lines.append("")
    lines.append("}  // namespace cpp_parse_cases")
    lines.append("")

    lines.append("namespace frame_vectors {")
    lines.append("struct FrameVector {")
    lines.append("    const char* id;")
    lines.append("    const char* operation;")
    lines.append("    const uint8_t* request;")
    lines.append("    size_t request_size;")
    lines.append("    const uint8_t* response_data;")
    lines.append("    size_t response_data_size;")
    lines.append("};")
    lines.append("")

    frame_entries: list[str] = []
    for index, case in enumerate(frame_vectors):
        if "cpp" not in case.get("implementations", []):
            continue
        request_name = f"kFrameRequest{index}"
        response_name = f"kFrameResponseData{index}"
        write_byte_array(lines, request_name, case["request_hex"])
        write_byte_array(lines, response_name, case.get("response_data_hex", ""))
        frame_entries.append(
            "{"
            f"\"{case['id']}\", "
            f"\"{case['operation']}\", "
            f"{request_name}, "
            f"sizeof({request_name}), "
            f"{response_name}, "
            f"sizeof({response_name})"
            "}"
        )
    lines.append("static constexpr FrameVector kCases[] = {")
    for entry in frame_entries:
        lines.append(f"    {entry},")
    lines.append("};")
    lines.append("")
    lines.append("}  // namespace frame_vectors")
    lines.append("")
    lines.append("}  // namespace shared_spec")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    cross_verify_dir = Path(args.cross_verify_dir).resolve()
    spec_dir = cross_verify_dir / "specs" / "shared"
    output_path = Path(args.output).resolve()

    required_files = (
        "device_spec_vectors.json",
        "high_level_address_normalize_vectors.json",
        "cpp_high_level_address_parse_vectors.json",
        "frame_golden_vectors.json",
    )
    missing = [name for name in required_files if not (spec_dir / name).exists()]
    if missing:
        joined = ", ".join(missing)
        raise SystemExit(
            f"Missing shared spec files under {spec_dir}: {joined}. "
            "Clone plc-comm-slmp-cross-verify next to this repo or pass --cross-verify-dir."
        )

    output_path.write_text(build_header(spec_dir), encoding="utf-8")
    print(f"[ok] generated {output_path}")
    print(f"[source] {spec_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
