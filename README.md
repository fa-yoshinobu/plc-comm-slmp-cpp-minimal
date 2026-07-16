[![CI](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml/badge.svg)](https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fa-yoshinobu/library/slmp-connect-cpp-minimal.svg)](https://registry.platformio.org/libraries/fa-yoshinobu/slmp-connect-cpp-minimal)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# MELSEC SLMP for C++ / ESP32 / RP2040

C++ library for MELSEC SLMP (Binary 3E/4E) PLC communication on ESP32/RP2040-class boards using Arduino-compatible cores or PlatformIO.

## PLC Comm Family

This library is part of the plc-comm family. See the [package matrix](https://fa-yoshinobu.github.io/plc-comm-docs-site/package-matrix/) for protocol, language, registry, and install-command mapping.

## Supported PLC profiles

The maintained profile table is in [PLC profiles](docsrc/user/PROFILES.md). Choose one exact canonical PLC profile from that table.

## Strict profile

`SlmpClient` always applies strict profile guards in its normal public API. Operations known to be unavailable for the selected PLC are rejected before sending.

## Supported device types

The maintained device and range tables are in the [SLMP Profile Reference](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/). Use that page for supported device families, address syntax, and profile-specific notes.

## Installation

Add the PlatformIO Registry package to your `platformio.ini`:

```ini
lib_deps =
  fa-yoshinobu/slmp-connect-cpp-minimal@^4.0.0
```

## Quick example

This ESP32 sketch connects over TCP to `192.168.250.100:1025`, configures the iQ-R profile, and reads `D100:U`.

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
slmp::ArduinoClientTransport transport(tcp, slmp::configureEsp32WifiClientKeepAlive);

uint8_t txBuffer[160] = {};
uint8_t rxBuffer[160] = {};
slmp::SlmpClient plc(transport, kProfile, slmp::TargetAddress{0x00, 0xFF, slmp::module_io::OwnStation, 0x00}, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    if (!plc.connect(kPlcHost, kPlcPort)) {
        Serial.println("PLC connection failed");
    }
}

void loop() {
    slmp::highlevel::Value value;
    const slmp::Error err = slmp::highlevel::readTyped(plc, kProfile, "D100:U", value);
    if (err == slmp::Error::Ok) {
        Serial.printf("D100=%u\n", static_cast<unsigned>(value.u16));
    } else {
        Serial.printf("readTyped failed: %s\n", slmp::errorString(err));
    }

    delay(1000);
}
```

The TCP transport always requests a 30-second keepalive idle after connecting.
ESP32 `WiFiClient` uses the supplied standard configurator. Other Arduino
`Client` implementations must provide an equivalent configurator; if it is
missing or fails, the transport closes the socket and reports connection failure.

## Documentation

| Page | Use it for |
| --- | --- |
| [Full documentation site](https://fa-yoshinobu.github.io/plc-comm-docs-site/) | Unified docs for all PLC communication libraries. |
| [Getting started](docsrc/user/GETTING_STARTED.md) | Install the library, choose a profile, and make your first read and write. |
| [Usage guide](docsrc/user/USAGE_GUIDE.md) | Use the low-level and high-level APIs in application code. |
| [API reference](docsrc/user/API_REFERENCE.md) | Generated reference for the public C++ headers. |
| [SLMP profile reference](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/) | Device families, value types, address forms, and profile limits. |
| [PLC profiles](docsrc/user/PROFILES.md) | Canonical profile names, API selectors, frame types, compatibility modes, and cautions. |
| [Examples](examples/README.md) | Maintained example folders and PlatformIO commands. |

## License and registry

| Item | Value |
| --- | --- |
| License | [MIT](LICENSE) |
| Registry | [PlatformIO Registry](https://registry.platformio.org/libraries/fa-yoshinobu/slmp-connect-cpp-minimal) |
| Package | `slmp-connect-cpp-minimal` |

## Commercial support

If you plan to embed this library in a paid or commercial product, please consider a separate support agreement or supporting the project as a sponsor.

Contact: <https://fa-labo.com/contact.html>
