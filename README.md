# (Mitsubishi MELSEC) SLMP C++ Minimal

[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-slmp-cpp-minimal?label=release)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-slmp-cpp-minimal/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Static Analysis: PlatformIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/plus/pio-check.html)

A lightweight, microcontroller-oriented C++ library for Mitsubishi SLMP (Seamless Message Protocol). Optimized for Arduino-compatible Wi-Fi and Ethernet targets with zero dynamic allocation.

## Key Features

- **Embedded Optimized**: No dynamic memory allocation (`malloc`/`new`). Predictable RAM usage.
- **Async Support**: Built-in state machine for non-blocking communication.
- **Broad Hardware Support**: Board-agnostic design for Wi-Fi and Ethernet (Arduino-compatible).
- **Binary 3E/4E**: Supports both modern and legacy SLMP frames.
- **CI-Ready**: Host-side G++ testing and size regression monitoring.
- **Maintained Samples**: Full serial consoles for M5Stack Atom Matrix and W6300-EVB-Pico2.

## Quick Start

### PC Console (Recommended First Test)

```bash
python scripts/w6300_console_cli.py --list-ports
python scripts/w6300_console_cli.py --port COM6 --auto
```

The `--auto` sequence verifies connectivity and runs a small command sweep.

### Minimal Firmware Usage
```cpp
#include <slmp_arduino_transport.h>

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);

uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp::SlmpClient plc(transport, tx_buffer, 128, rx_buffer, 128);

void setup() {
    plc.connect("192.168.1.10", 1025);
    slmp::TypeNameInfo info = {};
    plc.readTypeName(info);
}
```

### Passive Profile Recommendation
If `readTypeName()` succeeds, you can derive a lightweight default profile from the returned model information:

```cpp
slmp::TypeNameInfo info = {};
if (plc.readTypeName(info) == slmp::Error::Ok) {
    slmp::ProfileRecommendation recommendation = slmp::recommendProfile(info);
    if (recommendation.confident) {
        slmp::applyProfileRecommendation(plc, recommendation);
    }
}
```

## Device Compatibility Snapshot (Hardware-Verified)

This table is a readable snapshot. The full matrix lives in `docs/validation/reports/PLC_COMPATIBILITY.md` (Python is the source of truth).

| Family | Verified Models | Status | Recommended Profile |
| --- | --- | --- | --- |
| iQ-R | R00CPU, R08CPU, R08PCPU, R120PCPU, RJ71EN71 | YES (core commands) | 3e/ql, 3e/iqr, 4e/ql, 4e/iqr |
| iQ-L | L16HCPU | YES (core commands) | 3e/ql, 3e/iqr, 4e/ql, 4e/iqr |
| MELSEC-Q | Q06UDVCPU, Q26UDEHCPU, QJ71E71-100 | PARTIAL | 3e/ql (4e/ql for QJ71E71) |
| MELSEC-L | L26CPU-BT | PARTIAL | 3e/ql |
| iQ-F | FX5U, FX5UC | PARTIAL | 3e/ql (4e/ql for FX5U) |
| Third-Party MC | KV-7500, KV-XLE02 | PARTIAL (MC compatible) | 3e/ql, 4e/ql |

Notes:

- Q/L series often reject `0101` (type name) and may require 3E/QL.
- Third-party MC-compatible endpoints are not Mitsubishi native; results describe MC compatibility.

## Use Cases

- Edge devices that must read PLC signals with tight RAM/CPU budgets.
- Wi-Fi/Ethernet MCU gateways bridging SLMP data to MQTT/HTTP.
- Plant-floor test rigs that run the PC console CLI for quick verification.

## Documentation

Follows the workspace-wide hierarchical documentation policy:

- [**Setup Guide**](docs/user/SETUP_GUIDE.md): Installation for Arduino/PlatformIO and hardware setup.
- [**PC Console CLI**](scripts/w6300_console_cli.py): PC-side serial command runner and auto-inspection sequence for the W6300 console.
- [**Usage Guide**](docs/user/USAGE_GUIDE.md): Detailed API reference, Sync vs Async, and memory model.
- [**QA Reports**](docs/validation/reports/): Formal evidence of communication with Mitsubishi hardware.
- [**Developer Notes**](docs/maintainer/DEVELOPER_NOTES.md): Metrics, host-testing, and internal design.

## Development & CI

Quality is managed via `run_ci.bat`.

### Quality Checks
- **Building**: `pio run`
- **Static Analysis**: `pio check` (cppcheck / clang-tidy)
- **Host Testing**: G++ tests for protocol logic.

### Local CI Run
```bash
run_ci.bat
```

## License

Distributed under the [MIT License](LICENSE).

