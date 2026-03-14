# Hardware Validation

This file tracks real-board coverage that is still pending after the host-side and mock-PLC release checks.

## Current Matrix

| Target | Transport | Bring-up sketch | Status | Notes |
|---|---|---|---|---|
| `esp32dev` | `WiFiClient` | `examples/esp32_read_words` | pending | verify Wi-Fi join, `connect()`, `readTypeName()`, direct read |
| `esp32dev` | `WiFiClient` | `examples/esp32_password_read_loop` | pending | verify reconnect path, password unlock, periodic poll |
| `esp32dev` | `WiFiClient` | `examples/esp32_random_block` | pending | verify random/block read and optional write path |
| `esp32dev` | `WiFiClient` | `examples/esp32_dynamic_bits` | pending | verify dynamic bit walk and odd-address write logic |
| `esp32-s3-devkitc-1` | `WiFiClient` | `platformio/esp32dev_smoke.cpp` | pending | compile verified, board runtime not yet verified |
| `esp32-c3-devkitm-1` | `WiFiClient` | `platformio/esp32dev_smoke.cpp` | pending | compile verified, board runtime not yet verified |
| `pico` + W5500 | `EthernetClient` | `examples/rp2040_w5500_read_words` | pending | verify SPI wiring, DHCP or static IP, direct read |
| `wiznet_5500_evb_pico2` | `EthernetClient` | `examples/w5500_evb_pico2_read_words` | pending | compile target added with Arduino-Pico core, board runtime not yet verified |
| `nanorp2040connect` | `WiFiNINA` | ESP32-style session flow | pending | compile verified, board runtime not yet verified |
| real Mitsubishi PLC | TCP | any supported sketch | pending | verify real end codes, timing, and access rules beyond mock server coverage |

## Suggested Issue Titles

- `hardware: validate esp32dev direct read against real PLC`
- `hardware: validate esp32dev reconnect and remote password flow`
- `hardware: validate rp2040 w5500 direct read example`
- `hardware: validate W5500-EVB-Pico2 direct read example`
- `hardware: validate nano rp2040 connect with WiFiNINA transport`
- `hardware: verify real PLC end-code behavior against documented strings`

## Suggested Capture Data

When running a board validation pass, record:

- board and core package version
- transport type and network setup
- PLC model and firmware if known
- sketch path and any local configuration changes
- observed request/response frame dump on failure
- whether the same scenario passed against `scripts/mock_plc_server.py`
