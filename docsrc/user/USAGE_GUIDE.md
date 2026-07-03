# Usage guide

## Two layers

| Layer | Header | Use it when |
| --- | --- | --- |
| Low-level core | `slmp_minimal.h` | You want fixed caller-owned buffers, explicit `slmp::DeviceAddress` values, and direct sync or async SLMP calls. |
| High-level facade | `slmp_high_level.h` | You want string addresses, typed values, named snapshots, chunked reads, and reusable polling plans. |

The high-level layer is optional. Include `slmp_high_level.h` explicitly when you use helpers such as `slmp::highlevel::readTyped`.

## Setup

This complete TCP setup creates the transport, buffers, `slmp::SlmpClient`, and profile configuration.
For UDP transport, keep the same host `192.168.250.100` and use UDP port `1035`.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
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
    if (plc.connect(kPlcHost, kTcpPort)) {
        Serial.println("PLC connected");
    }
}

void loop() {
    delay(1000);
}
```

## Strict profile capability guard

`SlmpClient` enables strict built-in Ethernet profile guards by default. The guard is separate from PLC end-code handling: when an implemented high-level route is marked `blocked` or `unverified` for the selected profile, the call returns `slmp::Error::ProfileFeatureBlocked` before transport and no PLC response is consumed.

Use `hasLastProfileFeatureErrorInfo()` and `lastProfileFeatureErrorInfo()` to inspect the profile, feature key, state, evidence, and bypass hint. Call `setStrictProfile(false)` only for deliberate probe traffic where you want the PLC to answer directly. Point limits and write policy are still enforced while strict profile is disabled.

```cpp
slmp::highlevel::configureClientForPlcProfile(
    plc,
    slmp::highlevel::PlcProfile::QnUDV);

slmp::DeviceBlockRead block{};
block.device = slmp::dev::D(slmp::dev::dec(0));
block.points = 1U;
uint16_t word = 0U;

const slmp::Error err = plc.readBlock(&block, 1U, nullptr, 0U, &word, 1U, nullptr, 0U);
if (err == slmp::Error::ProfileFeatureBlocked && plc.hasLastProfileFeatureErrorInfo()) {
    const slmp::ProfileFeatureErrorInfo& info = plc.lastProfileFeatureErrorInfo();
    Serial.printf("%s %s %s\n", info.profile_id, info.feature_key, info.state);
}
```

## Read a single value

`slmp::highlevel::readTyped` reads one logical value from one address.

| Address form | Value type | Field to read |
| --- | --- | --- |
| `D100:U` | `slmp::highlevel::ValueType::U16` | `value.u16` |
| `D100:S` | `slmp::highlevel::ValueType::S16` | `value.s16` |
| `D200:D` | `slmp::highlevel::ValueType::U32` | `value.u32` |
| `D200:L` | `slmp::highlevel::ValueType::S32` | `value.s32` |
| `D300:F` | `slmp::highlevel::ValueType::Float32` | `value.f32` |
| `M1000:BIT` or `D50.3` | `slmp::highlevel::ValueType::Bit` | `value.bit` |

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
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
    plc.connect(kPlcHost, kTcpPort);
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

## Write a single value

`slmp::highlevel::writeTyped` writes one logical value. Use only test addresses while you are bringing up your PLC connection.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
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
    plc.connect(kPlcHost, kTcpPort);
}

void loop() {
    if (!wroteOnce) {
        const slmp::Error err = slmp::highlevel::writeTyped(
            plc,
            kProfile,
            "D9000:U",
            slmp::highlevel::Value::u16Value(321U));
        Serial.printf("write D9000: %s\n", slmp::errorString(err));
        wroteOnce = (err == slmp::Error::Ok);
    }
    delay(1000);
}
```

## Named snapshot

`slmp::highlevel::readNamed` reads mixed addresses in caller order. `slmp::highlevel::writeNamed` writes an ordered list of address/value pairs.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <string>
#include <vector>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
constexpr auto kProfile = slmp::highlevel::PlcProfile::IqR;

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);
uint8_t txBuffer[192] = {};
uint8_t rxBuffer[192] = {};
slmp::SlmpClient plc(transport, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));
bool wroteOnce = false;

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    slmp::highlevel::configureClientForPlcProfile(plc, kProfile);
    plc.connect(kPlcHost, kTcpPort);
}

