
[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-slmp-cpp-minimal/)

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

## Device Support (PLC Device Codes)

This minimal client focuses on direct device access. Actual availability depends on PLC model, firmware, and access settings.

| Group | Codes | Status | Notes |
| --- | --- | --- | --- |
| Bit devices (direct) | SM, X, Y, M, L, F, V, B, TS, TC, STS, STC, CS, CC, SB, DX, DY | Supported | `X/Y/B/SB/DX/DY` use hexadecimal numbering. |
| Word devices (direct) | SD, D, W, SW, TN, STN, CN, R, ZR | Supported | `W/SW` use hexadecimal numbering. |
| Not supported (direct API) | S, Z, LZ, RD, LTS/LTC/LTN, LSTS/LSTC/LSTN, LCS/LCC/LCN, G/HG, `Jn\\Xn` | Not supported | Appendix 1 and linked direct devices are not implemented in the minimal library. |

## Use Cases

- Edge devices that must read PLC signals with tight RAM/CPU budgets.
- Wi-Fi/Ethernet MCU gateways bridging SLMP data to MQTT/HTTP.
- Plant-floor test rigs that run the PC console CLI for quick verification.

## Documentation

Follows the workspace-wide hierarchical documentation policy:

- [**Setup Guide**](docsrc/user/SETUP_GUIDE.md): Installation for Arduino/PlatformIO and hardware setup.
- [**PC Console CLI**](scripts/w6300_console_cli.py): PC-side serial command runner and auto-inspection sequence for the W6300 console.
- [**Usage Guide**](docsrc/user/USAGE_GUIDE.md): Detailed API reference, Sync vs Async, and memory model.
- [**QA Reports**](docsrc/validation/reports/): Formal evidence of communication with Mitsubishi hardware.
- [**Developer Notes**](docsrc/maintainer/DEVELOPER_NOTES.md): Metrics, host-testing, and internal design.

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



