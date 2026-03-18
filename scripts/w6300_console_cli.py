#!/usr/bin/env python3
"""PC-side CLI for the W6300-EVB-Pico2 serial console.

This tool can:
- send arbitrary console commands from the PC
- run a built-in inspection sequence
- operate interactively as a simple serial terminal
"""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import serial
from serial.tools import list_ports


DEFAULT_BAUD = 115200
DEFAULT_OVERALL_TIMEOUT = 120.0
PROMPT = b"> "


@dataclass(frozen=True)
class CommandBatch:
    label: str
    commands: list[str]


AUTO_BATCHES: list[CommandBatch] = [
    CommandBatch(
        "baseline",
        [
            "help",
            "status",
            "transport list",
            "frame list",
            "connect",
            "type",
            "dump",
        ],
    ),
    CommandBatch(
        "mode sweep",
        [
            "transport tcp",
            "frame 4e",
            "type",
            "transport udp",
            "type",
            "frame 3e",
            "type",
            "frame 4e",
            "transport tcp",
        ],
    ),
    CommandBatch(
        "command coverage",
        [
            "target 0 255 0x03FF 0",
            "monitor 0",
            "timeout 2000",
            "row D100",
            "wow D120 1234",
            "rb M120",
            "wb M120 1",
            "rbits M100 4",
            "wbits M100 1 0 1 0",
            "rdw D200 2",
            "wdw D200 1 2",
            "rod D220",
            "wod D220 0x12345678",
            "rr 2 D100 D101 1 D200",
            "wrand 1 D120 123 1 D200 0x12345678",
            "wrandb 1 M120 1",
            "rblk 1 D300 2 1 M200 1",
            "wblk 1 D300 2 10 20 1 M200 1 0x0005",
            "funcheck list",
            "funcheck direct",
            "funcheck api",
            "txlimit calc",
            "txlimit probe",
            "txlimit sweep all",
            "txlimit sweep binary all",
            "bench list",
            "bench pair 10",
            "bench block 10",
            "pending",
            "reconnect status",
            "endurance status",
        ],
    ),
    CommandBatch(
        "cleanup",
        [
            "close",
        ],
    ),
]


def list_serial_ports() -> None:
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return
    for port in ports:
        print(f"{port.device}: {port.description}")


def read_until_prompt(ser: serial.Serial, overall_timeout: float) -> bytes:
    deadline = time.monotonic() + overall_timeout
    buffer = bytearray()
    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        chunk = ser.read(waiting if waiting > 0 else 1)
        if chunk:
            buffer.extend(chunk)
            if buffer.endswith(PROMPT):
                time.sleep(0.05)
                if ser.in_waiting == 0:
                    break
            continue
        time.sleep(0.02)
    return bytes(buffer)


def write_command(ser: serial.Serial, command: str, overall_timeout: float) -> bytes:
    ser.reset_input_buffer()
    ser.write(command.encode("utf-8") + b"\n")
    ser.flush()
    return read_until_prompt(ser, overall_timeout)


def print_output(command: str, output: bytes) -> None:
    text = output.decode("utf-8", errors="replace")
    if text:
        print(text, end="" if text.endswith("\n") else "\n")


def run_commands(
    ser: serial.Serial,
    commands: list[str],
    overall_timeout: float,
    echo_commands: bool,
) -> int:
    exit_code = 0
    for command in commands:
        if echo_commands:
            print(f"> {command}")
        output = write_command(ser, command, overall_timeout)
        print_output(command, output)
        lowered = output.decode("utf-8", errors="ignore").lower()
        if "unknown command" in lowered or "usage:" in lowered and command not in {"help", "?"}:
            exit_code = 1
        if "failed" in lowered:
            exit_code = 1
    return exit_code


def build_auto_commands(full: bool, password: str | None = None) -> list[str]:
    commands: list[str] = []
    for batch in AUTO_BATCHES:
        commands.extend(batch.commands)
        if not full and batch.label == "mode sweep":
            break
        if full and batch.label == "command coverage" and password:
            commands.extend([f"unlock {password}", f"lock {password}"])
    return commands


def interactive_shell(ser: serial.Serial, overall_timeout: float) -> int:
    print("Interactive mode. Type commands for the board console. Type 'quit' to exit.")
    while True:
        try:
            command = input("> ").strip()
        except EOFError:
            return 0
        if not command:
            continue
        if command.lower() in {"quit", "exit"}:
            return 0
        output = write_command(ser, command, overall_timeout)
        print_output(command, output)


def open_serial(port: str, baud: int, timeout: float) -> serial.Serial:
    return serial.Serial(port=port, baudrate=baud, timeout=timeout, write_timeout=timeout)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="W6300-EVB-Pico2 serial console CLI")
    parser.add_argument("--port", help="Serial port such as COM6 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--timeout", type=float, default=0.1, help="Serial read timeout in seconds")
    parser.add_argument("--overall-timeout", type=float, default=DEFAULT_OVERALL_TIMEOUT)
    parser.add_argument("--list-ports", action="store_true", help="List available serial ports and exit")
    parser.add_argument("-c", "--command", action="append", default=[], help="Send one console command")
    parser.add_argument("--command-file", type=Path, help="Send commands from a text file")
    parser.add_argument("--auto", action="store_true", help="Run the built-in inspection sequence")
    parser.add_argument("--auto-full", action="store_true", help="Run the built-in inspection sequence with extra coverage")
    parser.add_argument("--password", help="Password to use for unlock/lock during --auto-full")
    parser.add_argument("--interactive", action="store_true", help="Keep the session open for manual typing")
    parser.add_argument("--echo", action="store_true", help="Echo commands before their output")
    args = parser.parse_args(argv)

    if args.list_ports:
        list_serial_ports()
        return 0

    if not args.port:
        parser.error("--port is required unless --list-ports is used")

    commands: list[str] = []
    commands.extend(args.command)

    if args.command_file is not None:
        commands.extend(
            line.strip()
            for line in args.command_file.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        )

    if args.auto or args.auto_full:
        commands.extend(build_auto_commands(full=args.auto_full, password=args.password))

    try:
        with open_serial(args.port, args.baud, args.timeout) as ser:
            time.sleep(2.0)
            startup = read_until_prompt(ser, args.overall_timeout)
            if startup:
                print(startup.decode("utf-8", errors="replace"), end="" if startup.endswith(b"\n") else "\n")

            exit_code = 0
            if commands:
                exit_code = run_commands(ser, commands, args.overall_timeout, args.echo)

            if args.interactive or not commands:
                exit_code = max(exit_code, interactive_shell(ser, args.overall_timeout))
            return exit_code
    except serial.SerialException as exc:
        print(f"serial error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
