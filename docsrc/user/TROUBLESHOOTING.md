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

## X/Y/B/W device numbers look wrong

`B/W/SB/SW/DX/DY` use hexadecimal numbering in normal Mitsubishi notation.
High-level string `X/Y` also need one explicit `PlcProfile`:

- `slmp::highlevel::PlcProfile::IqF` uses octal string `X/Y`
- all other supported families use hexadecimal string `X/Y`

Examples:

- `X20` with `slmp::highlevel::PlcProfile::IqR` should be passed as `0x20`
- `Y217` with `slmp::highlevel::PlcProfile::IqF` should be passed as octal `0217`
- `D100` stays `100`

Typed helpers make this harder to mix up:

- `slmp::dev::Y(slmp::dev::hex(0x20))`
- `slmp::dev::D(slmp::dev::dec(100))`

## High-level address parsing fails

The optional high-level layer accepts:

- `D100`
- `D100:S`
- `D200:D`
- `D200:F`
- `D50.3`
- `M1000`

Common mistakes:

- `M1000.0`
- `X20.0`
- `Y1A.0`
- string `X/Y` without an explicit `PlcProfile`

Direct bit devices must be addressed directly. `.bit` notation is valid only for word devices such as `D50.3`.

If you want maximum control and no string parsing, use the low-level `DeviceAddress` API instead of the high-level helpers.

## `readTyped()` returns `InvalidArgument` for long families

`LTN`, `LSTN`, `LCN`, and `LZ` are 32-bit families in the high-level API.

Use:

- `LTN0:D` or `LTN0:L`
- `LSTN0:D` or `LSTN0:L`
- `LCN0:D` or `LCN0:L`
- `LZ0:D` or `LZ0:L`

Do not read these as plain 16-bit values.

## `DX` or `DY` fails on iQ-F

The profile-aware parser rejects `DX` and `DY` for `slmp::highlevel::PlcProfile::IqF`.

Use `X` and `Y` for iQ-F projects, and keep using the profile-aware overloads:
call `slmp::highlevel::readTyped(plc, slmp::highlevel::PlcProfile::IqF, "X20", value)` after configuring the same profile on the client.

## `G` or `HG` fails as a normal address

The low-level direct device path rejects `G` and `HG`.

Use module-buffer helpers instead:
call `plc.readWordsModuleBuf(3, false, 100, 4, words, 4)` with a caller-owned `uint16_t` output buffer.

## Connecting to GX Simulator 3 (GX Works3)

You can connect to the GX Simulator 3 using `127.0.0.1` and a calculated port number.

1.  **Start Simulation**: In GX Works3, go to `Debug` -> `Start Simulation` and ensure the simulator is in `RUN` state.
2.  **Calculate Port Number**:
    - The default port is `55` + `System Number` + `PLC Number`.
    - Example: System 1, PLC 1 => Port **`5511`**.
3. **Connection**: connect to `127.0.0.1` and the calculated port, for example `5511`.

Note: **GX Simulator 2** (GX Works2) uses a proprietary protocol and is **not compatible** with this SLMP library.

## Frame dump for debugging

Use:

- `lastRequestFrame()` / `lastRequestFrameLength()`
- `lastResponseFrame()` / `lastResponseFrameLength()`
- `formatHexBytes()`

For interactive frame-dump inspection, use the companion console-app repository:

- <https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal-console-app>

## High-level helpers are not available in my build

The optional helper layer in `slmp_high_level.h` can be compiled out.

Check whether your build defines `SLMP_MINIMAL_ENABLE_HIGH_LEVEL` as `0`. If it does, only the low-level `slmp_minimal.h` core API is available.
