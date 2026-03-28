# Usage Guide: SLMP C++ Minimal

This guide starts with the optional high-level helper API, then covers the lower-level core API for advanced embedded scenarios.

## Design Philosophy

The library intentionally keeps two user-facing layers:

- High-level helpers for user code that wants string addresses, typed values, mixed snapshots, and reusable polling plans.
- Low-level core APIs for firmware that wants explicit buffers, deterministic memory usage, and direct control over sync or async communication flow.

Recommended rule:

- Start with the high-level helper layer unless you have a clear embedded reason not to.
- Drop to the low-level core when buffer ownership, manual scheduling, or transport-level control matters more than convenience.

## 1. Transport Setup

The library decouples protocol logic from the network transport. For Arduino environments, you can use TCP or UDP.

### TCP Transport (Recommended)
```cpp
#include <slmp_arduino_transport.h>

WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);
```

### UDP Transport
```cpp
#include <slmp_arduino_transport.h>

WiFiUDP udp;
slmp::ArduinoUdpTransport transport(udp);
```

## 2. High-Level Helpers

The optional high-level layer is the recommended entry point for application code.

```cpp
#include <slmp_high_level.h>

plc.setFrameType(slmp::FrameType::Frame4E);
plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);

slmp::TypeNameInfo info = {};
if (plc.connect("192.168.250.100", 1025)) {
    plc.readTypeName(info);

    slmp::highlevel::Value word;
    slmp::highlevel::readTyped(plc, "D100", word);

    slmp::highlevel::Value temperature;
    slmp::highlevel::readTyped(plc, "D200:F", temperature);

    slmp::highlevel::Snapshot snapshot;
    slmp::highlevel::readNamed(plc, {"SM400", "D100", "D200:F", "D50.3"}, snapshot);
}
```

Use cases:

- `readTyped` / `writeTyped`: single logical value by string address
- `readNamed` / `writeNamed`: mixed snapshot by string address list
- `Poller`: compile one snapshot plan once and reuse it for repeated monitoring
- `readWordsChunked` / `readDWordsChunked`: large contiguous reads with optional split

Important notes:

- `.bit` notation is valid only for word devices such as `D50.3`
- direct bit devices should be addressed directly, for example `M1000`
- the optional high-level layer uses `std::string` and `std::vector`
- the core client in `slmp_minimal.h` stays fixed-buffer and allocation-free

### Address Syntax Cheat Sheet

| Form | Meaning | Example result type |
| --- | --- | --- |
| `D100` | default 16-bit unsigned word | `U16` |
| `D100:S` | signed 16-bit word | `S16` |
| `D200:D` | unsigned 32-bit dword | `U32` |
| `D200:L` | signed 32-bit dword | `S32` |
| `D300:F` | float32 | `Float32` |
| `D50.3` | one bit inside a word device | `Bit` |
| `M1000` | direct bit device | `Bit` |

Invalid form:

- `M1000.0`

Direct bit devices should be addressed directly. `.bit` is reserved for word devices such as `D50.3`.

### High-Level Write Example

```cpp
slmp::highlevel::writeTyped(plc, "D100", slmp::highlevel::Value::u16Value(321));
slmp::highlevel::writeTyped(plc, "D200:F", slmp::highlevel::Value::float32Value(12.5f));
slmp::highlevel::writeTyped(plc, "D50.3", slmp::highlevel::Value::bitValue(true));
```

### High-Level Mixed Snapshot Example

```cpp
slmp::highlevel::Snapshot snapshot;
if (slmp::highlevel::readNamed(
        plc,
        {"SM400", "D100", "D101:S", "D200:F", "D50.3"},
        snapshot) == slmp::Error::Ok) {
    // snapshot keeps caller order
}
```

### High-Level Mixed Write Example

```cpp
slmp::highlevel::Snapshot updates = {
    {"D100", slmp::highlevel::Value::u16Value(321)},
    {"D200:F", slmp::highlevel::Value::float32Value(12.5f)},
    {"D50.3", slmp::highlevel::Value::bitValue(true)},
};
slmp::highlevel::writeNamed(plc, updates);
```

### Poller Example

```cpp
slmp::highlevel::Poller poller;
poller.compile({"D100", "D101:S", "D200:F", "M1000"});

slmp::highlevel::Snapshot snapshot;
if (poller.readOnce(plc, snapshot) == slmp::Error::Ok) {
    // Reuse the compiled plan on every cycle
}
```

### Chunked Contiguous Read Example

```cpp
std::vector<uint16_t> words;
slmp::highlevel::readWordsChunked(plc, "D1000", 1200, words, 960, true);

std::vector<uint32_t> dwords;
slmp::highlevel::readDWordsChunked(plc, "D2000", 600, dwords, 480, true);
```

## 3. Basic Synchronous Usage

Synchronous methods block execution until a response is received or a timeout occurs. Best for simple scripts or setup routines.

