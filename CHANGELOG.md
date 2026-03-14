# Changelog

## Unreleased

- add a `W5500-EVB-Pico2` example sketch and PlatformIO smoke target using Arduino-Pico `EthernetCompat`
- add an interactive serial debug console to the `W5500-EVB-Pico2` example for ad-hoc PLC reads, writes, and frame dumps
- add operator-validated write verification commands (`verifyw`, `verifyb`, `judge`) to the `W5500-EVB-Pico2` serial console
- update GitHub Actions workflow dependencies to current major versions
- replace the release publish action with GitHub CLI commands to avoid Node 20 deprecation warnings
- verify required files are present in the release zip before uploading assets
- fix publishing notes to match the current CI checks

## 0.1.1

- add release/install guidance and shorter board-specific start paths in `README.md`
- add `library.json` and publishing notes for future PlatformIO Registry packaging
- add hardware validation backlog documentation and a GitHub issue template
- add Markdown link checks to CI and document Arduino Library Manager submission as a manual gate while hardware validation is still pending

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
