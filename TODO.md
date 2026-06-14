# TODO: SLMP C++ Minimal

This file tracks the remaining tasks and issues for the SLMP C++ Minimal library.

## 1. Validation and Platform Coverage
- [x] **No active validation follow-up**: No blocking live validation item is open right now.
- [x] **UDP disabled binary-size impact**: Captured `SLMP_ENABLE_UDP_TRANSPORT=0` ESP32-DevKitC low-level/high-level builds on 2026-05-02. The maintained TCP-only Wi-Fi samples showed Flash `+0` bytes and RAM `+0` bytes versus UDP-enabled builds.

## 2. Known Limitations
- None in the current public scope.

## 3. Cross-Stack API Alignment

- [x] **Finalize `PlcProfile` naming alignment**: The public high-level PLC selector is `PlcProfile`, frame type and compatibility mode are derived from that profile on the standard route, and displayed labels use canonical profiles such as `melsec:iq-r`. C++ uses typed selectors rather than string aliases.

