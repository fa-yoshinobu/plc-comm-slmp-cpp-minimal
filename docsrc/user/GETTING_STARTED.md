# Getting started

## Start here

Use this page to make your first SLMP read and write from an ESP32/RP2040-class board. The examples use TCP at `192.168.250.100:1025`; use UDP port `1035` when you build a UDP transport variant.

## Prerequisites

| Need | Detail |
| --- | --- |
| Build system | PlatformIO or Arduino IDE with ESP32/RP2040 board support. |
| Network stack | A board or shield that provides a `WiFiClient` or `EthernetClient` compatible TCP client. |
| PLC endpoint | Your PLC is reachable at `192.168.250.100`; TCP uses port `1025`, and UDP uses port `1035`. |
| Register for testing | A safe word register such as `D100:U` for reads and a test-only word such as `D9000:U` for writes. |

## Add the library

Add the PlatformIO Registry package to your `platformio.ini`:

```ini
lib_deps =
  fa-yoshinobu/slmp-connect-cpp-minimal@^0.8.0
```

## Include the right headers

| Header | Use |
| --- | --- |
| `#include <slmp_minimal.h>` | Low-level `slmp::SlmpClient`, device helpers, fixed buffers, and direct protocol calls. |
| `#include <slmp_high_level.h>` | Optional high-level helpers such as `slmp::highlevel::readTyped`, `slmp::highlevel::writeTyped`, and `slmp::highlevel::Poller`. |
| `#include <slmp_arduino_transport.h>` | TCP and UDP transport adapters for Arduino-compatible ESP32/RP2040 cores. |

The high-level layer is optional and is not included by `slmp_minimal.h` automatically.

## Choose your PLC profile

Call `slmp::highlevel::configureClientForPlcProfile` before the first read or write. It applies the frame type, compatibility mode, and profile-specific guards for the selected PLC profile; it does not auto-detect your PLC.

| Target | Profile to start with |
| --- | --- |
| MELSEC iQ-R | `slmp::highlevel::PlcProfile::IqR` |
| MELSEC iQ-F | `slmp::highlevel::PlcProfile::IqF` |
| MELSEC iQ-L | `slmp::highlevel::PlcProfile::IqL` |
| Q/L legacy CPU | `slmp::highlevel::PlcProfile::QCpu` or `slmp::highlevel::PlcProfile::LCpu` |

The complete sketches below show where the profile configuration belongs.

Strict profile checks are enabled by default. Leave them enabled for normal applications; use `plc.setStrictProfile(false)` only when you deliberately want to send the request and inspect the PLC response yourself.

## First read

This complete sketch connects to `192.168.250.100:1025` and reads `D100:U` once per second.

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
    if (plc.connect(kPlcHost, kPlcPort)) {
        Serial.println("PLC connected");
    } else {
        Serial.println("PLC connection failed");
    }
}

void loop() {
    slmp::highlevel::Value value;
    const slmp::Error err = slmp::highlevel::readTyped(plc, kProfile, "D100:U", value);
    if (err == slmp::Error::Ok) {
        Serial.printf("D100=%u\n", static_cast<unsigned>(value.u16));
    } else {
        Serial.printf("read failed: %s\n", slmp::errorString(err));
    }

    delay(1000);
}
```

Expected serial output:

```text
PLC connected
D100=1234
```

## First write

Use a test-only register that your PLC program does not use for control decisions. This example writes `1234` to `D9000:U` and then reads it back.

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
bool wroteOnce = false;

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
    if (!wroteOnce) {
        const slmp::Error writeErr = slmp::highlevel::writeTyped(
            plc,
            kProfile,
            "D9000:U",
            slmp::highlevel::Value::u16Value(1234U));
        Serial.printf("write D9000: %s\n", slmp::errorString(writeErr));
        wroteOnce = (writeErr == slmp::Error::Ok);
    }

    slmp::highlevel::Value value;
    const slmp::Error readErr = slmp::highlevel::readTyped(plc, kProfile, "D9000:U", value);
    if (readErr == slmp::Error::Ok) {
        Serial.printf("D9000=%u\n", static_cast<unsigned>(value.u16));
    }

    delay(1000);
}
```

## Confirm success

1. The board joins the same network as your PLC.
2. The serial monitor prints `PLC connected`.
3. The PLC-side communication data code is Binary and the port/open setting matches your transport; see the [MELSEC SLMP PLC Setup Guide](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/slmp/).
4. PLC-side RUN-time write permission is enabled before you run a write example where the PLC exposes that setting.
5. `D100:U` prints a stable value or a value you expect from the PLC.
6. The write test uses a safe address reserved for bring-up.
7. A write followed by a read returns the value you wrote.

## If it does not work

| Symptom | Check |
| --- | --- |
| `connect()` fails | Your board must provide a `WiFiClient` or `EthernetClient` compatible transport, and the PLC must listen on TCP port `1025`. |
| Connection opens but all requests fail | Confirm Binary communication data code in the PLC setup guide. |
| Reads work but writes fail | Confirm RUN-time write permission in the PLC setup guide and the selected profile write policy. |
| High-level helpers are undefined | Add `#include <slmp_high_level.h>`; it is not included automatically. |
| Address parsing fails | Check that your `slmp::highlevel::PlcProfile` matches your actual hardware. |
| `profile_feature_blocked` is returned | The selected profile does not support that operation. Use a supported operation, or intentionally call `plc.setStrictProfile(false)` for verification. |
| `X` or `Y` looks wrong | Use the profile-aware overloads of `slmp::highlevel::readTyped` and `slmp::highlevel::writeTyped`. |
