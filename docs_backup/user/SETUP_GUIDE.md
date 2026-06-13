# Setup Guide: SLMP C++ Minimal

This guide explains how to install and configure the SLMP C++ Minimal library for the maintained sample targets.

## 1. Supported Boards

The library is board-agnostic and works with any Arduino-compatible `Client` implementation.

Maintained sample targets in this repository:
- **ESP32-DevKitC** (`ESP32`, `WiFiClient`) for direct low-level vs optional high-level size comparison
- **Host compile smoke** (`high_level_snapshot`) for the helper API surface

Interactive console applications now live in the companion repository:

- <https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal-console-app>

## 2. Installation

### Using Arduino IDE
1. Download the latest release zip (`slmp-connect-cpp-minimal-vX.X.X.zip`).
2. In Arduino IDE, go to `Sketch -> Include Library -> Add .ZIP Library...`.
3. The library will now be available under `File -> Examples -> SLMP Connect C++ Minimal`.

### Using PlatformIO
1. Add the library to your project by cloning this repository into your `lib/` folder:
   ```bash
   git clone https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal.git lib/slmp-connect-cpp-minimal
   ```
2. Or reference it in your `platformio.ini` (once published to a registry).
3. Include the necessary headers in your code:
   ```cpp
   #include <slmp_minimal.h>
   #include <slmp_arduino_transport.h>
   ```

## 3. Network Hardware Setup

### ESP32-DevKitC (Wi-Fi)
Use `examples/esp32_devkitc_low_level` when you want the smallest direct `SlmpClient` sample.
Use `examples/esp32_devkitc_high_level` when you want the convenience-oriented helper surface on the same board.

Recommended comparison commands:

```bash
pio run -e esp32-devkitc-low-level
pio run -e esp32-devkitc-high-level
```

## 4. Initial Connection Check

The most reliable way to verify your setup in this repository is to start from the smallest maintained sample:
1. Connect your board to the same network as the PLC.
2. Open `examples/esp32_devkitc_low_level` or `examples/esp32_devkitc_high_level`.
3. Update the IP address and port (default 1025) to match your PLC.
4. Set one explicit `PlcFamily` in the high-level sample or set the low-level `frame` and `compatibility mode` explicitly.
5. Build and flash the sketch, then confirm one known device read succeeds.

If you need interactive serial consoles or the W6300 Ethernet bring-up tooling, use the companion console-app repository instead.

## 5. First High-Level Smoke Test

After transport bring-up works, the fastest application-level smoke test is:

1. Set one explicit `PlcFamily` for the target PLC.
2. Connect with `plc.connect(...)`.
3. Read one known word such as `D100`.
4. Read one mixed snapshot such as `{"SM400", "D100", "D200:F"}`.
5. If needed, write one safe test register such as `D9000`.

Minimal sequence:

```cpp
#include <slmp_high_level.h>

constexpr auto family = slmp::highlevel::PlcFamily::IqR;
slmp::highlevel::configureClientForPlcFamily(plc, family);

slmp::TypeNameInfo info = {};
if (plc.connect("192.168.250.100", 1025)) {
    plc.readTypeName(info);
    slmp::highlevel::Value value;
    slmp::highlevel::readTyped(plc, family, "D100", value);

    slmp::highlevel::Snapshot snapshot;
    slmp::highlevel::readNamed(plc, family, {"SM400", "D100", "D200:F"}, snapshot);
}
```

This confirms:

- transport connectivity
- explicit `PlcFamily` selection and derived frame/compatibility defaults
- typed decoding
- mixed snapshot handling
