# ESP32-DevKitC Size Comparison

- Date: 2026-05-02
- Board: `esp32dev` / ESP32-DevKitC
- Goal: compare the binary-size cost of enabling the optional high-level facade on the same transport, board, and compiler settings
- PlatformIO platform: `espressif32` `6.10.0`
- Arduino framework package: `framework-arduinoespressif32` `3.20017.241212+sha.dcc1105b`
- Toolchain: `toolchain-xtensa-esp32` `8.4.0+2021r2-patch5`

## Build Commands

```bash
pio run -e esp32-devkitc-low-level
pio run -e esp32-devkitc-high-level
pio run -e esp32-devkitc-low-level-no-udp
pio run -e esp32-devkitc-high-level-no-udp
```

## Sample Roles

- `examples/esp32_devkitc_low_level`: direct fixed-buffer `SlmpClient` usage only
- `examples/esp32_devkitc_high_level`: same board and transport, but with `slmp_high_level.cpp`, explicit frame/mode selection, `connect`, `readTyped`, and `Poller`

## Measured Output

| Sample | UDP helper | Flash bytes | RAM bytes |
| --- | --- | ---: | ---: |
| `esp32-devkitc-low-level` | enabled | 749909 | 45064 |
| `esp32-devkitc-low-level-no-udp` | disabled | 749909 | 45064 |
| `esp32-devkitc-high-level` | enabled | 777525 | 45184 |
| `esp32-devkitc-high-level-no-udp` | disabled | 777525 | 45184 |

## Delta

- High-level facade, UDP enabled: Flash `+27616` bytes, RAM `+120` bytes
- Low-level UDP disabled vs enabled: Flash `+0` bytes, RAM `+0` bytes
- High-level UDP disabled vs enabled: Flash `+0` bytes, RAM `+0` bytes

## Interpretation

- The low-level sample is the size baseline for firmware that wants the fixed-buffer core only.
- The high-level sample shows the incremental cost of string address parsing, typed helpers, mixed snapshots, and the polling facade.
- On these maintained ESP32 Wi-Fi samples, disabling `SLMP_ENABLE_UDP_TRANSPORT` does not reduce the final image. The samples instantiate `WiFiClient` TCP transport only; the Arduino WiFi library still appears in the link graph, including `WiFiUdp.cpp`, and the UDP helper class itself is not retained in the final binary.
- This delta is small enough for many ESP32-class application builds, but the low-level-only path remains available for tighter image budgets.

## Remaining Hardware Validation

This report covers build-size impact only. Live validation of `ArduinoUdpTransport`
on real boards remains open until an actual UDP board/PLC run is captured.