```cpp
// Initialize Client with fixed TX/RX buffers (No dynamic allocation)
uint8_t tx_buf[128];
uint8_t rx_buf[128];
slmp::SlmpClient plc(transport, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));

void setup() {
    // Open connection to PLC
    plc.connect("192.168.1.10", 1025);
}

void loop() {
    // Read 2 words from D100 using the fluent API
    uint16_t words[2] = {0};
    auto dev = slmp::dev::D(slmp::dev::dec(100));
    
    if (plc.readWords(dev, 2, words, 2) == slmp::Error::Ok) {
        // words[0] contains D100, words[1] contains D101
    }
    delay(1000);
}
```

Use the low-level synchronous API when you want:

- full control over caller-owned buffers
- explicit `DeviceAddress` handling instead of string parsing
- no STL usage in the hot path
- the smallest predictable runtime surface

## 4. Asynchronous (Non-blocking) Usage

Essential for real-time control loops. Asynchronous communication allows the microcontroller to handle UI, sensors, or other tasks while waiting for the PLC.

### Workflow
1.  Start an operation with a `beginXXX()` method.
2.  Call `plc.update(millis())` in your main `loop()`.
3.  Check `plc.isBusy()` to monitor progress.
4.  Handle results once `isBusy()` returns false.

### Example
```cpp
uint16_t data[10];
bool active = false;

void loop() {
    uint32_t now = millis();
    
    if (!active) {
        // Request 10 words from D100
        auto dev = slmp::dev::D(slmp::dev::dec(100));
        if (plc.beginReadWords(dev, 10, data, 10, now) == slmp::Error::Ok) {
            active = true;
        }
    }

    // Must call update() every loop to process transport and timeouts
    plc.update(now);

    if (active && !plc.isBusy()) {
        if (plc.lastError() == slmp::Error::Ok) {
            // Data successfully received in data[]
        }
        active = false; // Ready for next request
    }
}
```

Use the low-level async API when you want:

- cooperative scheduling from `loop()`
- no blocking waits in firmware
- one transport session integrated into a custom state machine
- deterministic ownership of request and response buffers

## 5. Extended Device Access

Access module buffer (`U..\G`, `U..\HG`) or link direct (`J..\device`) targets using the **Extended Device** specification.

### Module Buffer Access
```cpp
uint16_t buf[4] = {};
// Read 4 words from Slot 3, Buffer Memory G100
plc.readWordsModuleBuf(3, false, 100, 4, buf, 4);
```

### Link Direct Device (CC-Link IE Field)
```cpp
uint16_t words[1] = {};
// Read J2\SW10 (Network 2, Link Special Register 0x10)
plc.readWordsLinkDirect(2, slmp::DeviceCode::SW, 0x10, 1, words, 1);

bool bits[16] = {};
// Read J1\X10 (Network 1, Input 0x10)
plc.readBitsLinkDirect(1, slmp::DeviceCode::X, 0x10, 16, bits, 16);
```

### Random Read/Write with Extended Devices
```cpp
slmp::ExtDeviceSpec specs[] = {
    slmp::ExtDeviceSpec::moduleBuf(3, false, 100),   // U3\G100
    slmp::ExtDeviceSpec::linkDirect(2, slmp::DeviceCode::SW, 0x10), // J2\SW10
};
uint16_t values[2] = {};
plc.readRandomExt(specs, 2, values, 2, nullptr, 0, nullptr, 0);
```

## 6. Advanced Features

### 3E/4E and Compatibility Modes
Default is **Frame 4E + iQR mode**. To support older hardware (e.g., Q-series):
```cpp
plc.setFrameType(slmp::FrameType::Frame3E);
plc.setCompatibilityMode(slmp::CompatibilityMode::Legacy);
```

Choose the frame and compatibility mode explicitly before `connect()`. The library does not probe profiles automatically.

### Monitor Cycle (0x0801 / 0x0802)
Register a list once to reduce overhead, then poll repeatedly:
```cpp
slmp::DeviceAddress devs[] = {{slmp::DeviceCode::D, 100}, {slmp::DeviceCode::D, 200}};
plc.registerMonitorDevices(devs, 2, nullptr, 0);

// In loop:
uint16_t results[2] = {};
plc.runMonitorCycle(results, 2, nullptr, 0);
```

## 7. Memory Considerations

The core client is designed for zero dynamic allocation (`malloc`/`new`).
- **Buffer Sizes**: Your `tx_buffer` and `rx_buffer` must be large enough for the largest request/response.
- **Small Requests**: 64-128 bytes are usually sufficient.
- **Large Writes / Random Access**: Recommend 256-512 bytes depending on point counts.
- **Optional High-Level Layer**: `slmp_high_level.h` uses STL containers for convenience and should be treated as an additive facade on top of the fixed-buffer core.
- **Compile-Time Opt-Out**: Define `SLMP_MINIMAL_ENABLE_HIGH_LEVEL=0` if you do not need the high-level helpers and want to keep them out of the build.
