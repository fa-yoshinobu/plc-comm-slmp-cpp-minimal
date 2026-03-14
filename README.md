# SLMP4E Connect C++ Minimal

This is the microcontroller-oriented C++ version of:

- `slmp4e-connect-python`: <https://github.com/fa-yoshinobu/slmp4e-connect-python>

Status:

- this library is still under verification
- tested targets and combinations are being expanded incrementally

GitHub:

- this library: <https://github.com/fa-yoshinobu/slmp4e-connect-cpp-minimal>

Target boards:

- ESP32
- RP2040

Core protocol shape:

- `TCP`
- `SLMP 4E`
- binary
- iQ-R / iQ-F style direct device format (`0002` / `0003`)

Main functions:

- `readTypeName()`
- direct `word` / `bit` read and write
- direct `dword` read and write
- one-shot helpers for `word`, `bit`, `dword`
- `readRandom()` / `writeRandomWords()` / `writeRandomBits()`
- `readBlock()` / `writeBlock()`
- `remotePasswordUnlock()` / `remotePasswordLock()`

## Why this shape

For ESP32 and RP2040, the compact version should keep only the protocol core that is stable and small:

- transport abstraction
- 4E request/response encoding
- direct device access
- fixed caller-owned TX/RX buffers

Good points:

- compact enough for ESP32 / RP2040 class boards
- no dynamic allocation inside the client
- the caller can choose buffer size based on use case
- optional helpers stay small if you do not call them
- same library API works with Wi-Fi or Ethernet transports
- host-side tests can run without Arduino or a PLC
- reconnect helper is available without changing the core client API

Random and block access are exposed as flat buffer APIs:

- random read returns `word` and `dword` values into separate caller-owned arrays
- block read returns flattened values in the same block order you requested
- block bit access uses packed 16-bit words, not one `bool` per bit

Optional typed device helpers are available in `slmp4e::dev`:

- use `slmp4e::dev::D(slmp4e::dev::dec(100))` for decimal devices
- use `slmp4e::dev::Y(slmp4e::dev::hex(0x20))` for hex devices
- `F` device uses `slmp4e::dev::FDevice(...)` because Arduino already defines `F(...)`
- use `slmp4e::dev::blockRead(...)` / `slmp4e::dev::blockWrite(...)` for compact block declarations

Optional PLC end-code text is available through `endCodeString(uint16_t)`:

- it only covers the common end codes documented for this project target
- unknown values return `unknown_plc_end_code`
- if you do not call it, it stays out of the final binary in the current build setup

Direct-access convenience helpers are also available:

- `readDWords()` / `writeDWords()`
- `readOneWord()` / `writeOneWord()`
- `readOneBit()` / `writeOneBit()`
- `readOneDWord()` / `writeOneDWord()`

Optional connection helper is available in `slmp4e_utility.h`:

- `ReconnectHelper` retries `connect()` with a fixed interval
- `consumeConnectedEdge()` lets your sketch run password unlock or startup reads once per reconnect
- `forceReconnect()` lets your sketch restart the session after a transport failure

For request/response inspection:

- `lastRequestFrame()` / `lastRequestFrameLength()`
- `lastResponseFrame()` / `lastResponseFrameLength()`
- `formatHexBytes()` to convert raw bytes into printable hex text

## Directory layout

- `src/slmp4e_minimal.h`: public minimal API
- `src/slmp4e_minimal.cpp`: protocol codec and request handling
- `src/slmp4e_arduino_transport.h`: adapter for Arduino `Client`
- `src/slmp4e_utility.h`: optional reconnect helper
- `TROUBLESHOOTING.md`: practical error and bring-up notes
- `examples/README.md`: example sketch index by use case
- `scripts/size_report.py`: PlatformIO size regression report generator
- `scripts/size_baseline.json`: committed size baseline for CI comparison
- `scripts/mock_plc_server.py`: local SLMP 4E mock PLC for desktop bring-up
- `scripts/mock_plc_state.sample.json`: sample mock PLC memory image
- `scripts/release_notes.py`: changelog section extractor for GitHub Releases
- `scripts/run_socket_integration.py`: build-and-run helper for the real-socket host test
- `tests/python_golden_frames.h`: Python-compatibility golden request fixtures
- `tests/slmp4e_socket_integration.cpp`: real TCP integration test against the mock PLC
- `examples/esp32_read_words`: ESP32 example
- `examples/esp32_random_block`: ESP32 random/block example
- `examples/esp32_dynamic_bits`: ESP32 dynamic `M100..M500` write example with odd-number filtering
- `examples/esp32_password_read_loop`: ESP32 reconnect + password + periodic read example
- `examples/rp2040_w5500_read_words`: RP2040 + W5500 Ethernet example
- `tests/slmp4e_minimal_tests.cpp`: host-side mock transport tests

## Memory model

The client does not allocate memory.

You provide:

