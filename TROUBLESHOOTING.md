# Troubleshooting

## `connect()` fails

- Check the PLC IP address and TCP port first. The usual SLMP TCP port is `1025`, but the PLC project may use a different setting.
- Confirm the board can reach the PLC subnet.
- For local bring-up, verify the library against `scripts/mock_plc_server.py` before moving to real hardware.

## `TransportError`

`TransportError` means the TCP session failed before a valid SLMP response was parsed.

Common causes:

- Wi-Fi dropped
- Ethernet link is down
- PLC is not listening on the configured port
- the response timed out

What to check:

- use `ReconnectHelper` for retry logic
- inspect `lastRequestFrame()` and `lastResponseFrame()`
- increase `timeoutMs()` if the PLC is slow or the network is busy

## `PlcError` with end code `0x4013`

This usually means the session is still locked or the password was wrong.

Check:

- `remotePasswordUnlock()` was called before device access
- the password is `6..32` ASCII characters
- the PLC configuration expects the same password you are sending

## `BufferTooSmall`

This means the caller-owned buffers are too small for the selected command.

Starting points:

- `96` bytes for small direct reads and writes
- `128..192` bytes for larger direct writes
- `192..256` bytes for random and block access

If you use block write, random write, or long passwords, increase the TX buffer first.

## `ProtocolError`

This means bytes were received, but the response did not match the expected SLMP 4E binary shape.

Common causes:

- wrong PLC endpoint
- ASCII or non-4E endpoint on the PLC side
- another service is listening on the target port
- the response payload length does not match the requested point count

Use frame dump helpers to inspect the exact request and response bytes.

You can also reproduce this locally with:

- `python scripts\run_socket_integration.py --compiler g++ --scenario malformed`

## X/Y/B/W device numbers look wrong

These device families use hexadecimal numbering in normal Mitsubishi notation.

Examples:

- `X20` should be passed as `0x20`
- `Y1A` should be passed as `0x1A`
- `D100` stays `100`

Typed helpers make this harder to mix up:

- `slmp4e::dev::Y(slmp4e::dev::hex(0x20))`
- `slmp4e::dev::D(slmp4e::dev::dec(100))`

## Frame dump for debugging

Use:

- `lastRequestFrame()` / `lastRequestFrameLength()`
- `lastResponseFrame()` / `lastResponseFrameLength()`
- `formatHexBytes()`

The `examples/esp32_read_words` and `examples/esp32_password_read_loop` sketches show how to print the last request and response.

For desktop-only regression before flashing a board, use:

- `python scripts\run_socket_integration.py --compiler g++`
