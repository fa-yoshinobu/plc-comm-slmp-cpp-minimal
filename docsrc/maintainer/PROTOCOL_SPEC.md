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

- `1406` Write Block payloads are encoded as block count, then each block's
  device spec, point count, and data immediately after that block. Grouping all
  specs first and all data last is invalid for mixed or multi-block writes.
- synchronous `writeBlock()` and asynchronous `beginWriteBlock()` each emit
  exactly one mixed `1406` request
- split options and automatic mixed-write retry are not part of the API; PLC
  end codes are returned unchanged

Resolved `1406` layout investigation:

- The old cross-library bug was the payload shape
  `[counts][all specs][all word data][all bit data]`.
- The manual-conformant shape is
  `[counts][spec + point count + data][spec + point count + data]...`.
- Single-block word-only and bit-only writes masked the bug because both
  layouts collapse to the same byte order for one block.
- Mixed word+bit and multi-block writes made PLCs parse later bytes as the
  wrong field, producing target-dependent end codes:
  - R08CPU built-in Ethernet: `0xC05B` mixed, `0xC051` multi-word
  - LJ71E71-100/L02SCPU and QJ71E71-100/Q06UDVCPU: `0xC056`
  - FX5UC-32MT/D on the documented 3E path: `0xC061`
- The fixed layout was re-verified live on R08CPU, L02SCPU via LJ71E71-100,
  Q06UDVCPU via QJ71E71-100, L16HCPU, and FX5UC-32MT/D. Fixed Python/Rust/.NET
  clients returned `0x0000` with readback match; C++ minimal and Node-RED pass
  the shared cross-verify block cases with the same layout.
- QnUDV built-in Ethernet still returns `0xC059` for block commands when the
  request is intentionally sent. That is a target/path command-support
  limitation, not a mixed-layout issue. The public client now keeps this
  measured result in the built-in capability table and returns
  `Error::ProfileFeatureBlocked` before transport while strict profile guards
  are enabled.

Remote command note:

- `remoteReset()` sends `1006/0000` with fixed reset data `01 00` and completes
  after request transmission without waiting for a response

Profile selection note:

- the current API does not ship automatic frame/profile probing
- callers must select `FrameType` and `CompatibilityMode` explicitly before `connect()`
- user-facing high-level helpers sit on top of that explicit transport/profile setup

## 3. Device Encoding

- **Decimal Devices (D, M, etc.)**: Encoded as standard numeric offsets.
- **Hexadecimal Devices (X, Y, B, W, etc.)**: Encoded using hex base.
- **Special Cases**:
  - `F` device is mapped to `slmp::DeviceCode::FDevice` to avoid conflicts with Arduino's `F()` macro.

### 3.1 iQ-R SD Device Range Maximums

For iQ-R-series device range catalogs, the profile-specific `SD` register value
is the PLC-configured current point count. Treat the manual maximum below as a
protocol-side cap and derive the displayed upper bound from the capped point
count:

1. read the configured point count from the row's `SD` register pair
2. calculate `point_count = min(SD point count, max_point_count)`
3. calculate `upper_bound = point_count - 1`

The iQ-R `0002/0003` device access format uses a 4-byte device number, so all
of these maximum addresses fit the iQ-R-series request payload. The minimal C++
library does not currently auto-build a device range catalog from these SD
values; this section is the reference for caller-side diagnostics and future
catalog support.

| Item | C++ device codes | Max address | max_point_count | Setting unit |
| --- | --- | --- | --- | --- |
| `X` | `X` | `X2FFF` | `12288` (`0x3000`) | n/a |
| `Y` | `Y` | `Y2FFF` | `12288` (`0x3000`) | n/a |
| `M` | `M` | `M94674943` | `94674944` (`0x5A4A000`) | 64 points |
| `B` | `B` | `B5A49FFF` | `94674944` (`0x5A4A000`) | 64 points |
| `F` | `FDevice` | `F32767` | `32768` | 64 points |
| `SB` | `SB` | `SB5A49FFF` | `94674944` (`0x5A4A000`) | 64 points |
| `V` | `V` | `V32767` | `32768` | 64 points |
| `L` | `L` | `L32767` | `32768` | 64 points |
| `T` | `TS`, `TC`, `TN` | `T5259711` | `5259712` | 32 points |
| `ST` | `STS`, `STC`, `STN` | `ST5259711` | `5259712` | 32 points |
| `LT` | `LTS`, `LTC`, `LTN` | `LT1479295` | `1479296` | 1 point |
| `LST` | `LSTS`, `LSTC`, `LSTN` | `LST1479295` | `1479296` | 1 point |
| `C` | `CS`, `CC`, `CN` | `C5259711` | `5259712` | 32 points |
| `LC` | `LCS`, `LCC`, `LCN` | `LC2784543` | `2784544` | 32 points |
| `D` | `D` | `D5917183` | `5917184` (`0x5A4A00`) | 4 points |
| `W` | `W` | `W5A49FF` | `5917184` (`0x5A4A00`) | 4 points |
| `SW` | `SW` | `SW5A49FF` | `5917184` (`0x5A4A00`) | 4 points |

## 4. Transport Abstraction

The library uses the `Slmp4eTransport` abstract class to decouple protocol logic from the physical network stack (Wi-Fi, Ethernet, etc.).
- `ArduinoClientTransport`: A concrete implementation for the standard Arduino `Client` class.
