# Getting started

## Start here

Use this page to make your first SLMP read and write from an Arduino-compatible board. The examples use TCP at `192.168.250.100:1025`; use UDP port `1035` when you build a UDP transport variant.

## Prerequisites

| Need | Detail |
| --- | --- |
| Build system | PlatformIO or Arduino IDE. |
| Network stack | A board or shield that provides a `WiFiClient` or `EthernetClient` compatible TCP client. |
| PLC endpoint | Your PLC is reachable at `192.168.250.100`; TCP uses port `1025`, and UDP uses port `1035`. |
| Register for testing | A safe word register such as `D100` for reads and a test-only word such as `D9000` for writes. |

## Add the library

Add the PlatformIO Registry package to your `platformio.ini`:

```ini
lib_deps =
  fa-yoshinobu/slmp-connect-cpp-minimal@^0.4.12
```

## Include the right headers

| Header | Use |
| --- | --- |
| `#include <slmp_minimal.h>` | Low-level `slmp::SlmpClient`, device helpers, fixed buffers, and direct protocol calls. |
| `#include <slmp_high_level.h>` | Optional high-level helpers such as `slmp::highlevel::readTyped`, `slmp::highlevel::writeTyped`, and `slmp::highlevel::Poller`. |
| `#include <slmp_arduino_transport.h>` | Arduino TCP and UDP transport adapters. |

The high-level layer is optional and is not included by `slmp_minimal.h` automatically.

## Choose your PLC profile

Call `slmp::highlevel::configureClientForPlcProfile` before the first read or write. It applies the frame type and compatibility mode for the selected PLC profile; it does not auto-detect your PLC.

```cpp
constexpr auto profile = slmp::highlevel::PlcProfile::IqR;
slmp::highlevel::configureClientForPlcProfile(plc, profile);
Serial.println(slmp::highlevel::plcProfileLabel(profile));
```

## First read

This complete sketch connects to `192.168.250.100:1025` and reads `D100` once per second.

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
    const slmp::Error err = slmp::highlevel::readTyped(plc, kProfile, "D100", value);
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

Use a test-only register that your PLC program does not use for control decisions. This example writes `1234` to `D9000` and then reads it back.

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
            "D9000",
            slmp::highlevel::Value::u16Value(1234U));
        Serial.printf("write D9000: %s\n", slmp::errorString(writeErr));
        wroteOnce = (writeErr == slmp::Error::Ok);
    }

    slmp::highlevel::Value value;
    const slmp::Error readErr = slmp::highlevel::readTyped(plc, kProfile, "D9000", value);
    if (readErr == slmp::Error::Ok) {
        Serial.printf("D9000=%u\n", static_cast<unsigned>(value.u16));
    }

    delay(1000);
}
```

## Confirm success

1. The board joins the same network as your PLC.
2. The serial monitor prints `PLC connected`.
3. `D100` prints a stable value or a value you expect from the PLC.
4. The write test uses a safe address reserved for bring-up.
5. A write followed by a read returns the value you wrote.

## If it does not work

| Symptom | Check |
| --- | --- |
| `connect()` fails | Your board must provide a `WiFiClient` or `EthernetClient` compatible transport, and the PLC must listen on TCP port `1025`. |
| High-level helpers are undefined | Add `#include <slmp_high_level.h>`; it is not included automatically. |
| Address parsing fails | Check that your `slmp::highlevel::PlcProfile` matches your actual hardware. |
| `X` or `Y` looks wrong | Use the profile-aware overloads of `slmp::highlevel::readTyped` and `slmp::highlevel::writeTyped`. |

## Next pages

| Page | Use it when |
| --- | --- |
| [Usage guide](USAGE_GUIDE.md) | You want typed reads, named snapshots, block reads, polling, or low-level calls. |
| [Supported registers](SUPPORTED_REGISTERS.md) | You need the public device families, value types, and address forms. |
