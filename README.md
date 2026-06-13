[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-slmp-cpp-minimal/)
[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-slmp-cpp-minimal?label=release)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-slmp-cpp-minimal/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Static Analysis: PlatformIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/plus/pio-check.html)

# SLMP Protocol (Minimal C++)

A minimal C++ SLMP client for Mitsubishi PLCs, compatible with Arduino and PlatformIO projects.

## Supported PLC profiles

Select one explicit profile before you read or write devices. The helper `slmp::highlevel::configureClientForPlcProfile` applies the frame type and compatibility mode shown here.

| Profile identifier | Canonical profile | Hardware | Frame type | Notes |
| --- | --- | --- | --- | --- |
| `slmp::highlevel::PlcProfile::IqF` | `melsec:iq-f` | MELSEC iQ-F | `slmp::FrameType::Frame3E` | Uses `slmp::CompatibilityMode::Legacy`; string `X` and `Y` addresses use octal notation; `DX` and `DY` are not valid. |
| `slmp::highlevel::PlcProfile::IqR` | `melsec:iq-r` | MELSEC iQ-R | `slmp::FrameType::Frame4E` | Uses `slmp::CompatibilityMode::iQR`; this is the default profile used in the examples. |
| `slmp::highlevel::PlcProfile::IqL` | `melsec:iq-l` | MELSEC iQ-L | `slmp::FrameType::Frame4E` | Uses `slmp::CompatibilityMode::iQR`; keeps its own address/range profile with iQ-R-equivalent rules. |
| `slmp::highlevel::PlcProfile::MxF` | `melsec:mx-f` | MELSEC MX-F profile | `slmp::FrameType::Frame4E` | Uses `slmp::CompatibilityMode::iQR`. |
| `slmp::highlevel::PlcProfile::MxR` | `melsec:mx-r` | MELSEC MX-R profile | `slmp::FrameType::Frame4E` | Uses `slmp::CompatibilityMode::iQR`. |
| `slmp::highlevel::PlcProfile::QCpu` | `melsec:qcpu` | MELSEC Q CPU | `slmp::FrameType::Frame3E` | Uses `slmp::CompatibilityMode::Legacy`. |
| `slmp::highlevel::PlcProfile::LCpu` | `melsec:lcpu` | MELSEC L CPU | `slmp::FrameType::Frame3E` | Uses `slmp::CompatibilityMode::Legacy`. |
| `slmp::highlevel::PlcProfile::QnU` | `melsec:qnu` | MELSEC QnU | `slmp::FrameType::Frame3E` | Uses `slmp::CompatibilityMode::Legacy`. |
| `slmp::highlevel::PlcProfile::QnUDV` | `melsec:qnudv` | MELSEC QnU(DV) | `slmp::FrameType::Frame3E` | Uses `slmp::CompatibilityMode::Legacy`. |

## Supported device types

These are the most common device families you will use first. See [supported registers](docsrc/user/SUPPORTED_REGISTERS.md) for the full catalog and profile cautions.

| Family | Meaning |
| --- | --- |
| `D` | Data registers for normal 16-bit, 32-bit, and float values. |
| `M` | Internal relays for direct bit reads and writes. |
| `X` | Inputs; profile-aware string parsing is required for correct numbering. |
| `Y` | Outputs; profile-aware string parsing is required for correct numbering. |
| `W` | Link registers using Mitsubishi hexadecimal numbering. |
| `R` | File registers using decimal numbering. |
| `LTN` | Long timer current values; use `ValueType::U32` or `ValueType::S32`. |
| `LCN` | Long counter current values; use `ValueType::U32` or `ValueType::S32`. |

## Installation

Add the PlatformIO Registry package to your `platformio.ini`:

```ini
lib_deps =
  fa-yoshinobu/slmp-connect-cpp-minimal@^0.4.12
```

## Quick example

This Arduino sketch connects over TCP to `192.168.250.100:1025`, configures the iQ-R profile, and reads `D100`.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kPlcPort = 1025;
constexpr auto kProfile = slmp::highlevel::PlcProfile::IqR;

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);

uint8_t txBuffer[160] = {};
uint8_t rxBuffer[160] = {};
slmp::SlmpClient plc(transport, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }

    slmp::highlevel::configureClientForPlcProfile(plc, kProfile);
    if (!plc.connect(kPlcHost, kPlcPort)) {
        Serial.println("PLC connection failed");
    }
}

void loop() {
    slmp::highlevel::Value value;
    const slmp::Error err = slmp::highlevel::readTyped(plc, kProfile, "D100", value);
    if (err == slmp::Error::Ok) {
        Serial.printf("D100=%u\n", static_cast<unsigned>(value.u16));
    } else {
        Serial.printf("readTyped failed: %s\n", slmp::errorString(err));
    }

    delay(1000);
}
```

## Documentation

| Page | What it covers |
| --- | --- |
| [Getting started](docsrc/user/GETTING_STARTED.md) | Install the library, choose a profile, and make your first read and write. |
| [Usage guide](docsrc/user/USAGE_GUIDE.md) | Use the low-level and high-level APIs in application code. |
| [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md) | Device families, value types, and address forms. |
| [PLC profiles](docsrc/user/PROFILES.md) | Profile identifiers, frame types, compatibility modes, and cautions. |
| [Examples](examples/README.md) | Maintained example folders and PlatformIO commands. |

## Hardware verified

Recorded validation for this library includes the following hardware combinations.

| Board or target | Transport | PLC |
| --- | --- | --- |
| M5Stack Atom | `WiFiClient` | Mitsubishi iQ-R `R08CPU` |
| W6300-EVB-Pico2 | `WiFiClient` and `WiFiUDP` through W6300lwIP | Mitsubishi iQ-R `R120PCPU` |
| W6300-EVB-Pico2 | TCP, 3E frame, legacy compatibility | Mitsubishi Q-series `Q06UDVCPU` |

## License and registry

This project is distributed under the [MIT License](LICENSE).

PlatformIO Registry: <https://registry.platformio.org/libraries/fa-yoshinobu/slmp-connect-cpp-minimal>
