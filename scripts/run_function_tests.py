#!/usr/bin/env python3
"""Build and run the host-side library function tests."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--cross-verify-dir", default="")
    parser.add_argument("--skip-unit", action="store_true")
    parser.add_argument("--skip-socket", action="store_true")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=0)
    parser.add_argument("--password", default="123456")
    parser.add_argument(
        "--scenario",
        choices=("all", "normal", "plc_error", "disconnect", "delay", "malformed"),
        default="all",
    )
    parser.add_argument("--verbose-server", action="store_true")
    return parser.parse_args()


def build_and_run_unit_tests(project_dir: Path, compiler: str) -> None:
    exe_name = "slmp_minimal_tests.exe" if os.name == "nt" else "slmp_minimal_tests"
    output_path = Path(tempfile.gettempdir()) / exe_name
    build_cmd = [
        compiler,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Isrc",
        "tests/slmp_minimal_tests.cpp",
        "src/slmp_minimal.cpp",
        "src/slmp_high_level.cpp",
        "-o",
        str(output_path),
    ]
    subprocess.check_call(build_cmd, cwd=project_dir)
    subprocess.check_call([str(output_path)], cwd=project_dir)


def build_example_smoke(project_dir: Path, compiler: str) -> None:
    exe_name = "slmp_high_level_example.exe" if os.name == "nt" else "slmp_high_level_example"
    output_path = Path(tempfile.gettempdir()) / exe_name
    build_cmd = [
        compiler,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Isrc",
        "examples/high_level_snapshot/high_level_snapshot.cpp",
        "src/slmp_minimal.cpp",
        "src/slmp_high_level.cpp",
        "-o",
        str(output_path),
    ]
    subprocess.check_call(build_cmd, cwd=project_dir)


def run_socket_integration(project_dir: Path, args: argparse.Namespace) -> None:
    cmd = [
        sys.executable,
        "scripts/run_socket_integration.py",
        "--compiler",
        args.compiler,
        "--host",
        args.host,
        "--port",
        str(args.port),
        "--password",
        args.password,
        "--scenario",
        args.scenario,
    ]
    if args.verbose_server:
        cmd.append("--verbose-server")
    subprocess.check_call(cmd, cwd=project_dir)


def main() -> int:
    args = parse_args()
    project_dir = Path(__file__).resolve().parents[1]

    generate_cmd = [sys.executable, "scripts/generate_shared_spec.py"]
    if args.cross_verify_dir:
        generate_cmd.extend(["--cross-verify-dir", args.cross_verify_dir])
    subprocess.check_call(generate_cmd, cwd=project_dir)

    if not args.skip_unit:
        build_and_run_unit_tests(project_dir, args.compiler)
        build_example_smoke(project_dir, args.compiler)
    if not args.skip_socket:
        run_socket_integration(project_dir, args)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
