# SLMP Protocol Specification (C++ Minimal)

Technical details of the SLMP implementation used in this library.

## 1. Frame Formats

### 4E Binary Frame (Default)
The library focuses on the **4E Binary Frame** due to its rich features and support for newer iQ-R/iQ-F series.

**Request Structure:**
1. Subheader (`54 00`)
2. Serial No.
3. Reserved
4. Network No.
5. Station No.
6. Module I/O No.
7. Multidrop Station No.
8. Data Length
9. Monitoring Timer
10. Command / Subcommand
11. Request Data

### 3E Binary Frame
Supported for compatibility with older Q-series PLCs. Enable via `setFrameType(FrameType::Frame3E)`.

## 2. Command Support

Currently implemented commands:
- `0101`: Read Type Name
- `0401`: Read Device (Word/Bit)
- `1401`: Write Device (Word/Bit)
- `0403`: Read Random
- `1402`: Write Random
- `0406`: Read Block
- `1406`: Write Block
- `1001/1002/1003/1005/1006`: Remote Run/Stop/Pause/Latch Clear/Reset
- `0619`: Self Test (loopback helper)
- `1617`: Clear Error
- `1630/1631`: Remote Password Unlock/Lock

Practical mixed block note:

- synchronous `writeBlock()` also exposes `BlockWriteOptions`
- `split_mixed_blocks=true` forces separate word-only and bit-only `1406` requests
- `retry_mixed_on_error=true` retries a failed mixed request as separate writes only on `0xC056`, `0xC05B`, or `0xC061`
- the async `beginWriteBlock(..., options, now_ms)` overload now mirrors the same split/retry behavior

Remote command note:

- `remoteReset(subcommand, expect_response)` defaults to the practical `1006/0000` no-response mode
- use `expect_response=true` only when the target and reset mode are expected to return a normal completion frame

Profile selection note:

- the current API does not ship automatic frame/profile probing
- callers must select `FrameType` and `CompatibilityMode` explicitly before `connect()`
- user-facing high-level helpers sit on top of that explicit transport/profile setup

## 3. Device Encoding

- **Decimal Devices (D, M, etc.)**: Encoded as standard numeric offsets.
- **Hexadecimal Devices (X, Y, B, W, etc.)**: Encoded using hex base.
- **Special Cases**:
  - `F` device is mapped to `slmp::DeviceCode::FDevice` to avoid conflicts with Arduino's `F()` macro.

## 4. Transport Abstraction

The library uses the `Slmp4eTransport` abstract class to decouple protocol logic from the physical network stack (Wi-Fi, Ethernet, etc.).
- `ArduinoClientTransport`: A concrete implementation for the standard Arduino `Client` class.
