# TODO: SLMP C++ Minimal

This file tracks the remaining tasks and issues for the SLMP C++ Minimal library.

## 1. Validation and Platform Coverage
- [ ] **3E Validation Expansion**: Extend the current 3E verification beyond the existing simulator-focused coverage to additional PLC targets.
- [ ] **UDP Transport Validation**: Validate `ArduinoUdpTransport` on real boards and capture the binary-size impact of `SLMP_ENABLE_UDP_TRANSPORT`.
- [ ] **Board Matrix**: Confirm representative ESP32 / RP2040 / RP2350 builds with the current `slmp::SlmpClient` API.

## 2. Testing and CI
- [ ] **PlatformIO Test Flow**: Add on-device tests for the transport layer and representative read/write paths.
- [ ] **Board CI**: Add CI coverage for the board matrix and size-sensitive builds.

## 3. Packaging and Documentation
- [ ] **Library Export Rules**: Update `library.json` / `library.properties` so repo-only docs and `TODO.md` are excluded from distribution packages.
- [ ] **User Guide Refresh**: Update the user docs to explain the canonical `slmp::SlmpClient` name and optional UDP transport.

## 4. Known Limitations
- Linked direct devices (`Jn\Xn` style) are not supported yet.
