[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-slmp-cpp-minimal/)
[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-slmp-cpp-minimal?label=release)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-slmp-cpp-minimal/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Static Analysis: PlatformIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/plus/pio-check.html)

# SLMP Protocol (Minimal C++)

![Illustration](https://raw.githubusercontent.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/main/docsrc/assets/melsec_rpi.png)

A lightweight, microcontroller-oriented C++ library for Mitsubishi SLMP (Seamless Message Protocol). The core client stays buffer-oriented and allocation-free, and an optional high-level layer adds string-address helpers for typed reads, mixed snapshots, and polling.

PlatformIO Registry:

- <https://registry.platformio.org/libraries/fa-yoshinobu/slmp-connect-cpp-minimal>

Install from PlatformIO Registry:

```ini
lib_deps =
  fa-yoshinobu/slmp-connect-cpp-minimal@^0.4.12
```

## Design Philosophy

This library deliberately exposes two layers.

- **Core low-level layer** in `slmp_minimal.h`
  - fixed caller-owned buffers
  - no heap allocation in the core path
  - direct control over sync and async request flow
  - intended for embedded firmware, RTOS tasks, and size-sensitive builds
- **Optional high-level layer** in `slmp_high_level.h`
  - string addresses such as `D100`, `D200:F`, and `D50.3`
  - typed helpers, named snapshots, and reusable polling plans
  - intended for application code, quick bring-up, and parity with the Python and .NET libraries

The goal is not to force one style. The goal is to let firmware stay predictable while still offering a user-friendly surface when convenience matters more than raw control.

## Key Features

- **Embedded Optimized**: No dynamic memory allocation (`malloc`/`new`). Predictable RAM usage.
- **Async Support**: Built-in state machine for non-blocking communication.
- **Optional High-Level API**: `readTyped`, `writeTyped`, `readNamed`, `writeNamed`, and `Poller`.
- **Broad Hardware Support**: Board-agnostic design for Wi-Fi and Ethernet (Arduino-compatible).
- **Binary 3E/4E**: Supports both modern and legacy SLMP frames.
- **CI-Ready**: Host-side G++ testing and size regression monitoring.
- **Cross-Verify Shared Vectors**: Host tests regenerate `tests/generated_shared_spec.h`
  from `plc-comm-slmp-cross-verify/specs/shared` so frame/address/device
  expectations stay aligned with the cross-library parity harness.
- **Maintained Samples**: Small low-level, high-level, and compile-checked host examples.

## Quick Start

### ESP32-DevKitC Size Comparison Samples

Two maintained ESP32 examples make the binary-size tradeoff visible on the same board:

- `examples/esp32_devkitc_low_level`
- `examples/esp32_devkitc_high_level`

Build them with:

```bash
pio run -e esp32-devkitc-low-level
pio run -e esp32-devkitc-high-level
pio run -e esp32-devkitc-low-level-no-udp
pio run -e esp32-devkitc-high-level-no-udp
```

Then compare:

- `.pio/build/esp32-devkitc-low-level/firmware.bin`
- `.pio/build/esp32-devkitc-high-level/firmware.bin`

Measured on the current reference build:

- low-level sample: Flash `749909` bytes, RAM `45064` bytes
- high-level sample: Flash `777525` bytes, RAM `45184` bytes
- high-level delta: Flash `+27616` bytes, RAM `+120` bytes
- disabling `SLMP_ENABLE_UDP_TRANSPORT` in these TCP-only ESP32 samples: Flash `+0` bytes, RAM `+0` bytes

The measurement log is stored in `docsrc/validation/reports/ESP32_DEVKITC_SIZE_COMPARISON.md`.

### Companion Console Applications

The interactive Atom Matrix and W6300 console programs were moved to the companion repository:

- <https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal-console-app>

This repository now stays focused on the library itself and the smallest maintained samples.

### High-Level Firmware Usage
```cpp
#include <slmp_high_level.h>
#include <slmp_arduino_transport.h>

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);

uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp::SlmpClient plc(transport, tx_buffer, 128, rx_buffer, 128);

void setup() {
    constexpr auto family = slmp::highlevel::PlcProfile::IqR;
    slmp::highlevel::configureClientForPlcProfile(plc, family);
    slmp::TypeNameInfo info = {};
    if (!plc.connect("192.168.250.100", 1025)) {
        return;
    }
    plc.readTypeName(info);
}

void loop() {
    slmp::highlevel::Snapshot snapshot;
    constexpr auto family = slmp::highlevel::PlcProfile::IqR;
    if (slmp::highlevel::readNamed(plc, family, {"SM400", "D100", "D200:F", "D50.3"}, snapshot) ==
        slmp::Error::Ok) {
        // snapshot[0] -> SM400
        // snapshot[1] -> D100
        // snapshot[2] -> D200:F
        // snapshot[3] -> D50.3
    }
}
```

### Core Low-Level Firmware Usage

```cpp
#include <slmp_arduino_transport.h>
#include <slmp_minimal.h>

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);

uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

void setup() {
    plc.setFrameType(slmp::FrameType::Frame4E);
    plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);
    plc.connect("192.168.250.100", 1025);
}

void loop() {
    uint16_t word = 0;
    plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), word);
}
```

### Recommended High-Level Flow

For application code, the recommended order is:

1. Create the fixed-buffer `slmp::SlmpClient`.
2. Set one explicit `PlcProfile` through `configureClientForPlcProfile(...)`.
3. Connect with `plc.connect(...)`.
4. Use `readTyped`, `writeTyped`, `readNamed`, `writeNamed`, and `Poller`.
5. Drop to `slmp_minimal.h` only when you need direct frame-level control, manual async state machines, or specialized embedded integration.

Automatic profile probing is intentionally not part of the current API surface. The high-level helper layer derives fixed `frame` and `compatibility mode` defaults from one explicit `PlcProfile`. Use the `PlcProfile` overloads of `readTyped`, `writeTyped`, `readNamed`, `writeNamed`, `Poller::compile`, `parseAddressSpec()`, `normalizeAddress()`, or `formatAddressSpec()` whenever you need deterministic string-address handling.

### High-Level Address Forms

The optional helper layer accepts the same user-facing address forms that the Python and .NET libraries use.

| Form | Meaning | Example |
| --- | --- | --- |
| `D100` | 16-bit unsigned word | one word from `D100` |
| `D100:S` | 16-bit signed word | signed value from `D100` |
| `D200:D` | 32-bit unsigned dword | `D200` + `D201` |
| `D200:L` | 32-bit signed dword | signed `D200` + `D201` |
| `D300:F` | IEEE-754 float32 | `D300` + `D301` as float |
| `D50.3` | bit inside one word device | bit 3 of `D50` |
| `M1000` | direct bit device | one logical bit |

Notes:

- Use `:` for data types and special views such as `D100:S`, `D200:D`,
  `D300:F`; use `.` only for bit-in-word access such as `D50.3`.
- `D50.D` is bit `0xD` / bit 13, not a 32-bit data type request.
- `.bit` notation is valid only for word devices such as `D50.3`.
- Direct bit devices should be addressed directly, for example `M1000`, `X20`, or `Y1A`.
- `B`, `W`, `SB`, `SW`, `DX`, and `DY` keep Mitsubishi hexadecimal numbering rules.
- The high-level profile-aware parser rejects `DX` and `DY` for `PlcProfile::IqF` before transport.
- High-level string `X/Y` addresses require an explicit `PlcProfile`.
- `PlcProfile::IqF` interprets string `X/Y` in octal. Other supported families use hexadecimal string `X/Y`.

### Optional High-Level Layer Notes
The high-level layer lives in `slmp_high_level.h`.

- It is user-facing and convenience-oriented.
- It uses `std::string` and `std::vector`.
- The core `slmp_minimal.h` client remains the fixed-buffer, no-allocation transport and protocol layer.
- You can compile it out with `SLMP_MINIMAL_ENABLE_HIGH_LEVEL=0` when image size matters and the high-level helpers are not used.

### Device Range Catalogs

The high-level layer also exposes explicit PLC-family device-range helpers. This path does not call `ReadTypeName`. The caller chooses the family and the helper reads the family-specific `SD` block once to build a catalog of `point_count`, inclusive `upper_bound`, and formatted ranges such as `X0000-X1777`.

```cpp
slmp::highlevel::DeviceRangeCatalog catalog;
const slmp::Error err = slmp::highlevel::readDeviceRangeCatalogForPlcProfile(
    plc,
    slmp::highlevel::PlcProfile::QnU,
    catalog);

if (err == slmp::Error::Ok) {
    // catalog.entries -> X/Y/M/... with point_count and address_range
}
```

Supported `PlcProfile` values are `IqF`, `IqR`, `IqL`, `MxF`, `MxR`, `QCpu`, `LCpu`, `QnU`, and `QnUDV`.

### More High-Level Examples

```cpp
slmp::highlevel::Value v = slmp::highlevel::Value::u16Value(321);
slmp::highlevel::writeTyped(plc, "D100", v);

slmp::highlevel::Snapshot updates = {
    {"D100", slmp::highlevel::Value::u16Value(321)},
    {"D200:F", slmp::highlevel::Value::float32Value(12.5f)},
    {"D50.3", slmp::highlevel::Value::bitValue(true)},
};
slmp::highlevel::writeNamed(plc, updates);

std::vector<uint16_t> words;
slmp::highlevel::readWordsChunked(plc, "D1000", 1200, words, 960, true);
```

Chunked helpers are explicit opt-in. Typed helpers, named snapshots, and other logical-value APIs do not silently change one caller-visible value into a different fallback request shape.

```cpp
char normalized[32] = {};
if (slmp::highlevel::normalizeAddress(" d200:f ", normalized, sizeof(normalized)) == slmp::Error::Ok) {
    // normalized -> "D200:F"
}

if (slmp::highlevel::normalizeAddress(" y217 ", slmp::highlevel::PlcProfile::IqF, normalized, sizeof(normalized)) == slmp::Error::Ok) {
    // normalized -> "Y217"
}
```

## Device Support (PLC Device Codes)

This minimal client focuses on direct device access. Actual availability depends on PLC model, firmware, and access settings.

| Group | Codes | Status | Notes |
| --- | --- | --- | --- |
| Bit devices (direct / high-level) | SM, X, Y, M, L, F, V, B, TS, TC, LTS, LTC, STS, STC, LSTS, LSTC, CS, CC, LCS, LCC, SB, DX, DY | Supported | `X/Y/B/SB/DX/DY` use hexadecimal numbering. `DX/DY` are rejected by the high-level profile-aware parser for `PlcProfile::IqF`. Long timer / counter bit families are iQ-R device codes. |
| Word devices (direct / high-level) | SD, D, W, SW, TN, LTN, STN, LSTN, CN, LCN, Z, LZ, R, ZR, RD | Supported | `W/SW` use hexadecimal numbering. `LTN/LSTN` also have dedicated decoded helper APIs. |
| Direct device codes that stay excluded from generic direct access | G, HG | Not supported | Use the dedicated module-buffer / extended-device APIs instead of normal direct-device helpers. |
| Extended device access | `U\\G`, `U\\HG`, `J\\device` | Supported via dedicated APIs | Use `readWordsModuleBuf` / `writeWordsModuleBuf`, `readBitsModuleBuf` / `writeBitsModuleBuf`, `readWordsLinkDirect` / `writeWordsLinkDirect`, or the `ExtDeviceSpec` random-read helpers. |

### iQ-R Range Maximum Reference

For iQ-R-series targets, the PLC-configured current point count is read from
the family-specific `SD` range registers by higher-level catalog code. The
maximum below is the cap for that SD-derived point count:

`point_count = min(SD point count, max_point_count)`

The displayed upper bound is then `point_count - 1`. This minimal C++ client
uses iQ-R `0002/0003` device access when `CompatibilityMode::iQR` is selected,
so these maximum addresses fit the 4-byte iQ-R device number field. The library
does not currently auto-build a range catalog from these SD values; keep this as
a caller-side diagnostics and documentation reference.

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

Long-family route notes:

- `LTN`, `LSTN`, `LCN`, and `LZ` are 32-bit scalar forms in the high-level API.
- `LCN` current-value reads and writes use random dword access in the high-level API.
- `LTS`, `LTC`, `LSTS`, and `LSTC` state reads use the long timer 4-word decode helpers.
- `LCS` and `LCC` state reads use direct bit read.
- High-level state writes for `LTS`/`LTC`/`LSTS`/`LSTC`/`LCS`/`LCC` use random bit write (`0x1402`).
- Low-level direct bit writes and direct word writes to these long-family logical forms are guarded before transport.

### Optional SLMP End-Code Text

The core client stores the raw PLC end code in `SlmpClient::lastEndCode()`.
Text conversion is split out so small firmware can omit it:

| Need | Function | Add these files to the build |
| --- | --- | --- |
| Code-derived labels and category helpers, such as `slmp_end_code_c201` and `isRemotePasswordEndCode()` | `endCodeString()`, `isRemotePasswordEndCode()` | `src/slmp_error_codes.cpp` |
| English messages only | `endCodeMessageEnglish()` | `src/slmp_error_codes.cpp`, `src/slmp_error_messages_en.cpp` |
| Japanese messages only | `endCodeMessageJapanese()` | `src/slmp_error_codes.cpp`, `src/slmp_error_messages_ja.cpp` |
| Runtime English/Japanese switch | `endCodeMessage()` | `src/slmp_error_codes.cpp`, `src/slmp_error_messages.cpp`, `src/slmp_error_messages_en.cpp`, `src/slmp_error_messages_ja.cpp` |

Message text is kept in `src/lang/slmp_end_code_messages_en.def` and
`src/lang/slmp_end_code_messages_ja.def`. The tables contain error
detail/cause text. Remote-password retry-delay codes (`C810H` to `C815H`)
also include the wait time because the detail text alone is identical.
Use numeric end codes or category helpers such as `isRemotePasswordEndCode()`
for application branching; `endCodeString()` is only a diagnostic label.

## Use Cases

- Edge devices that must read PLC signals with tight RAM/CPU budgets.
- Wi-Fi/Ethernet MCU gateways bridging SLMP data to MQTT/HTTP.
- Firmware projects that want a small Arduino-oriented SLMP core plus an optional high-level helper layer.

## Documentation

Follows the workspace-wide hierarchical documentation policy:

- `docsrc/user/SETUP_GUIDE.md`: installation for Arduino/PlatformIO and hardware setup
- `docsrc/user/USAGE_GUIDE.md`: high-level helpers first, then core sync/async and memory model
- `examples/README.md`: example selection, including the compile-checked high-level snapshot sample
- `plc-comm-slmp-cpp-minimal-console-app`: companion repository for interactive console sketches
- `docsrc/validation/reports/`: formal evidence of communication with Mitsubishi hardware
- `docsrc/maintainer/DEVELOPER_NOTES.md`: metrics, host-testing, and internal design

## API Reference Generation

The generated API reference is driven mainly by comments in the public headers.

- `src/slmp_minimal.h`: low-level client lifecycle, device helpers, sync/async operations, long timer helpers, module-buffer access, link-direct access, memory access, and label APIs
- `src/slmp_high_level.h`: string address parsing, typed values, named snapshots, compiled read plans, chunked reads, and `Poller`
- This README is the overview. The header comments are the per-symbol reference used by the published documentation site.

When adding or changing public APIs, update the header comments in the same change so the generated docs stay aligned with the implementation.

## Development & CI

Quality is managed via `run_ci.bat`.

### Quality Checks
- **Building**: `pio run`
- **Static Analysis**: `pio check` (cppcheck / clang-tidy)
- **Host Testing**: G++ tests for protocol logic.

### Local CI Run
```bash
run_ci.bat
release_check.bat
```

## License

Distributed under the [MIT License](LICENSE).
