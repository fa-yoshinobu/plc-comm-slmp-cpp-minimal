# SLMP4E Connect C++ Minimal

[![release](https://img.shields.io/github/v/release/fa-yoshinobu/slmp4e-connect-cpp-minimal?label=release)](https://github.com/fa-yoshinobu/slmp4e-connect-cpp-minimal/releases/latest) [![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/slmp4e-connect-cpp-minimal/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/slmp4e-connect-cpp-minimal/actions/workflows/ci.yml) [![license](https://img.shields.io/github/license/fa-yoshinobu/slmp4e-connect-cpp-minimal?label=license)](./LICENSE) [![C++](https://img.shields.io/badge/language-C%2B%2B-00599C)](#host-tests)

This is the microcontroller-oriented C++ version of:

- `slmp4e-connect-python`: <https://github.com/fa-yoshinobu/slmp4e-connect-python>

Status:

- this library is still under verification
- tested targets and combinations are being expanded incrementally

GitHub:

- this library: <https://github.com/fa-yoshinobu/slmp4e-connect-cpp-minimal>

Release:

- latest tagged release: [v0.3.0](https://github.com/fa-yoshinobu/slmp4e-connect-cpp-minimal/releases/tag/v0.3.0)
- ready-to-install Arduino library archive: `slmp4e-connect-cpp-minimal-v0.3.0.zip`
- real-board coverage is still being expanded; development-side validation currently includes an Atom Matrix run against Mitsubishi iQ-R `R08CPU`

Target boards:

- ESP32
- RP2040
- RP2350

## Documentation Guide

Use this repository in this order if you want the shortest path:

1. Read [Install](#install).
2. Pick a board or example from [Quick Start By Board](#quick-start-by-board) or [examples/README.md](./examples/README.md).
3. Use [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) if bring-up fails.

If you are looking for a specific kind of information:

| Need | Read |
|---|---|
| install the library and open a first sketch | [Install](#install) and [Fastest Start](#fastest-start) |
| choose the right example for a board or use case | [Quick Start By Board](#quick-start-by-board) and [examples/README.md](./examples/README.md) |
| use the interactive debug consoles | [Example choice](#example-choice) |
| debug connection, buffer, or protocol problems | [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) |
| understand API compatibility expectations | [API_POLICY.md](./API_POLICY.md) and [CHANGELOG.md](./CHANGELOG.md) |

## Install

If you use Arduino IDE:

1. Download `slmp4e-connect-cpp-minimal-v0.2.1.zip` from the release page.
2. Open `Sketch -> Include Library -> Add .ZIP Library...`.
3. Open `File -> Examples -> SLMP4E Connect C++ Minimal`.

If you use PlatformIO:

1. Download the release zip or clone this repository.
2. Place it under your project `lib/slmp4e-connect-cpp-minimal/`, or keep it as a checked-out local library.
3. Include `slmp4e_minimal.h` or `slmp4e_arduino_transport.h` from your application code.

Notes:

- this repository already contains `library.properties`, so the release zip has the layout Arduino expects
- this repository also contains `library.json` for PlatformIO package metadata
- if Arduino Library Manager does not list it yet, use the release zip path above

## Quick Start By Board

- ESP32 + `WiFiClient`: start with `examples/esp32_read_words`
- ESP32 + reconnect/password/session flow: start with `examples/esp32_password_read_loop`
- ESP32 + random/block access: start with `examples/esp32_random_block`
- ESP32 + dynamic bit walk: start with `examples/esp32_dynamic_bits`
- ESP32-C3 + `WiFiClient` interactive console: start with `examples/esp32_c3_serial_console`
- Atom Matrix (ESP32-PICO-D4) + `WiFiClient` interactive console: start with `examples/atom_matrix_serial_console`
- RP2040 + W5500 + `EthernetClient`: start with `examples/rp2040_w5500_read_words`
- W5500-EVB-Pico2 + onboard W5500 + `EthernetClient`: start with `examples/w5500_evb_pico2_read_words`
- W6300-EVB-Pico2 + onboard W6300 + `WiFiClient` via `W6300lwIP`: start with `examples/w6300_evb_pico2_read_words`
- Nano RP2040 Connect + `WiFiNINA`: use the same core API shape as the ESP32 examples, but pass `WiFiClient` from `WiFiNINA`
- Pico W class boards: start from the RP2040 example and swap `EthernetClient` for the Wi-Fi client class provided by your core package

## Fastest Start

For the smallest bring-up path, wire up networking first and then copy one of these:

- ESP32: `examples/esp32_read_words/esp32_read_words.ino`
- ESP32-C3: `examples/esp32_c3_serial_console/esp32_c3_serial_console.ino`
- Atom Matrix: `examples/atom_matrix_serial_console/atom_matrix_serial_console.ino`
- RP2040 + W5500: `examples/rp2040_w5500_read_words/rp2040_w5500_read_words.ino`
- W5500-EVB-Pico2: `examples/w5500_evb_pico2_read_words/w5500_evb_pico2_read_words.ino`
- W6300-EVB-Pico2: `examples/w6300_evb_pico2_read_words/w6300_evb_pico2_read_words.ino`

The minimal session shape is:

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
```

Use the example sketches for the surrounding Wi-Fi or Ethernet setup.

Core protocol shape:

- `TCP`
- `SLMP 4E` / `3E`
- binary
- iQ-R / iQ-F style direct device format (`0002` / `0003`)

To use 3E frames:

```cpp
plc.setFrameType(slmp4e::FrameType::Frame3E);
```

Default is `Frame4E`.

## Asynchronous (Non-blocking) Communication

The library supports asynchronous communication using an internal state machine. This allows your microcontroller to perform other tasks (like sensor reading or UI updates) while waiting for a PLC response.

### Async Workflow

1.  Call a `beginXXX()` method to start an operation.
2.  In your `loop()`, call `client.update(millis())` periodically.
3.  Check `client.isBusy()` to see if the operation is still in progress.
4.  Once `isBusy()` returns `false`, check `client.lastError()` for the result.

### Example

```cpp
uint16_t words[10];
bool request_started = false;

void loop() {
    uint32_t now = millis();

    if (!request_started) {
        // Start a non-blocking read
        auto device = slmp4e::dev::D(slmp4e::dev::dec(100));
        if (plc.beginReadWords(device, 10, words, 10, now) == slmp4e::Error::Ok) {
            request_started = true;
        }
    }

    // Must call update() to progress the state machine
    plc.update(now);

    if (request_started && !plc.isBusy()) {
        if (plc.lastError() == slmp4e::Error::Ok) {
            // Success: words[] is now populated
        } else {
            // Error occurred
        }
        request_started = false; // Ready for next request
    }

    // perform other non-blocking tasks here...
}
```

### Async API

All standard operations have a non-blocking `begin` counterpart:

- `beginReadTypeName(TypeNameInfo& out, uint32_t now_ms)`
- `beginReadWords(...)`, `beginWriteWords(...)`
- `beginReadBits(...)`, `beginWriteBits(...)`
- `beginReadRandom(...)`, `beginWriteRandomWords(...)`, `beginWriteRandomBits(...)`
- `beginReadBlock(...)`, `beginWriteBlock(...)`
- `beginRemotePasswordUnlock(...)`, `beginRemotePasswordLock(...)`

Note: Synchronous methods (like `readWords()`) still work and internally use this state machine in a simple `while(isBusy())` loop.

## Main functions

- `readTypeName()`
- direct `word` / `bit` read and write
- direct `dword` read and write
- one-shot helpers for `word`, `bit`, `dword`
- `readRandom()` / `writeRandomWords()` / `writeRandomBits()`
- `readBlock()` / `writeBlock()`
- `remotePasswordUnlock()` / `remotePasswordLock()`

## Why this shape

For ESP32, RP2040, and RP2350, the compact version should keep only the protocol core that is stable and small:

- transport abstraction
- 4E request/response encoding
- direct device access
- fixed caller-owned TX/RX buffers

Good points:

- compact enough for ESP32 / RP2040 / RP2350 class boards
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
- `library.json`: PlatformIO package manifest
- `TROUBLESHOOTING.md`: practical error and bring-up notes
- `examples/README.md`: example sketch index by use case
- `scripts/size_report.py`: PlatformIO size regression report generator
- `scripts/check_markdown_links.py`: local relative-link validator for Markdown docs
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
- `examples/esp32_c3_serial_console`: ESP32-C3 Wi-Fi example with a full interactive serial debug console and `config.h`
- `examples/atom_matrix_serial_console`: Atom Matrix (ESP32-PICO-D4) Wi-Fi example with the same full interactive serial debug console shape and its own `config.h`
- `examples/rp2040_w5500_read_words`: RP2040 + W5500 Ethernet example
- `examples/w5500_evb_pico2_read_words`: W5500-EVB-Pico2 onboard Ethernet example with an interactive serial debug console and human-evaluated write verification
- `examples/w6300_evb_pico2_read_words`: W6300-EVB-Pico2 onboard Ethernet example with a full interactive serial debug console for direct, one-shot, random, block, and password APIs over Arduino-Pico `W6300lwIP`
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

If you want a single entry point for function-level automatic checks, run:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" scripts\run_function_tests.py --compiler g++
```

This wrapper runs:

- `tests/slmp4e_minimal_tests.cpp` for host-side function coverage
- `scripts/run_socket_integration.py` for real-socket integration against the bundled mock PLC

From VS Code you can also use `Terminal -> Run Task... -> Host: Function Tests`.

```powershell
g++ -std=c++17 -Wall -Wextra -Isrc tests\slmp4e_minimal_tests.cpp src\slmp4e_minimal.cpp -o $env:TEMP\slmp4e_minimal_tests.exe
& $env:TEMP\slmp4e_minimal_tests.exe
```

The host tests cover:

- all direct device families through representative `readOneBit` / `writeOneBit` or `readOneWord` / `writeOneWord` paths
- Python-compatibility golden request frames
- real TCP socket integration against `scripts/mock_plc_server.py`
- real TCP abnormal cases: PLC error, disconnect, timeout, malformed response
- direct read and frame capture
- `dword` and one-shot helpers
- `dword` write helpers and random word/dword write encoding
- random and block access
- target-address and monitoring-timer header customization
- PLC error decoding
- password unlock request encoding
- block write request encoding
- invalid-argument and buffer boundary failures for random/block and `dword` paths
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

## Project Docs

- [examples/README.md](./examples/README.md): choose an example sketch by board and use case
- [TROUBLESHOOTING.md](./TROUBLESHOOTING.md): practical bring-up and failure notes
- [API_POLICY.md](./API_POLICY.md): public API stability intent during the `0.x` phase
- [CHANGELOG.md](./CHANGELOG.md): released and unreleased project changes

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

## ESP32 / RP2040 / RP2350 note

The protocol code is board-agnostic.

On ESP32, pass a `WiFiClient`.

On RP2040 or RP2350, pass any Arduino-compatible `Client` implementation that your board support package provides, for example:

- `WiFiClient` on Pico W class boards
- `EthernetClient` on W5x00 based boards

Only the network stack changes. The `slmp4e::Slmp4eClient` API stays the same.

## Example choice

- Use `examples/esp32_read_words` if the board has built-in Wi-Fi and exposes `WiFiClient`.
- Use `examples/esp32_random_block` if you want a concrete `readRandom` / `readBlock` sketch and optional write examples.
- Use `examples/esp32_dynamic_bits` if you want a `loop()` example that walks `M100..M500` dynamically and only writes odd addresses.
- Use `examples/esp32_password_read_loop` if you want reconnect, password unlock, `readTypeName()`, and periodic polling in one sketch.
  Edit `examples/esp32_password_read_loop/config.h` for Wi-Fi, PLC host, password, and poll settings.
- Use `examples/esp32_c3_serial_console` if you want the same interactive debug console style on an ESP32-C3 board over Wi-Fi.
  Edit `examples/esp32_c3_serial_console/config.h` for Wi-Fi credentials and PLC host settings.
- Use `examples/atom_matrix_serial_console` if you want the same Wi-Fi based interactive debug console on an M5Stack Atom Matrix (`ESP32-PICO-D4`) board.
  Edit `examples/atom_matrix_serial_console/config.h` for Wi-Fi credentials and PLC host settings. Type `demo` to enter the Atom-specific mode where the front button increments `D0` and the 5x5 matrix mirrors `M0..M24`.
- Use `examples/rp2040_w5500_read_words` for RP2040 boards paired with a W5500 Ethernet module.
- Use `examples/w5500_evb_pico2_read_words` for the RP2350-based W5500-EVB-Pico2 board with the onboard W5500 chip. It prints command help on `Serial`, supports interactive read/write debugging commands, and has `verifyw` / `verifyb` plus `judge ok|ng` for operator-validated write checks.
- Use `examples/w6300_evb_pico2_read_words` for the RP2350-based W6300-EVB-Pico2 board with the onboard W6300 chip. It now exposes an interactive serial console for the full `Slmp4eClient` surface: direct access, one-shot helpers, random access, block access, password lock/unlock, target selection, and frame dumps over Arduino-Pico `W6300lwIP` and `WiFiClient`.
- For Pico W, keep the RP2040 side but swap `EthernetClient` for the Wi-Fi client class provided by your core package.

### Atom Matrix Demo Mode

If you start with `examples/atom_matrix_serial_console`, type `demo` on `Serial` to enable the Atom Matrix hardware demo mode:

```text
demo
demo off
```

While demo mode is on:

- one press of the Atom front button increments `D0`
- the 5x5 RGB matrix mirrors `M0..M24`
- ON bits light green and OFF bits stay dark

### W6300 Console Quick Commands

If you start with `examples/w6300_evb_pico2_read_words`, these are good first commands to type on `Serial`:

```text
help
status
connect
type
row D100
rw D100 4
rb M100
rbits M100 4
rod D200
rdw D200 2
rr 2 D100 D101 1 D200
rblk 1 D300 2 1 M200 1
dump
```

For write-path checks:

```text
wow D120 123
ww D120 123 456
wb M100 1
wbits M100 1 0 1 0
wod D220 0x12345678
wdw D220 0x12345678 100
wrand 1 D120 123 1 D220 0x12345678
wrandb 2 M100 1 Y20 0
wblk 1 D300 2 10 20 1 M200 1 0x0005
verifyw D120 123 456
verifyb M100 1
pending
judge ok
```

For target, timing, and password-related checks:

```text
target
target 0 255 0x03FF 0
monitor
monitor 16
timeout 2000
unlock 123456
lock 123456
close
reinit
```

Notes:

- `row` / `wow` are one-word helpers, and `rod` / `wod` are one-dword helpers.
- `rr` / `wrand` / `wrandb` exercise random access APIs.
- `rblk` / `wblk` exercise block access APIs; bit blocks use packed 16-bit words.
- Start with read-only commands on a live PLC, then move to writes after confirming the target device range is safe.

### Atom Matrix Console Footprint

Measured with `pio run -e m5stack-atom-console` for the Atom Matrix interactive console example:

- RAM: `49,208` bytes / `327,680` bytes (`15.0%`)
- Flash: `781,649` bytes / `1,310,720` bytes (`59.6%`)

This footprint includes the Wi-Fi based interactive serial console plus the Atom-specific `demo` mode that increments `D0` from the front button and mirrors `M0..M24` on the 5x5 RGB matrix.

### ESP32-C3 Console Footprint

Measured with `pio run -e esp32-c3-devkitm-1-console` for the ESP32-C3 interactive console example:

- RAM: `40,284` bytes / `327,680` bytes (`12.3%`)
- Flash: `753,570` bytes / `1,310,720` bytes (`57.5%`)

This footprint includes the Wi-Fi based interactive serial console with direct, one-shot, random, block, password, target, and frame-dump commands enabled.

### W6300 Console Footprint

Measured with `pio run -e wiznet_6300_evb_pico2` for the W6300-EVB-Pico2 interactive console example:

- RAM: `69,996` bytes / `524,288` bytes (`13.4%`)
- Flash: `144,312` bytes / `2,093,056` bytes (`6.9%`)

This footprint includes the fixed-IP W6300 interactive serial console with direct, one-shot, random, block, password, target, and frame-dump commands enabled.

## PlatformIO

Smoke-build environments are included in `platformio.ini`:

- `esp32dev`
- `esp32dev-random-block`
- `esp32dev-dynamic-bits`
- `esp32dev-session`
- `esp32-s3-devkitc-1`
- `esp32-c3-devkitm-1`
- `esp32-c3-devkitm-1-console`
- `m5stack-atom-console`
- `pico`
- `wiznet_5500_evb_pico2`
- `wiznet_6300_evb_pico2`
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
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-c3-devkitm-1-console
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e m5stack-atom-console
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e pico
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e wiznet_5500_evb_pico2
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e wiznet_6300_evb_pico2
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nanorp2040connect
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-size-debug
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32dev-size-all
```

This build compiles:

- `src/slmp4e_minimal.cpp`
- `platformio/esp32dev_smoke.cpp`

The `pico` environment uses `platformio/pico_w5500_smoke.cpp` and links `arduino-libraries/Ethernet` for a W5500-based RP2040 check.

The `esp32-c3-devkitm-1-console` environment uses `platformio/esp32_c3_serial_console_smoke.cpp` and builds the Wi-Fi based interactive debug console example for ESP32-C3.

The `m5stack-atom-console` environment uses `platformio/atom_matrix_serial_console_smoke.cpp` and builds the same Wi-Fi based interactive debug console for the M5Stack Atom Matrix (`ESP32-PICO-D4`).

The `wiznet_5500_evb_pico2` environment uses `platformio/w5500_evb_pico2_smoke.cpp` and pulls the Arduino-Pico core through `maxgerhardt/platform-raspberrypi.git` so the RP2350 `wiznet_5500_evb_pico2` board definition and `EthernetCompat` support are available.

The `wiznet_6300_evb_pico2` environment uses `platformio/w6300_evb_pico2_smoke.cpp` and assumes the same Arduino-Pico platform source exposes the RP2350 `wiznet_6300_evb_pico2` board definition plus `W6300lwIP`.

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
- PlatformIO builds for representative ESP32, RP2040, and RP2350 environments

These environments are intended to verify the library compiles cleanly under PlatformIO before flashing a board-specific sketch.

## Release Automation

GitHub tag releases are automated in [./.github/workflows/release.yml](./.github/workflows/release.yml).

On a `v*` tag, the workflow:

- verifies `library.properties` matches the tag version
- runs host tests, coverage, and size regression
- builds representative ESP32, RP2040, and RP2350 smoke targets
- generates release notes from `CHANGELOG.md`
- publishes a zip archive plus coverage and size artifacts

You can generate release notes locally with:

```powershell
python scripts\release_notes.py --changelog CHANGELOG.md --version 0.2.1 --output release-notes.md
```
