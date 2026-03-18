# Setup Guide: SLMP C++ Minimal

This guide explains how to install and configure the SLMP C++ Minimal library for the maintained sample targets.

## 1. Supported Boards

The library is board-agnostic and works with any Arduino-compatible `Client` implementation.

Maintained sample targets in this repository:
- **M5Stack Atom Matrix** (`ESP32-PICO-D4`, `WiFiClient`)
- **W6300-EVB-Pico2** (`RP2350`, onboard W6300 via `W6300lwIP`)

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

### Atom Matrix (Wi-Fi)
Pass a `WiFiClient` instance to the library's transport layer and start from `examples/atom_matrix_serial_console`.

### W6300-EVB-Pico2 (Ethernet)
Use Arduino-Pico with `W6300lwIP` and start from `examples/w6300_evb_pico2_serial_console`.

## 4. Initial Connection Check

The most reliable way to verify your setup is to run one of the maintained serial console samples:
1. Connect your board to the same network as the PLC.
2. Open `atom_matrix_serial_console` for Wi-Fi or `w6300_evb_pico2_serial_console` for Ethernet.
3. Update the IP address and port (default 1025) to match your PLC.
4. Open the Serial Monitor at 115200 baud and run `type`, `funcheck`, or `bench`.
