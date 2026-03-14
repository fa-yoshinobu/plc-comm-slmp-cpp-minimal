#!/usr/bin/env python3
"""Minimal SLMP 4E binary mock PLC server for local bring-up and demo testing."""

from __future__ import annotations

import argparse
import json
import re
import socketserver
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path


REQUEST_SUBHEADER = b"\x54\x00"
RESPONSE_SUBHEADER = b"\xD4\x00"

CMD_READ_TYPE_NAME = 0x0101
CMD_DEVICE_READ = 0x0401
CMD_DEVICE_WRITE = 0x1401
CMD_DEVICE_READ_RANDOM = 0x0403
CMD_DEVICE_WRITE_RANDOM = 0x1402
CMD_DEVICE_READ_BLOCK = 0x0406
CMD_DEVICE_WRITE_BLOCK = 0x1406
CMD_REMOTE_PASSWORD_UNLOCK = 0x1630
CMD_REMOTE_PASSWORD_LOCK = 0x1631

SUB_WORD = 0x0002
SUB_BIT = 0x0003

DEVICE_RADIX = {
    "X": 16,
    "Y": 16,
    "B": 16,
    "W": 16,
    "SB": 16,
    "SW": 16,
    "DX": 16,
    "DY": 16,
}

DEVICE_NAME_TO_CODE = {
    "SM": 0x0091,
    "SD": 0x00A9,
    "X": 0x009C,
    "Y": 0x009D,
    "M": 0x0090,
    "L": 0x0092,
    "F": 0x0093,
    "V": 0x0094,
    "B": 0x00A0,
    "D": 0x00A8,
    "W": 0x00B4,
    "TS": 0x00C1,
    "TC": 0x00C0,
    "TN": 0x00C2,
    "STS": 0x00C7,
    "STC": 0x00C6,
    "STN": 0x00C8,
    "CS": 0x00C4,
    "CC": 0x00C3,
    "CN": 0x00C5,
    "SB": 0x00A1,
    "SW": 0x00B5,
    "DX": 0x00A2,
    "DY": 0x00A3,
    "R": 0x00AF,
    "ZR": 0x00B0,
}

DEVICE_CODE_TO_NAME = {value: key for key, value in DEVICE_NAME_TO_CODE.items()}
DEVICE_REGEX = re.compile(r"([A-Z]+)([0-9A-F]+)")
COMMAND_NAME_TO_VALUE = {
    "any": None,
    "read_type_name": CMD_READ_TYPE_NAME,
    "direct_read": CMD_DEVICE_READ,
    "direct_write": CMD_DEVICE_WRITE,
    "random_read": CMD_DEVICE_READ_RANDOM,
    "random_write": CMD_DEVICE_WRITE_RANDOM,
    "block_read": CMD_DEVICE_READ_BLOCK,
    "block_write": CMD_DEVICE_WRITE_BLOCK,
    "unlock": CMD_REMOTE_PASSWORD_UNLOCK,
    "lock": CMD_REMOTE_PASSWORD_LOCK,
}


def read_u16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 2], "little")


def read_u32(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "little")


def write_u16(value: int) -> bytes:
    return int(value).to_bytes(2, "little", signed=False)


def write_u32(value: int) -> bytes:
    return int(value).to_bytes(4, "little", signed=False)


def parse_device_text(text: str) -> tuple[str, int]:
    match = DEVICE_REGEX.fullmatch(text.strip().upper())
    if not match:
        raise ValueError(f"invalid device: {text}")
    code, number_text = match.groups()
    base = DEVICE_RADIX.get(code, 10)
    return code, int(number_text, base)


def pack_bit_pairs(values: list[bool]) -> bytes:
    out = bytearray()
    for index in range(0, len(values), 2):
        high = 0x10 if values[index] else 0x00
        low = 0x01 if index + 1 < len(values) and values[index + 1] else 0x00
        out.append(high | low)
    return bytes(out)


def unpack_bit_pairs(data: bytes, count: int) -> list[bool]:
    out: list[bool] = []
    for value in data:
        out.append(((value >> 4) & 0x1) != 0)
        if len(out) >= count:
            break
        out.append((value & 0x1) != 0)
        if len(out) >= count:
            break
    return out


