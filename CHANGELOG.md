# Changelog

## Unreleased

- no unreleased entries yet

## 0.1.0

- initial public minimal C++ release for ESP32 and RP2040 class boards
- TCP + SLMP 4E binary core with direct, random, and block device access
- direct helper coverage for word, bit, and dword single-shot and multi-point access
- remote password lock and unlock support
- typed device helpers for decimal and hexadecimal device families
- reconnect helper and config-driven ESP32 session example
- Python-compatible golden request fixtures and host-side mock transport tests
- real TCP socket integration tests against a local mock PLC
- mock PLC server with sample state file and fault injection for delay, forced PLC end code, disconnect, and malformed response
- GitHub Actions for host tests, sanitizers, coverage, size regression, and representative PlatformIO builds
- release automation, API policy, troubleshooting notes, and release checklist documents
