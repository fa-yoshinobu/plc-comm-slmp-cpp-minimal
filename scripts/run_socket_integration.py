#!/usr/bin/env python3
"""Build and run the real-socket integration test against the local mock PLC."""

from __future__ import annotations

import argparse
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=0, help="0 means choose a free local port")
    parser.add_argument("--password", default="123456")
    parser.add_argument(
        "--scenario",
        choices=("all", "normal", "plc_error", "disconnect", "delay", "malformed"),
        default="all",
    )
    parser.add_argument("--verbose-server", action="store_true")
    return parser.parse_args()


def choose_free_port(host: str) -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((host, 0))
        return int(sock.getsockname()[1])


def wait_for_server(host: str, port: int, timeout_s: float) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return
        except OSError:
            time.sleep(0.1)
    raise RuntimeError(f"mock PLC did not accept connections on {host}:{port}")


def run_scenario(
    project_dir: Path,
    output_path: Path,
    host: str,
    port: int,
    scenario: str,
    password: str,
    verbose_server: bool,
) -> None:
    server_cmd = [
        sys.executable,
        "scripts/mock_plc_server.py",
        "--host",
        host,
        "--port",
        str(port),
        "--seed-demo",
    ]

    if scenario == "normal":
        server_cmd += ["--password", password]
    elif scenario == "plc_error":
        server_cmd += ["--inject-end-code", "0xC051", "--inject-command", "direct_read"]
    elif scenario == "disconnect":
        server_cmd += ["--disconnect-after-requests", "2"]
    elif scenario == "delay":
        server_cmd += ["--response-delay-ms", "250"]
    elif scenario == "malformed":
        server_cmd += ["--malformed-command", "read_type_name"]
    else:
        raise ValueError(f"unsupported scenario: {scenario}")

    if verbose_server:
        server_cmd.append("--verbose")

    server = subprocess.Popen(server_cmd, cwd=project_dir)
    try:
        wait_for_server(host, port, timeout_s=5.0)
        env = os.environ.copy()
        env["SLMP4E_TEST_HOST"] = host
        env["SLMP4E_TEST_PORT"] = str(port)
        env["SLMP4E_TEST_PASSWORD"] = password
        env["SLMP4E_TEST_SCENARIO"] = scenario
        subprocess.check_call([str(output_path)], cwd=project_dir, env=env)
    finally:
        server.terminate()
        try:
            server.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()


def main() -> int:
    args = parse_args()
    project_dir = Path(__file__).resolve().parents[1]
    port = args.port or choose_free_port(args.host)

    exe_name = "slmp4e_socket_integration.exe" if os.name == "nt" else "slmp4e_socket_integration"
    output_path = Path(tempfile.gettempdir()) / exe_name

    build_cmd = [
        args.compiler,
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Isrc",
        "tests/slmp4e_socket_integration.cpp",
        "src/slmp4e_minimal.cpp",
        "-o",
        str(output_path),
    ]
    if os.name == "nt":
        build_cmd.append("-lws2_32")

    subprocess.check_call(build_cmd, cwd=project_dir)

    scenarios = ["normal", "plc_error", "disconnect", "delay", "malformed"] if args.scenario == "all" else [args.scenario]
    for index, scenario in enumerate(scenarios):
        scenario_port = port + index if args.port != 0 else choose_free_port(args.host)
        run_scenario(project_dir, output_path, args.host, scenario_port, scenario, args.password, args.verbose_server)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
