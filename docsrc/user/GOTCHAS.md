# Gotchas

## `connect()` or transport I/O fails

| Item | Detail |
| --- | --- |
| Symptom | `connect()` fails, a transport call times out, or `TransportError` is returned before a valid SLMP response is parsed. |
| Root cause | The PLC IP address, port, transport path, or local network route does not match the target. |
| Fix | Check the PLC endpoint first. The usual SLMP TCP port is `1025`; use UDP only when the PLC is configured for UDP. Increase the timeout only after the endpoint and wiring are known-good. |

## `BufferTooSmall` is returned

| Item | Detail |
| --- | --- |
| Symptom | A request fails locally with `BufferTooSmall`. |
| Root cause | The caller-owned TX/RX buffers are too small for the selected command and point count. |
| Fix | Increase the TX buffer first for block write, random write, or long password commands. Small direct reads and writes usually fit in `96` bytes; random and block access often need `192..256` bytes. |

## `ProtocolError` is returned

| Item | Detail |
| --- | --- |
| Symptom | Bytes are received, but the response is rejected as an invalid SLMP frame. |
| Root cause | The target endpoint is not speaking the expected SLMP binary frame, or the response length does not match the request. |
| Fix | Verify the PLC frame setting and inspect `lastRequestFrame()` / `lastResponseFrame()` with `formatHexBytes()`. |

## GX Simulator 3 uses port 5511 for the default local target

| Item | Detail |
| --- | --- |
| Symptom | A local GX Works3 simulator connection fails on the hardware default port. |
| Root cause | GX Simulator 3 uses a simulator port, not the hardware SLMP port. For System `1`, PLC `1`, the port is `5511`. |
| Fix | Connect to `127.0.0.1:5511` for the default GX Works3 simulator target. GX Simulator 2 uses a proprietary protocol and is not supported by this SLMP library. |

## LTN/LSTN/LCN/LZ reads return unexpected values

| Item | Detail |
| --- | --- |
| Symptom | Long timer, long counter, or long index values look truncated or offset. |
| Root cause | `LTN`, `LSTN`, `LCN`, and `LZ` are 32-bit logical families. |
| Fix | Read them with `:D` for unsigned 32-bit or `:L` for signed 32-bit. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>
#include <slmp_minimal.h>

class NoopTransport final : public slmp::ITransport {
  public:
    bool connect(const char*, uint16_t) override { return false; }
    void close() override {}
    bool connected() const override { return false; }
    bool writeAll(const uint8_t*, size_t) override { return false; }
    bool readExact(uint8_t*, size_t, uint32_t) override { return false; }
    size_t write(const uint8_t*, size_t) override { return 0U; }
    size_t read(uint8_t*, size_t) override { return 0U; }
    size_t available() override { return 0U; }
};

int main() {
    NoopTransport transport;
    uint8_t tx[128] = {};
    uint8_t rx[128] = {};
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));
    constexpr auto profile = slmp::highlevel::PlcProfile::IqR;
    slmp::highlevel::configureClientForPlcProfile(plc, profile);

    slmp::highlevel::Value value;
    const slmp::Error err = slmp::highlevel::readTyped(plc, profile, "LTN0:D", value);
    return (err == slmp::Error::Ok || err == slmp::Error::NotConnected) ? 0 : 1;
}
```

## Mixed block write is rejected by the PLC

| Item | Detail |
| --- | --- |
| Symptom | A block write that mixes word devices and bit devices fails. |
| Root cause | SLMP command `0x1406` does not accept word and bit blocks in one request on the target PLC path. |
| Fix | Send word writes and bit writes as separate calls. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_minimal.h>

class NoopTransport final : public slmp::ITransport {
  public:
    bool connect(const char*, uint16_t) override { return false; }
    void close() override {}
    bool connected() const override { return false; }
    bool writeAll(const uint8_t*, size_t) override { return false; }
    bool readExact(uint8_t*, size_t, uint32_t) override { return false; }
    size_t write(const uint8_t*, size_t) override { return 0U; }
    size_t read(uint8_t*, size_t) override { return 0U; }
    size_t available() override { return 0U; }
};

int main() {
    NoopTransport transport;
    uint8_t tx[128] = {};
    uint8_t rx[128] = {};
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));
    plc.setFrameType(slmp::FrameType::Frame4E);
    plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);

    const slmp::Error wordErr = plc.writeOneWord(slmp::dev::D(slmp::dev::dec(9000)), 1234U);
    const slmp::Error bitErr = plc.writeOneBit(slmp::dev::M(slmp::dev::dec(9000)), true);
    return (wordErr == slmp::Error::NotConnected && bitErr == slmp::Error::NotConnected) ? 0 : 1;
}
```

## G or HG address fails

| Item | Detail |
| --- | --- |
| Symptom | A normal high-level address such as `G100` or `HG100` does not work as an ordinary device. |
| Root cause | Module buffer memory is an extended SLMP target, not a normal high-level direct device family. |
| Fix | Use `slmp::SlmpClient` module-buffer APIs such as `readWordsModuleBuf`. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_minimal.h>

class NoopTransport final : public slmp::ITransport {
  public:
    bool connect(const char*, uint16_t) override { return false; }
    void close() override {}
    bool connected() const override { return false; }
    bool writeAll(const uint8_t*, size_t) override { return false; }
    bool readExact(uint8_t*, size_t, uint32_t) override { return false; }
    size_t write(const uint8_t*, size_t) override { return 0U; }
    size_t read(uint8_t*, size_t) override { return 0U; }
    size_t available() override { return 0U; }
};

