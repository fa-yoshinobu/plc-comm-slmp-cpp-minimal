# Usage Guide: SLMP C++ Minimal

This guide covers basic and advanced usage of the library, including synchronous and asynchronous communication.

## 1. Basic Synchronous Usage

Synchronous methods block execution until a response is received or a timeout occurs.

```cpp
#include <slmp_arduino_transport.h>

// 1. Setup Transport
WiFiClient tcp;
slmp::ArduinoClientTransport transport(tcp);

// 2. Initialize Client with fixed TX/RX buffers
uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

void setup() {
    plc.connect("192.168.1.10", 1025);
}

void loop() {
    // Read 2 words from D100
    uint16_t words[2] = {0};
    if (plc.readWords({slmp::DeviceCode::D, 100}, 2, words, 2) == slmp::Error::Ok) {
        // Success
    }
    delay(1000);
}
```

## 2. Asynchronous (Non-blocking) Usage

Asynchronous communication allows the microcontroller to perform other tasks while waiting for the PLC.

### Workflow
1.  Start an operation with a `beginXXX()` method.
2.  Call `plc.update(millis())` in your main `loop()`.
3.  Check `plc.isBusy()` to monitor progress.
4.  Check `plc.lastError()` once finished.

### Example
```cpp
uint16_t data[10];
bool active = false;

void loop() {
    uint32_t now = millis();
    if (!active) {
        auto dev = slmp::dev::D(slmp::dev::dec(100));
        if (plc.beginReadWords(dev, 10, data, 10, now) == slmp::Error::Ok) {
            active = true;
        }
    }

    plc.update(now);

    if (active && !plc.isBusy()) {
        if (plc.lastError() == slmp::Error::Ok) {
            // Data is ready in data[]
        }
        active = false;
    }
}
```

## 3. Advanced Features

### 3E/4E Frame Selection
Default is **4E**. To use **3E** frames (e.g., for older Q-series):
```cpp
plc.setFrameType(slmp::FrameType::Frame3E);
```

### Random and Block Access
- **Random Read**: Get multiple non-contiguous devices in one request.
- **Block Read**: Efficiently read mixed word and bit blocks.
- *Note: Bit blocks use packed 16-bit words.*

For synchronous mixed `writeBlock()` calls, you can control the practical fallback behavior:

```cpp
const uint16_t word_values[] = {0x1234, 0x5678};
const uint16_t bit_values[] = {0x0005};

const slmp::DeviceBlockWrite word_blocks[] = {
    slmp::dev::blockWrite(slmp::dev::D(slmp::dev::dec(300)), word_values, 2),
};
const slmp::DeviceBlockWrite bit_blocks[] = {
    slmp::dev::blockWrite(slmp::dev::M(slmp::dev::dec(200)), bit_values, 1),
};

slmp::BlockWriteOptions options = {};
options.retry_mixed_on_error = true;

plc.writeBlock(word_blocks, 1, bit_blocks, 1, options);
```

Current rules:

- `split_mixed_blocks=true` sends separate word-only and bit-only `1406` requests immediately
- `retry_mixed_on_error=true` sends one mixed request first, then retries as separate writes only on known PLC rejection end codes
- the current retry set is `0xC056`, `0xC05B`, and `0xC061`
- the async `beginWriteBlock(..., options, now_ms)` overload mirrors the same split/retry behavior

### Password Security
Remote control helpers follow the same typed style as the Python client:

- `remoteRun(force, clear_mode)`
- `remoteStop()`
- `remotePause(force)`
- `remoteLatchClear()`
- `remoteReset(subcommand, expect_response)`
- `clearError()`

Self-test loopback is also available through the typed helper:

```cpp
const uint8_t loopback_in[] = {'A', 'B', 'C', 'D', 'E'};
uint8_t loopback_out[8] = {};
size_t loopback_out_length = 0;

plc.selfTestLoopback(
    loopback_in,
    sizeof(loopback_in),
    loopback_out,
    sizeof(loopback_out),
    loopback_out_length
);
```

Practical reset rule:

- `remoteReset()` defaults to `1006/0000` with no-response handling
- use `remoteReset(0x0001U, true)` only when the PLC is expected to return a normal completion frame

### Passive Profile Recommendation

If `readTypeName()` succeeds, you can derive a lightweight profile recommendation without active probing:

```cpp
slmp::TypeNameInfo info = {};
if (plc.readTypeName(info) == slmp::Error::Ok) {
    slmp::ProfileRecommendation recommendation = slmp::recommendProfile(info);
    if (recommendation.confident) {
        slmp::applyProfileRecommendation(plc, recommendation);
    }
}
```

Current recommendation rules:

- `model_code` is used first when available
- Q-series and legacy L-series recommend `Frame3E + Legacy`
- iQ-R, iQ-L, and `FX5*` names recommend `Frame4E + iQR`
- unknown models fall back to the library default (`Frame4E + iQR`) with `confident=false`

Use `remotePasswordUnlock(password)` and `remotePasswordLock(password)` for PLCs with enabled remote security.

## 4. Memory Model and Buffers

The library does not use dynamic allocation.
- **Small reads/writes**: 96 bytes buffer.
- **Large word writes**: 128 - 192 bytes.
- **Random/Block access**: 192 - 256 bytes.