@dataclass
class PlcState:
    model: str = "Q03UDVCPU"
    model_code: int = 0x1234
    password: str = ""
    word_memory: dict[tuple[str, int], int] = field(default_factory=dict)
    bit_memory: dict[tuple[str, int], bool] = field(default_factory=dict)
    lock: threading.Lock = field(default_factory=threading.Lock)

    def load_json(self, path: Path) -> None:
        data = json.loads(path.read_text(encoding="utf-8"))
        self.model = str(data.get("model", self.model))
        self.model_code = int(data.get("model_code", self.model_code), 0) if isinstance(data.get("model_code"), str) else int(data.get("model_code", self.model_code))
        self.password = str(data.get("password", self.password))

        for name, value in data.get("words", {}).items():
            code, number = parse_device_text(name)
            self.word_memory[(code, number)] = int(value) & 0xFFFF

        for name, value in data.get("bits", {}).items():
            code, number = parse_device_text(name)
            self.bit_memory[(code, number)] = bool(value)

    def seed_demo(self) -> None:
        self.word_memory[("D", 100)] = 1234
        self.word_memory[("D", 101)] = 5678
        self.write_dword(("D", 200), 0x12345678)
        self.word_memory[("D", 300)] = 0x1234
        self.word_memory[("D", 301)] = 0x5678
        self.bit_memory[("M", 100)] = True
        self.bit_memory[("M", 102)] = True
        self.bit_memory[("M", 200)] = True
        self.bit_memory[("M", 202)] = True
        self.bit_memory[("Y", 0x20)] = False

    def read_word(self, device: tuple[str, int]) -> int:
        return int(self.word_memory.get(device, 0)) & 0xFFFF

    def write_word(self, device: tuple[str, int], value: int) -> None:
        self.word_memory[device] = int(value) & 0xFFFF

    def read_dword(self, device: tuple[str, int]) -> int:
        code, number = device
        low = self.read_word((code, number))
        high = self.read_word((code, number + 1))
        return low | (high << 16)

    def write_dword(self, device: tuple[str, int], value: int) -> None:
        code, number = device
        self.write_word((code, number), value & 0xFFFF)
        self.write_word((code, number + 1), (value >> 16) & 0xFFFF)

    def read_bit(self, device: tuple[str, int]) -> bool:
        return bool(self.bit_memory.get(device, False))

    def write_bit(self, device: tuple[str, int], value: bool) -> None:
        self.bit_memory[device] = bool(value)

    def read_bit_block_word(self, device: tuple[str, int]) -> int:
        code, number = device
        value = 0
        for bit in range(16):
            if self.read_bit((code, number + bit)):
                value |= 1 << bit
        return value

    def write_bit_block_word(self, device: tuple[str, int], value: int) -> None:
        code, number = device
        for bit in range(16):
            self.write_bit((code, number + bit), ((value >> bit) & 0x1) != 0)


@dataclass
class RequestFrame:
    serial: int
    target: bytes
    monitoring_timer: int
    command: int
    subcommand: int
    payload: bytes


@dataclass
class FaultConfig:
    response_delay_ms: int = 0
    inject_end_code: int | None = None
    inject_command: int | None = None
    inject_once: bool = True
    malformed_command: int | None = None
    malformed_once: bool = True
    disconnect_after_requests: int = 0
    lock: threading.Lock = field(default_factory=threading.Lock)
    request_count: int = 0
    inject_used: bool = False
    malformed_used: bool = False

    def next_request_index(self) -> int:
        with self.lock:
            self.request_count += 1
            return self.request_count

    def should_disconnect(self, request_index: int) -> bool:
        return self.disconnect_after_requests > 0 and request_index >= self.disconnect_after_requests

    def match_inject(self, command: int) -> int | None:
        if self.inject_end_code is None:
            return None
        if self.inject_command is not None and self.inject_command != command:
            return None
        with self.lock:
            if self.inject_once and self.inject_used:
                return None
            self.inject_used = True
            return self.inject_end_code

    def match_malformed(self, command: int) -> bool:
        if self.malformed_command is None:
            return False
        if self.malformed_command is not None and self.malformed_command != command:
            return False
        with self.lock:
            if self.malformed_once and self.malformed_used:
                return False
            self.malformed_used = True
            return True