void loop() {
    if (!wroteOnce) {
        slmp::highlevel::Snapshot updates = {
            {"D9000:U", slmp::highlevel::Value::u16Value(100U)},
            {"D9002:S", slmp::highlevel::Value::s16Value(-10)},
            {"D9004:L", slmp::highlevel::Value::s32Value(-123456)},
            {"D9008:F", slmp::highlevel::Value::float32Value(12.5f)}
        };
        const slmp::Error writeErr = slmp::highlevel::writeNamed(plc, kProfile, updates);
        Serial.printf("writeNamed: %s\n", slmp::errorString(writeErr));
        wroteOnce = (writeErr == slmp::Error::Ok);
    }

    const std::vector<std::string> addresses = {
        "SM400:BIT",
        "D100:U",
        "D101:S",
        "D200:F",
        "D50.3"
    };

    slmp::highlevel::Snapshot snapshot;
    const slmp::Error readErr = slmp::highlevel::readNamed(plc, kProfile, addresses, snapshot);
    if (readErr == slmp::Error::Ok && snapshot.size() == addresses.size()) {
        Serial.printf(
            "SM400:BIT=%u D100:U=%u D101:S=%d D200:F=%.3f D50.3=%u\n",
            snapshot[0].value.bit ? 1U : 0U,
            static_cast<unsigned>(snapshot[1].value.u16),
            static_cast<int>(snapshot[2].value.s16),
            static_cast<double>(snapshot[3].value.f32),
            snapshot[4].value.bit ? 1U : 0U);
    }

    delay(1000);
}
```

## Block reads

Use `slmp::highlevel::readWordsChunked` when you intentionally allow a large contiguous word read to split into multiple low-level requests.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <vector>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
constexpr auto kProfile = slmp::highlevel::PlcProfile::IqR;

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);
uint8_t txBuffer[192] = {};
uint8_t rxBuffer[2048] = {};
slmp::SlmpClient plc(transport, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    slmp::highlevel::configureClientForPlcProfile(plc, kProfile);
    plc.connect(kPlcHost, kTcpPort);
}

void loop() {
    std::vector<uint16_t> words;
    const slmp::Error err = slmp::highlevel::readWordsChunked(
        plc,
        "D1000",
        1200,
        words,
        960,
        true);
    Serial.printf("readWordsChunked: %s count=%u\n", slmp::errorString(err), static_cast<unsigned>(words.size()));
    delay(5000);
}
```

## Polling

`slmp::highlevel::Poller` stores one compiled `slmp::highlevel::ReadPlan` so repeated reads do not re-parse the address strings.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <string>
#include <vector>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
constexpr auto kProfile = slmp::highlevel::PlcProfile::IqR;

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);
uint8_t txBuffer[192] = {};
uint8_t rxBuffer[192] = {};
slmp::SlmpClient plc(transport, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));
slmp::highlevel::Poller poller;

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    slmp::highlevel::configureClientForPlcProfile(plc, kProfile);
    plc.connect(kPlcHost, kTcpPort);
    poller.compile({"D100:U", "D101:S", "D200:F", "M1000:BIT"}, kProfile);
}

void loop() {
    slmp::highlevel::Snapshot snapshot;
    const slmp::Error err = poller.readOnce(plc, snapshot);
    Serial.printf("poller: %s values=%u\n", slmp::errorString(err), static_cast<unsigned>(snapshot.size()));
    delay(1000);
}
```

## Device range catalog

`slmp::highlevel::readDeviceRangeCatalogForPlcProfile` reads live device range bounds for one explicit profile. It requires your selected PLC profile and does not auto-discover the PLC profile.

```cpp
#include <Arduino.h>
#include <WiFi.h>

#include <slmp_arduino_transport.h>
#include <slmp_high_level.h>
#include <slmp_minimal.h>

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kTcpPort = 1025;
constexpr auto kProfile = slmp::highlevel::PlcProfile::IqR;

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);
uint8_t txBuffer[192] = {};
uint8_t rxBuffer[192] = {};
slmp::SlmpClient plc(transport, txBuffer, sizeof(txBuffer), rxBuffer, sizeof(rxBuffer));
bool printedCatalog = false;

void setup() {
    Serial.begin(115200);
    WiFi.begin(kWifiSsid, kWifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    slmp::highlevel::configureClientForPlcProfile(plc, kProfile);
    plc.connect(kPlcHost, kTcpPort);
}

void loop() {
    if (!printedCatalog) {
        slmp::highlevel::DeviceRangeCatalog catalog;
        const slmp::Error err = slmp::highlevel::readDeviceRangeCatalogForPlcProfile(plc, kProfile, catalog);
        if (err == slmp::Error::Ok && !catalog.entries.empty()) {
            const slmp::highlevel::DeviceRangeEntry& entry = catalog.entries.front();
            Serial.printf(
                "%s supported=%u range=%s\n",
                entry.device.c_str(),
                entry.supported ? 1U : 0U,
                entry.address_range.c_str());
        } else {
            Serial.printf("catalog failed: %s\n", slmp::errorString(err));
        }
        printedCatalog = true;
    }
    delay(1000);
}
```

## Long device families

`LTN`, `LSTN`, `LCN`, and `LZ` are 32-bit families in the high-level API. Use `slmp::highlevel::ValueType::U32` or `slmp::highlevel::ValueType::S32`.

| Family | Unsigned form | Signed form | Caution |
| --- | --- | --- | --- |
| `LTN` | `LTN0:D` | `LTN0:L` | Plain 16-bit access yields wrong data. |
| `LSTN` | `LSTN0:D` | `LSTN0:L` | Plain 16-bit access yields wrong data. |
| `LCN` | `LCN0:D` | `LCN0:L` | Plain 16-bit access yields wrong data. |
| `LZ` | `LZ0:D` | `LZ0:L` | Plain 16-bit access yields wrong data. |

## Address reference table

| Form | Meaning | Example |
| --- | --- | --- |
| `:U` | Unsigned 16-bit word. | `D100:U` |
| `:S` | Signed 16-bit word. | `D100:S` |
| `:D` | Unsigned 32-bit value from two words. | `D200:D` |
| `:L` | Signed 32-bit value from two words. | `D200:L` |
| `:F` | IEEE-754 float32 from two words. | `D300:F` |
| `:BIT` | Direct bit device value. | `M1000:BIT` |
| `.n` | One bit inside a word device, where `n` is hexadecimal `0` through `F`. | `D50.3` |

Named addresses used with `readTyped(address)`, `readNamed`, `writeNamed`, and `Poller` must include the intended type, for example `D100:U` or `M1000:BIT`.
