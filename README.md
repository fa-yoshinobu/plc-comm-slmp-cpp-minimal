[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/cpp/GETTING_STARTED/)
[![Release](https://img.shields.io/github/v/release/fa-yoshinobu/plc-comm-slmp-cpp-minimal?label=release)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/releases/latest)
[![CI](https://img.shields.io/github/actions/workflow/status/fa-yoshinobu/plc-comm-slmp-cpp-minimal/ci.yml?branch=main&label=CI&logo=github)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Static Analysis: PlatformIO Check](https://img.shields.io/badge/Lint-PIO%20Check-blue.svg)](https://docs.platformio.org/en/latest/plus/pio-check.html)

# MELSEC SLMP for C++ / Arduino

C++ library for MELSEC SLMP (Binary 3E/4E) PLC communication on Arduino and PlatformIO.

## Supported PLC profiles

The maintained profile table is in [PLC profiles](docsrc/user/PROFILES.md). Choose one exact canonical PLC profile from that table.

## Supported device types

The maintained device and range tables are in [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md). Use that page for supported device families, address syntax, and profile-specific notes.

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

| Page | Use it for |
| --- | --- |
| [Full documentation site](https://fa-yoshinobu.github.io/plc-comm-docs-site/) | Unified docs for all PLC communication libraries. |
| [Getting started](docsrc/user/GETTING_STARTED.md) | Install the library, choose a profile, and make your first read and write. |
| [Usage guide](docsrc/user/USAGE_GUIDE.md) | Use the low-level and high-level APIs in application code. |
| [Supported registers](docsrc/user/SUPPORTED_REGISTERS.md) | Device families, value types, and address forms. |
| [PLC profiles](docsrc/user/PROFILES.md) | Canonical profile names, API selectors, frame types, compatibility modes, and cautions. |
| [Examples](examples/README.md) | Maintained example folders and PlatformIO commands. |

## Hardware verified

Live-device verification is maintained in [Hardware validation](docsrc/validation/reports/HARDWARE_VALIDATION.md).
See that page for verified PLC models, transports, dates, limitations, and retained validation notes.

## License and registry

| Item | Value |
| --- | --- |
| License | [MIT](LICENSE) |
| Registry | [PlatformIO Registry](https://registry.platformio.org/libraries/fa-yoshinobu/slmp-connect-cpp-minimal) |
| Package | `slmp-connect-cpp-minimal` |
