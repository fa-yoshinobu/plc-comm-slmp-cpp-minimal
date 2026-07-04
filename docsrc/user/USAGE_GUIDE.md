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

## Routing / target station

Most applications keep the default target, which means the directly connected
own station/control CPU. Change the target only when your PLC network is
configured for another station, multi-CPU module I/O, or multidrop access.

`slmp::TargetAddress` controls the SLMP destination header. It is not a device
family selector; routed devices such as `Un\Gn` and `Jn\...` still need their
own address syntax.

```cpp
slmp::TargetAddress target{};
target.network = 0x01;
target.station = 0x02;
target.module_io = 0x03FF;
target.multidrop = 0x00;
plc.setTarget(target);
```

Use the default target unless the PLC routing setup gives you specific values.

## Extended device access

`G`, `HG`, and `J` devices are not normal standalone addresses. The C++ high-level
facade is for normal device strings such as `D100:U`; use the low-level
`slmp::SlmpClient` APIs for extended device routes.

| Address form | Low-level call shape |
| --- | --- |
| `U3\G100` | `readWordsModuleBuf(3, false, 100, ...)` |
| `U3E0\HG0` | `readWordsModuleBuf(0x03E0, true, 0, ...)` |
| `J2\SW10` | `readWordsLinkDirect(2, slmp::DeviceCode::SW, 0x10, ...)` |
| `J1\X10` | `readBitsLinkDirect(1, slmp::DeviceCode::X, 0x10, ...)` |

The selected PLC profile and the actual PLC configuration still decide whether
the route is accepted.

```cpp
uint16_t moduleWords[4] = {};
slmp::Error err = plc.readWordsModuleBuf(3, false, 100, 4, moduleWords, 4);

const uint16_t moduleWrite[] = {1, 2, 3, 4};
err = plc.writeWordsModuleBuf(3, false, 100, moduleWrite, 4);

uint16_t cpuBufferWords[2] = {};
err = plc.readWordsModuleBuf(0x03E0, true, 0, 2, cpuBufferWords, 2);

uint16_t linkWords[1] = {};
err = plc.readWordsLinkDirect(2, slmp::DeviceCode::SW, 0x10, 1, linkWords, 1);

bool linkBits[16] = {};
err = plc.readBitsLinkDirect(1, slmp::DeviceCode::X, 0x10, 16, linkBits, 16);
```

For extended random access, build `slmp::ExtDeviceSpec` entries:

```cpp
const slmp::ExtDeviceSpec wordDevices[] = {
    slmp::ExtDeviceSpec::moduleBuf(3, false, 100),
    slmp::ExtDeviceSpec::linkDirect(2, slmp::DeviceCode::SW, 0x10),
};
uint16_t values[2] = {};
err = plc.readRandomExt(wordDevices, 2, values, 2, nullptr, 0, nullptr, 0);
```

## Strict profile

`SlmpClient` enables strict profile checks by default. With a selected profile, operations known to be unavailable for that PLC are rejected before sending.

Leave this enabled for normal applications. Call `setStrictProfile(false)` only for deliberate verification where you want the PLC to answer directly. Point limits and write policy still apply.

## Remote password

Remote password lock/unlock commands are available on the low-level `slmp::SlmpClient`.
The C++ high-level facade does not automatically unlock or lock a remote password.
If your PLC route uses remote password protection, unlock after connecting and lock before closing.

```cpp
slmp::Error err = plc.remotePasswordUnlock("secret");
if (err == slmp::Error::Ok) {
    slmp::highlevel::Value value;
    err = slmp::highlevel::readTyped(plc, kProfile, "D100:U", value);
    plc.remotePasswordLock("secret");
}
```

For `C200`-series password end codes, see the shared
[SLMP Troubleshooting & Codes](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/slmp/troubleshooting-codes/)
page.

## SLMP response end codes

When the PLC returns a non-zero SLMP end code, low-level calls return `slmp::Error::PlcError`.
Read `lastEndCode()` for the PLC response code and `lastErrorInfo()` when the PLC returned the structured error-information block.

```cpp
slmp::highlevel::Value value;
const slmp::Error err = slmp::highlevel::readTyped(plc, kProfile, "D100:U", value);
if (err == slmp::Error::PlcError) {
    Serial.printf("SLMP end_code=0x%04X\n", plc.lastEndCode());
    if (plc.hasLastErrorInfo()) {
        const slmp::SlmpErrorInfo& info = plc.lastErrorInfo();
        Serial.printf("command=0x%04X\n", info.command);
        Serial.printf("subcommand=0x%04X\n", info.subcommand);
    }
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
The catalog is for diagnostics and application-layer validation. Normal read/write helpers do not use it to reject addresses by configured upper bound before sending a request.
The source rules for this catalog are maintained in the shared [SLMP device ranges](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/device-ranges/) reference.

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
