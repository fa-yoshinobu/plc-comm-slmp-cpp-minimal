# TODO: SLMP C++ Minimal

This file tracks the remaining tasks and issues for the SLMP C++ Minimal library.

## 1. Validation and Platform Coverage
- [ ] **UDP Transport live validation**: Validate `ArduinoUdpTransport` on real boards and capture the target board, PLC model, transport settings, and pass/fail evidence.
- [x] **UDP disabled binary-size impact**: Captured `SLMP_ENABLE_UDP_TRANSPORT=0` ESP32-DevKitC low-level/high-level builds on 2026-05-02. The maintained TCP-only Wi-Fi samples showed Flash `+0` bytes and RAM `+0` bytes versus UDP-enabled builds.

## 2. Known Limitations
- None in the current public scope.

