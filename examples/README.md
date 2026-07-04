# Examples

## What is in this directory

This directory contains the maintained minimal examples for `slmp-connect-cpp-minimal`. Use the ESP32 examples for board uploads and `high_level_snapshot` as a host-side compile smoke sample for the high-level API.

Use only test addresses that are safe for your PLC program before you run any write example.

## How to run

| Folder | Command | Notes |
| --- | --- | --- |
| `esp32_devkitc_low_level` | `pio run -e esp32-devkitc-low-level -t upload` | Uploads the smallest direct `slmp::SlmpClient` TCP example to ESP32-DevKitC. |
| `esp32_devkitc_high_level` | `pio run -e esp32-devkitc-high-level -t upload` | Uploads the high-level `slmp::highlevel` TCP example to ESP32-DevKitC. |
| `esp32_devkitc_polling_reconnect` | `pio run -e esp32-devkitc-polling-reconnect -t upload` | Uploads the read-only UDP polling reconnect example to ESP32-DevKitC. |
| `wiznet_w6300_evb_pico2_polling_reconnect` | `pio run -e wiznet-w6300-evb-pico2-polling-reconnect -t upload` | Uploads the read-only UDP polling reconnect example to W6300-EVB-Pico2. |
| `high_level_snapshot` | `g++ -std=c++17 -Isrc examples/high_level_snapshot/high_level_snapshot.cpp src/slmp_minimal.cpp src/slmp_high_level.cpp -o high_level_snapshot` | Host compile-only sample; it does not have a PlatformIO upload target. |

Optional TCP-only size comparison builds:

```bash
pio run -e esp32-devkitc-low-level-no-udp
pio run -e esp32-devkitc-high-level-no-udp
```

For the reconnect example, keep credentials out of the source file by passing
compile-time values when you build:

```bash
pio run -e esp32-devkitc-polling-reconnect -t upload --project-option "build_flags=-Isrc -DSLMP_WIFI_SSID=\"YOUR_WIFI_SSID\" -DSLMP_WIFI_PASSWORD=\"YOUR_WIFI_PASSWORD\" -DSLMP_PLC_HOST=\"192.168.250.100\" -DSLMP_PLC_PORT=1035"
```

For W6300-EVB-Pico2, the reconnect example uses wired Ethernet and defaults to
static IP `192.168.250.201/24`, PLC `192.168.250.100:1035`, and `D100:U`.
Override the network settings with build flags when needed:

```bash
pio run -e wiznet-w6300-evb-pico2-polling-reconnect -t upload --project-option "build_flags=-Isrc -DSLMP_LOCAL_IP=\"192.168.250.201\" -DSLMP_GATEWAY_IP=\"192.168.250.1\" -DSLMP_SUBNET_MASK=\"255.255.255.0\" -DSLMP_DNS_IP=\"192.168.250.1\" -DSLMP_PLC_HOST=\"192.168.250.100\" -DSLMP_PLC_PORT=1035"
```

## Example index

| Folder | What it demonstrates |
| --- | --- |
| `esp32_devkitc_low_level` | ESP32 `WiFiClient` transport setup, fixed buffers, explicit `slmp::PlcProfile::IqR`, `slmp::SlmpClient::readOneWord`, and `slmp::SlmpClient::readOneBit`. |
| `esp32_devkitc_high_level` | ESP32 `WiFiClient` transport setup, explicit `slmp::highlevel::PlcProfile::IqR`, `slmp::highlevel::configureClientForPlcProfile`, `slmp::highlevel::readTyped`, and `slmp::highlevel::Poller`. |
| `esp32_devkitc_polling_reconnect` | Read-only UDP `D100` polling with `connected` / `lost` / `reconnecting` / `recovered` logs and exponential reconnect backoff. |
| `wiznet_w6300_evb_pico2_polling_reconnect` | W6300-EVB-Pico2 wired Ethernet, read-only UDP `D100` polling, and reconnect state logs for live cable-pull evidence. |
| `high_level_snapshot` | Host-side examples of `slmp::highlevel::readTyped`, `slmp::highlevel::readNamed`, `slmp::highlevel::writeNamed`, and `slmp::highlevel::Poller` without requiring a board. |

## Related pages

| Page | Use it for |
| --- | --- |
| [Getting started](../docsrc/user/GETTING_STARTED.md) | First connection, first read, and first write. |
| [Usage guide](../docsrc/user/USAGE_GUIDE.md) | High-level and low-level API patterns. |
| [Gotchas](../docsrc/user/GOTCHAS.md) | Common address, profile, and value-type mistakes. |
