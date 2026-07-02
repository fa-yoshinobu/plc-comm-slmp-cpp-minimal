# Examples

## What is in this directory

This directory contains the maintained minimal examples for `slmp-connect-cpp-minimal`. Use the ESP32 examples for board uploads and `high_level_snapshot` as a host-side compile smoke sample for the high-level API.

## How to run

| Folder | Command | Notes |
| --- | --- | --- |
| `esp32_devkitc_low_level` | `pio run -e esp32-devkitc-low-level -t upload` | Uploads the smallest direct `slmp::SlmpClient` TCP example to ESP32-DevKitC. |
| `esp32_devkitc_high_level` | `pio run -e esp32-devkitc-high-level -t upload` | Uploads the high-level `slmp::highlevel` TCP example to ESP32-DevKitC. |
| `high_level_snapshot` | `g++ -std=c++17 -Isrc examples/high_level_snapshot/high_level_snapshot.cpp src/slmp_minimal.cpp src/slmp_high_level.cpp -o high_level_snapshot` | Host compile-only sample; it does not have a PlatformIO upload target. |

Optional TCP-only size comparison builds:

```bash
pio run -e esp32-devkitc-low-level-no-udp
pio run -e esp32-devkitc-high-level-no-udp
```

## Example index

| Folder | What it demonstrates |
| --- | --- |
| `esp32_devkitc_low_level` | ESP32 `WiFiClient` transport setup, fixed buffers, explicit `slmp::PlcProfile::IqR`, `slmp::SlmpClient::readOneWord`, and `slmp::SlmpClient::readOneBit`. |
| `esp32_devkitc_high_level` | ESP32 `WiFiClient` transport setup, explicit `slmp::highlevel::PlcProfile::IqR`, `slmp::highlevel::configureClientForPlcProfile`, `slmp::highlevel::readTyped`, and `slmp::highlevel::Poller`. |
| `high_level_snapshot` | Host-side examples of `slmp::highlevel::readTyped`, `slmp::highlevel::readNamed`, `slmp::highlevel::writeNamed`, and `slmp::highlevel::Poller` without requiring a board. |

## Related pages

| Page | Use it for |
| --- | --- |
| [Getting started](../docsrc/user/GETTING_STARTED.md) | First connection, first read, and first write. |
| [Usage guide](../docsrc/user/USAGE_GUIDE.md) | High-level and low-level API patterns. |
| [Gotchas](../docsrc/user/GOTCHAS.md) | Common address, profile, and value-type mistakes. |