int main() {
    NoopTransport transport;
    uint8_t tx[128] = {};
    uint8_t rx[128] = {};
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));
    plc.setFrameType(slmp::FrameType::Frame4E);
    plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);

    uint16_t words[4] = {};
    const slmp::Error err = plc.readWordsModuleBuf(3, false, 100, 4, words, 4);
    return err == slmp::Error::NotConnected ? 0 : 1;
}
```

## DX or DY fails on iQ-F

| Item | Detail |
| --- | --- |
| Symptom | `DX` or `DY` is rejected when the selected profile is iQ-F. |
| Root cause | The iQ-F profile does not support `DX` or `DY` in the high-level parser. |
| Fix | Use `X` and `Y` with `slmp::highlevel::PlcProfile::IqF`. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>

int main() {
    slmp::highlevel::AddressSpec input{};
    const slmp::Error err = slmp::highlevel::parseAddressSpec(
        "X20",
        slmp::highlevel::PlcProfile::IqF,
        input);
    return err == slmp::Error::Ok ? 0 : 1;
}
```

## Read or write fails because no PLC profile was set

| Item | Detail |
| --- | --- |
| Symptom | Profile-sensitive addressing, frame type, or compatibility mode does not match your PLC. |
| Root cause | The helper layer does not probe the PLC and does not infer the profile from model text. |
| Fix | Choose a concrete `slmp::highlevel::PlcProfile` and call `configureClientForPlcProfile` before communication. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>
#include <slmp_minimal.h>

class NoopTransport final : public slmp::ITransport {
  public:
    bool connect(const char*, uint16_t) override { return false; }
    void close() override {}
    bool connected() const override { return false; }
    bool writeAll(const uint8_t*, size_t) override { return false; }
    bool readExact(uint8_t*, size_t, uint32_t) override { return false; }
    size_t write(const uint8_t*, size_t) override { return 0U; }
    size_t read(uint8_t*, size_t) override { return 0U; }
    size_t available() override { return 0U; }
};

int main() {
    NoopTransport transport;
    uint8_t tx[64] = {};
    uint8_t rx[64] = {};
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));

    constexpr auto profile = slmp::highlevel::PlcProfile::IqR;
    slmp::highlevel::configureClientForPlcProfile(plc, profile);
    return plc.compatibilityMode() == slmp::CompatibilityMode::iQR ? 0 : 1;
}
```

## High-level helpers are not found by the compiler

| Item | Detail |
| --- | --- |
| Symptom | Names such as `slmp::highlevel::readTyped` or `slmp::highlevel::Poller` are undefined. |
| Root cause | `slmp_high_level.h` is optional and is not included by `slmp_minimal.h`. |
| Fix | Include `slmp_high_level.h` explicitly in any file that uses the high-level facade. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>

int main() {
    slmp::highlevel::AddressSpec spec{};
    const slmp::Error err = slmp::highlevel::parseAddressSpec(
        "D100",
        slmp::highlevel::PlcProfile::IqR,
        spec);
    return err == slmp::Error::Ok ? 0 : 1;
}
```

## Unspecified profile returns an immediate error

| Item | Detail |
| --- | --- |
| Symptom | Profile-aware helpers return `slmp::Error::InvalidArgument` before any PLC request is sent. |
| Root cause | `slmp::highlevel::PlcProfile::Unspecified` is not a concrete PLC profile. |
| Fix | Pass one concrete profile such as `slmp::highlevel::PlcProfile::IqR`. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>

int main() {
    slmp::highlevel::AddressSpec spec{};
    const slmp::Error bad = slmp::highlevel::parseAddressSpec(
        "D100",
        slmp::highlevel::PlcProfile::Unspecified,
        spec);
    const slmp::Error good = slmp::highlevel::parseAddressSpec(
        "D100",
        slmp::highlevel::PlcProfile::IqR,
        spec);
    return (bad == slmp::Error::InvalidArgument && good == slmp::Error::Ok) ? 0 : 1;
}
```

## X or Y string address is rejected

| Item | Detail |
| --- | --- |
| Symptom | A string address such as `X20` is rejected or maps to the wrong point. |
| Root cause | `X` and `Y` numbering depends on the PLC profile; iQ-F uses octal, while the other supported profiles use hexadecimal. |
| Fix | Use the profile-aware overloads of the high-level helpers. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>

int main() {
    slmp::highlevel::AddressSpec spec{};
    const slmp::Error err = slmp::highlevel::parseAddressSpec(
        "X20",
        slmp::highlevel::PlcProfile::IqR,
        spec);
    return err == slmp::Error::Ok ? 0 : 1;
}
```

## D50.D reads bit 13 instead of a 32-bit value

| Item | Detail |
| --- | --- |
| Symptom | `D50.D` behaves like one bit rather than a double word. |
| Root cause | Dot notation is bit-in-word access; `D` after the dot is hexadecimal bit index 13. |
| Fix | Use `D50:D` for unsigned 32-bit data and reserve `D50.D` for bit 13. |

```cpp
#include <cstddef>
#include <cstdint>

#include <slmp_high_level.h>

int main() {
    slmp::highlevel::AddressSpec dword{};
    slmp::highlevel::AddressSpec bit13{};
    const slmp::Error dwordErr = slmp::highlevel::parseAddressSpec(
        "D50:D",
        slmp::highlevel::PlcProfile::IqR,
        dword);
    const slmp::Error bitErr = slmp::highlevel::parseAddressSpec(
        "D50.D",
        slmp::highlevel::PlcProfile::IqR,
        bit13);
    return (dwordErr == slmp::Error::Ok && bitErr == slmp::Error::Ok) ? 0 : 1;
}
```