- one TX buffer
- one RX buffer
- a transport object

Recommended starting sizes:

- `96` bytes if you only do small reads and writes
- `128` to `192` bytes if you expect larger word writes
- `192` to `256` bytes if you plan to use random/block access

## Function Size Matrix

Measured with dedicated `esp32dev-size-*` probe environments on PlatformIO.

Conditions:

- board: `esp32dev`
- framework: Arduino ESP32 + `WiFi`
- fixed buffers for all rows: `tx_buffer[256]`, `rx_buffer[256]`
- baseline probe references `connect()` and `readTypeName()` only
- deltas are relative to that baseline

| Function set | Probe env | Flash used | Flash delta | RAM used | RAM delta |
|---|---|---:|---:|---:|---:|
| baseline (`readTypeName`) | `esp32dev-size-base` | `742,721` bytes (`56.67%`) | `0` bytes (`0.00 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| direct read/write + dword/one-shot | `esp32dev-size-direct` | `744,789` bytes (`56.82%`) | `+2,068` bytes (`+0.16 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| password | `esp32dev-size-password` | `743,037` bytes (`56.69%`) | `+316` bytes (`+0.02 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| end code text | `esp32dev-size-endcode` | `743,449` bytes (`56.72%`) | `+728` bytes (`+0.06 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| frame hex dump | `esp32dev-size-debug` | `743,105` bytes (`56.69%`) | `+384` bytes (`+0.03 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| random read/write | `esp32dev-size-random` | `744,161` bytes (`56.77%`) | `+1,440` bytes (`+0.11 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| block read/write | `esp32dev-size-block` | `744,113` bytes (`56.77%`) | `+1,392` bytes (`+0.11 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |
| all above | `esp32dev-size-all` | `748,789` bytes (`57.13%`) | `+6,068` bytes (`+0.46 pt`) | `45,272` bytes (`13.82%`) | `0` bytes (`0.00 pt`) |

In this measurement, RAM stays flat because these APIs add code but not extra static buffers. Local arrays used inside the probe sketch live on the stack and do not show up in PlatformIO's static RAM report.

## Host Tests

The protocol core can be tested on a desktop compiler without Arduino by building `tests/slmp4e_minimal_tests.cpp`.

```powershell
g++ -std=c++17 -Wall -Wextra -Isrc tests\slmp4e_minimal_tests.cpp src\slmp4e_minimal.cpp -o $env:TEMP\slmp4e_minimal_tests.exe
& $env:TEMP\slmp4e_minimal_tests.exe
```

The host tests cover:

- Python-compatibility golden request frames
- real TCP socket integration against `scripts/mock_plc_server.py`
- real TCP abnormal cases: PLC error, disconnect, timeout, malformed response
- direct read and frame capture
- `dword` and one-shot helpers
- random and block access
- PLC error decoding
- password unlock request encoding
- block write request encoding
- reconnect helper behavior
- transport read failure handling
- protocol failure handling
- payload mismatch handling

## Local Mock PLC

For local bring-up without a real PLC, a small SLMP 4E mock server is available in `scripts/mock_plc_server.py`.

It supports:

- `readTypeName()`
- direct word/bit read and write
- random read and write
- block read and write
- remote password unlock and lock
- optional fault injection for delay, forced end code, disconnect, and malformed response

Quick start:

```powershell
python scripts\mock_plc_server.py --seed-demo --verbose
```

Or load a fixed memory image:

```powershell
python scripts\mock_plc_server.py --state scripts\mock_plc_state.sample.json
```

This mock server is self-contained in this repository and does not depend on the ignored `slmp4e-connect-python-main/` directory.

Run the end-to-end host integration test with:

```powershell
python scripts\run_socket_integration.py --compiler g++
```

This runner executes:

- `normal`
- `plc_error`
- `disconnect`
- `delay`
- `malformed`

You can also run one scenario at a time:

```powershell
python scripts\run_socket_integration.py --compiler g++ --scenario delay
```

Useful mock PLC fault options:

- `--response-delay-ms 250`
- `--inject-end-code 0xC051 --inject-command direct_read`
- `--disconnect-after-requests 2`
- `--malformed-command read_type_name`

## API Stability

Public API policy is documented in [API_POLICY.md](./API_POLICY.md).

Current direction:

- keep core headers source-compatible where practical
- prefer additive changes over renames
- document breaking changes in `CHANGELOG.md`

Release preparation helpers:

- [CHANGELOG.md](./CHANGELOG.md)
- [RELEASE_CHECKLIST.md](./RELEASE_CHECKLIST.md)
- [TROUBLESHOOTING.md](./TROUBLESHOOTING.md)

## Example Index

See [examples/README.md](./examples/README.md) for a quick "which sketch should I start from?" table.

## Basic usage

```cpp
#include <slmp4e_arduino_transport.h>

WiFiClient tcp;
slmp4e::ArduinoClientTransport transport(tcp);

uint8_t tx_buffer[96];
uint8_t rx_buffer[96];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

plc.connect("192.168.250.101", 1025);

slmp4e::TypeNameInfo type_name = {};
plc.readTypeName(type_name);

uint16_t words[2] = {};
plc.readWords({slmp4e::DeviceCode::D, 100}, 2, words, 2);

auto y20 = slmp4e::dev::Y(slmp4e::dev::hex(0x20));
auto d100 = slmp4e::dev::D(slmp4e::dev::dec(100));
uint32_t d200 = 0;
plc.readOneDWord(slmp4e::dev::D(slmp4e::dev::dec(200)), d200);

// Optional: decode a PLC end code when lastError() == slmp4e::Error::PlcError
const char* end_text = slmp4e::endCodeString(plc.lastEndCode());

char request_hex[160] = {};
slmp4e::formatHexBytes(plc.lastRequestFrame(), plc.lastRequestFrameLength(), request_hex, sizeof(request_hex));
```

## ESP32 / RP2040 note

The protocol code is board-agnostic.

On ESP32, pass a `WiFiClient`.

On RP2040, pass any Arduino-compatible `Client` implementation that your board support package provides, for example:

- `WiFiClient` on Pico W class boards
- `EthernetClient` on W5x00 based boards

Only the network stack changes. The `slmp4e::Slmp4eClient` API stays the same.

## Example choice

- Use `examples/esp32_read_words` if the board has built-in Wi-Fi and exposes `WiFiClient`.
- Use `examples/esp32_random_block` if you want a concrete `readRandom` / `readBlock` sketch and optional write examples.
- Use `examples/esp32_dynamic_bits` if you want a `loop()` example that walks `M100..M500` dynamically and only writes odd addresses.
- Use `examples/esp32_password_read_loop` if you want reconnect, password unlock, `readTypeName()`, and periodic polling in one sketch.
  Edit `examples/esp32_password_read_loop/config.h` for Wi-Fi, PLC host, password, and poll settings.
- Use `examples/rp2040_w5500_read_words` for RP2040 boards paired with a W5500 Ethernet module.
- For Pico W, keep the RP2040 side but swap `EthernetClient` for the Wi-Fi client class provided by your core package.

## PlatformIO

Smoke-build environments are included in `platformio.ini`:

- `esp32dev`
- `esp32dev-random-block`
- `esp32dev-dynamic-bits`
- `esp32dev-session`
- `esp32-s3-devkitc-1`
- `esp32-c3-devkitm-1`
- `pico`
- `nanorp2040connect`

Function-size probe environments are also included:

- `esp32dev-size-base`
- `esp32dev-size-direct`
- `esp32dev-size-password`
- `esp32dev-size-endcode`
- `esp32dev-size-debug`
- `esp32dev-size-random`
- `esp32dev-size-block`
- `esp32dev-size-all`

Build with:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-random-block
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-dynamic-bits
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-session
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-s3-devkitc-1
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-c3-devkitm-1
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e pico
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nanorp2040connect
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-size-debug
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-size-all
```

This build compiles:

- `src/slmp4e_minimal.cpp`
- `platformio/esp32dev_smoke.cpp`

The `pico` environment uses `platformio/pico_w5500_smoke.cpp` and links `arduino-libraries/Ethernet` for a W5500-based RP2040 check.

The `nanorp2040connect` environment uses `platformio/nanorp2040connect_smoke.cpp` and links `arduino-libraries/WiFiNINA`.

With PlatformIO `raspberrypi` 1.19.0 and bundled `framework-arduino-mbed` 4.5.0, the framework-bundled `WiFi` library failed to compile in this environment. The workaround here is to use external `WiFiNINA` and ignore the bundled `WiFi` package.

## CI

GitHub Actions is configured in `.github/workflows/ci.yml`.

It runs:

- host-side mock transport tests on `g++`
- host-side real-socket integration test against the local mock PLC
- host-side abnormal socket integration scenarios through the same runner
- host-side sanitizer tests on `g++` with `ASan` / `UBSan`
- host-side coverage report generation for `src/*`
- PlatformIO size regression comparison against `scripts/size_baseline.json`
- PlatformIO builds for representative ESP32 and RP2040 environments

These environments are intended to verify the library compiles cleanly under PlatformIO before flashing a board-specific sketch.

## Release Automation

GitHub tag releases are automated in [./.github/workflows/release.yml](./.github/workflows/release.yml).

On a `v*` tag, the workflow:

- verifies `library.properties` matches the tag version
- runs host tests, coverage, and size regression
- builds representative ESP32 and RP2040 smoke targets
- generates release notes from `CHANGELOG.md`
- publishes a zip archive plus coverage and size artifacts

You can generate release notes locally with:

```powershell
python scripts\release_notes.py --changelog CHANGELOG.md --version 0.1.0 --output release-notes.md
```