def decode_request(frame: bytes) -> RequestFrame:
    if len(frame) < 19 or frame[:2] != REQUEST_SUBHEADER:
        raise ValueError("invalid 4E request")
    serial = read_u16(frame, 2)
    target = frame[6:11]
    data_length = read_u16(frame, 11)
    if len(frame) != 13 + data_length or data_length < 6:
        raise ValueError("request length mismatch")
    return RequestFrame(
        serial=serial,
        target=target,
        monitoring_timer=read_u16(frame, 13),
        command=read_u16(frame, 15),
        subcommand=read_u16(frame, 17),
        payload=frame[19:],
    )


def encode_response(req: RequestFrame, end_code: int, payload: bytes = b"") -> bytes:
    body = write_u16(end_code) + payload
    return RESPONSE_SUBHEADER + write_u16(req.serial) + b"\x00\x00" + req.target + write_u16(len(body)) + body


def decode_device_spec(payload: bytes, offset: int) -> tuple[tuple[str, int], int]:
    number = read_u32(payload, offset)
    code = read_u16(payload, offset + 4)
    name = DEVICE_CODE_TO_NAME.get(code)
    if name is None:
        raise KeyError(f"unsupported device code: 0x{code:04X}")
    return (name, number), offset + 6


class MockPlcHandler(socketserver.BaseRequestHandler):
    server: "MockPlcServer"

    def setup(self) -> None:
        self.unlocked = self.server.state.password == ""

    def handle(self) -> None:
        while True:
            header = self._recv_exact(13)
            if not header:
                return
            if header[:2] != REQUEST_SUBHEADER:
                return
            data_length = read_u16(header, 11)
            rest = self._recv_exact(data_length)
            if rest is None:
                return
            frame = header + rest
            try:
                req = decode_request(frame)
                request_index = self.server.faults.next_request_index()
                response = self.dispatch(req, request_index)
            except Exception:
                # generic rejected path
                req = RequestFrame(serial=read_u16(frame, 2), target=frame[6:11], monitoring_timer=0, command=0, subcommand=0, payload=b"")
                response = encode_response(req, 0xC059)
            if response is None:
                return
            try:
                self.request.sendall(response)
            except OSError:
                return

    def _recv_exact(self, size: int) -> bytes | None:
        chunks = bytearray()
        while len(chunks) < size:
            try:
                data = self.request.recv(size - len(chunks))
            except OSError:
                return None
            if not data:
                return None if not chunks else bytes(chunks)
            chunks += data
        return bytes(chunks)

    def dispatch(self, req: RequestFrame, request_index: int) -> bytes | None:
        if self.server.verbose:
            print(f"mock-plc: req={request_index} cmd=0x{req.command:04X} sub=0x{req.subcommand:04X} len={len(req.payload)}")

        if self.server.faults.response_delay_ms > 0:
            time.sleep(self.server.faults.response_delay_ms / 1000.0)

        if self.server.faults.should_disconnect(request_index):
            if self.server.verbose:
                print(f"mock-plc: disconnecting on request {request_index}")
            self.request.close()
            return None

        injected_end_code = self.server.faults.match_inject(req.command)
        if injected_end_code is not None:
            if self.server.verbose:
                print(f"mock-plc: injecting end code 0x{injected_end_code:04X}")
            return encode_response(req, injected_end_code)

        if self.server.faults.match_malformed(req.command):
            if self.server.verbose:
                print("mock-plc: sending malformed response")
            response = bytearray(encode_response(req, 0x0000))
            response[0] = 0x00
            response[1] = 0x00
            return bytes(response)

        if self.server.state.password and not self.unlocked and req.command not in {CMD_REMOTE_PASSWORD_UNLOCK, CMD_READ_TYPE_NAME}:
            return encode_response(req, 0x4013)

        with self.server.state.lock:
            if req.command == CMD_READ_TYPE_NAME:
                model = self.server.state.model.encode("ascii", errors="ignore")[:16].ljust(16, b" ")
                payload = model + write_u16(self.server.state.model_code)
                return encode_response(req, 0x0000, payload)
            if req.command == CMD_DEVICE_READ:
                return self.handle_device_read(req)
            if req.command == CMD_DEVICE_WRITE:
                return self.handle_device_write(req)
            if req.command == CMD_DEVICE_READ_RANDOM:
                return self.handle_read_random(req)
            if req.command == CMD_DEVICE_WRITE_RANDOM:
                return self.handle_write_random(req)
            if req.command == CMD_DEVICE_READ_BLOCK:
                return self.handle_read_block(req)
            if req.command == CMD_DEVICE_WRITE_BLOCK:
                return self.handle_write_block(req)
            if req.command == CMD_REMOTE_PASSWORD_UNLOCK:
                return self.handle_password(req, unlock=True)
            if req.command == CMD_REMOTE_PASSWORD_LOCK:
                return self.handle_password(req, unlock=False)
        return encode_response(req, 0xC059)

    def handle_device_read(self, req: RequestFrame) -> bytes:
        device, offset = decode_device_spec(req.payload, 0)
        points = read_u16(req.payload, offset)
        if req.subcommand == SUB_WORD:
            payload = bytearray()
            code, number = device
            for index in range(points):
                payload += write_u16(self.server.state.read_word((code, number + index)))
            return encode_response(req, 0x0000, bytes(payload))
        if req.subcommand == SUB_BIT:
            code, number = device
            values = [self.server.state.read_bit((code, number + index)) for index in range(points)]
            return encode_response(req, 0x0000, pack_bit_pairs(values))
        return encode_response(req, 0xC059)

    def handle_device_write(self, req: RequestFrame) -> bytes:
        device, offset = decode_device_spec(req.payload, 0)
        points = read_u16(req.payload, offset)
        offset += 2
        code, number = device
        if req.subcommand == SUB_WORD:
            for index in range(points):
                self.server.state.write_word((code, number + index), read_u16(req.payload, offset + (index * 2)))
            return encode_response(req, 0x0000)
        if req.subcommand == SUB_BIT:
            states = unpack_bit_pairs(req.payload[offset:], points)
            for index, state in enumerate(states):
                self.server.state.write_bit((code, number + index), state)
            return encode_response(req, 0x0000)
        return encode_response(req, 0xC059)

    def handle_read_random(self, req: RequestFrame) -> bytes:
        word_count = req.payload[0]
        dword_count = req.payload[1]
        offset = 2
        payload = bytearray()
        for _ in range(word_count):
            device, offset = decode_device_spec(req.payload, offset)
            payload += write_u16(self.server.state.read_word(device))
        for _ in range(dword_count):
            device, offset = decode_device_spec(req.payload, offset)
            payload += write_u32(self.server.state.read_dword(device))
        return encode_response(req, 0x0000, bytes(payload))

    def handle_write_random(self, req: RequestFrame) -> bytes:
        if req.subcommand == SUB_WORD:
            word_count = req.payload[0]
            dword_count = req.payload[1]
            offset = 2
            for _ in range(word_count):
                device, offset = decode_device_spec(req.payload, offset)
                self.server.state.write_word(device, read_u16(req.payload, offset))
                offset += 2
            for _ in range(dword_count):
                device, offset = decode_device_spec(req.payload, offset)
                self.server.state.write_dword(device, read_u32(req.payload, offset))
                offset += 4
            return encode_response(req, 0x0000)

        if req.subcommand == SUB_BIT:
            bit_count = req.payload[0]
            offset = 1
            for _ in range(bit_count):
                device, offset = decode_device_spec(req.payload, offset)
                self.server.state.write_bit(device, read_u16(req.payload, offset) != 0)
                offset += 2
            return encode_response(req, 0x0000)

        return encode_response(req, 0xC059)

    def handle_read_block(self, req: RequestFrame) -> bytes:
        word_block_count = req.payload[0]
        bit_block_count = req.payload[1]
        offset = 2
        word_blocks: list[tuple[tuple[str, int], int]] = []
        bit_blocks: list[tuple[tuple[str, int], int]] = []
        for _ in range(word_block_count):
            device, offset = decode_device_spec(req.payload, offset)
            points = read_u16(req.payload, offset)
            offset += 2
            word_blocks.append((device, points))
        for _ in range(bit_block_count):
            device, offset = decode_device_spec(req.payload, offset)
            points = read_u16(req.payload, offset)
            offset += 2
            bit_blocks.append((device, points))

        payload = bytearray()
        for (code, number), points in word_blocks:
            for index in range(points):
                payload += write_u16(self.server.state.read_word((code, number + index)))
        for device, points in bit_blocks:
            code, number = device
            for index in range(points):
                payload += write_u16(self.server.state.read_bit_block_word((code, number + (index * 16))))
        return encode_response(req, 0x0000, bytes(payload))

    def handle_write_block(self, req: RequestFrame) -> bytes:
        word_block_count = req.payload[0]
        bit_block_count = req.payload[1]
        offset = 2
        word_blocks: list[tuple[tuple[str, int], int]] = []
        bit_blocks: list[tuple[tuple[str, int], int]] = []
        for _ in range(word_block_count):
            device, offset = decode_device_spec(req.payload, offset)
            points = read_u16(req.payload, offset)
            offset += 2
            word_blocks.append((device, points))
        for _ in range(bit_block_count):
            device, offset = decode_device_spec(req.payload, offset)
            points = read_u16(req.payload, offset)
            offset += 2
            bit_blocks.append((device, points))

        for (code, number), points in word_blocks:
            for index in range(points):
                self.server.state.write_word((code, number + index), read_u16(req.payload, offset))
                offset += 2
        for device, points in bit_blocks:
            code, number = device
            for index in range(points):
                self.server.state.write_bit_block_word((code, number + (index * 16)), read_u16(req.payload, offset))
                offset += 2
        return encode_response(req, 0x0000)

    def handle_password(self, req: RequestFrame, *, unlock: bool) -> bytes:
        if len(req.payload) < 2:
            return encode_response(req, 0xC051)
        length = read_u16(req.payload, 0)
        text = req.payload[2 : 2 + length].decode("ascii", errors="ignore")
        if self.server.state.password and text != self.server.state.password:
            return encode_response(req, 0x4013)
        self.unlocked = unlock
        return encode_response(req, 0x0000)


class MockPlcServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True

    def __init__(self, server_address: tuple[str, int], state: PlcState, faults: FaultConfig, verbose: bool) -> None:
        super().__init__(server_address, MockPlcHandler)
        self.state = state
        self.faults = faults
        self.verbose = verbose


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Minimal SLMP 4E mock PLC server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1025)
    parser.add_argument("--model", default="Q03UDVCPU")
    parser.add_argument("--model-code", default="0x1234")
    parser.add_argument("--password", default="")
    parser.add_argument("--state", type=Path)
    parser.add_argument("--seed-demo", action="store_true")
    parser.add_argument("--response-delay-ms", type=int, default=0)
    parser.add_argument("--inject-end-code")
    parser.add_argument("--inject-command", choices=tuple(COMMAND_NAME_TO_VALUE.keys()), default="any")
    parser.add_argument("--inject-repeat", action="store_true")
    parser.add_argument("--malformed-command", choices=tuple(COMMAND_NAME_TO_VALUE.keys()))
    parser.add_argument("--malformed-repeat", action="store_true")
    parser.add_argument("--disconnect-after-requests", type=int, default=0)
    parser.add_argument("--verbose", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    state = PlcState(model=args.model, model_code=int(args.model_code, 0), password=args.password)
    if args.state is not None:
        state.load_json(args.state)
    if args.seed_demo:
        state.seed_demo()

    faults = FaultConfig(
        response_delay_ms=max(0, args.response_delay_ms),
        inject_end_code=int(args.inject_end_code, 0) if args.inject_end_code else None,
        inject_command=COMMAND_NAME_TO_VALUE[args.inject_command],
        inject_once=not args.inject_repeat,
        malformed_command=COMMAND_NAME_TO_VALUE[args.malformed_command] if args.malformed_command else None,
        malformed_once=not args.malformed_repeat,
        disconnect_after_requests=max(0, args.disconnect_after_requests),
    )

    server = MockPlcServer((args.host, args.port), state, faults, args.verbose)
    print(f"mock-plc: listening on {args.host}:{args.port}")
    print(f"mock-plc: model={state.model} code=0x{state.model_code:04X} password={'set' if state.password else 'none'}")
    if faults.response_delay_ms > 0:
        print(f"mock-plc: response_delay_ms={faults.response_delay_ms}")
    if faults.inject_end_code is not None:
        print(f"mock-plc: inject_end_code=0x{faults.inject_end_code:04X} inject_command={args.inject_command}")
    if args.malformed_command:
        print(f"mock-plc: malformed_command={args.malformed_command}")
    if faults.disconnect_after_requests > 0:
        print(f"mock-plc: disconnect_after_requests={faults.disconnect_after_requests}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
