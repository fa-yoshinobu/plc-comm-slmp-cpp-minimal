# Changelog

## Unreleased

- add W6300-EVB-Pico2 console mode switching for TCP/UDP and SLMP 3E/4E from both serial commands and BOOTSEL button shortcuts
- add `BlockWriteOptions` for synchronous `writeBlock()` so mixed word+bit block writes can be split up front or retried automatically after known PLC rejection end codes
- add async mixed-block fallback parity through `beginWriteBlock(..., options, now_ms)`
- add typed remote control helpers for `1001/1002/1003/1005`
- add typed helpers for `1006` remote reset, `0619` self-test loopback, and `1617` clear error
- add passive `recommendProfile(...)` helpers that derive `3E/4E` and `Legacy/iQR` recommendations from `model_code` or `type_name`
- extend `endCodeString()` with additional practical hardware codes including `0xC056`, `0xC201`, `0xC810`, and `0x414A`

## 0.3.0

- add support for SLMP 3E binary frames via `setFrameType()`
- validate 3E/4E cross-frame communication stability against GX Simulator 3 for all major device families
- fix buffer capacity checks and payload construction in `writeBlock()` and `readBlock()`
- update documentation with GX Simulator 3 connection rules and port numbering guidance

## 0.2.1

- fix the host-side dword request header tests so `writeOneDWord()` and `writeDWords()` expect the correct transmitted word count

## 0.2.0

- add W6300-EVB-Pico2, ESP32-C3, and Atom Matrix interactive console examples with shared direct, one-shot, random, block, target, password, and frame-dump command coverage
- add Atom Matrix hardware-specific demo, manual check, automated `funcheck`, `txlimit`, `bench`, `endurance`, and `reconnect` modes
- add PlatformIO smoke targets for the new console examples and document board-specific footprints and startup paths
- add host-side function test runner coverage for additional direct, dword, random, block, timeout, and target-setting API paths
- mark `LCS`, `LCC`, and `LCN` unsupported in the minimal client to match observed real-board behavior
- record real-board validation results for Atom Matrix against Mitsubishi iQ-R `R08CPU`, including reconnect, endurance, benchmark, and buffer-limit captures
- add Python comparison worksheet documentation for the remaining mixed block-write investigation
- add a `W5500-EVB-Pico2` example sketch and PlatformIO smoke target using Arduino-Pico `EthernetCompat`
- add an interactive serial debug console to the `W5500-EVB-Pico2` example for ad-hoc PLC reads, writes, and frame dumps
- add operator-validated write verification commands (`verifyw`, `verifyb`, `judge`) to the `W5500-EVB-Pico2` serial console
- add an Atom Matrix (`ESP32-PICO-D4`) interactive Wi-Fi debug console example and PlatformIO smoke target
- factor the ESP32 Wi-Fi interactive console so the same command surface can be reused across ESP32-C3 and Atom Matrix boards
- add Atom Matrix `demo` mode for button-driven `D0` increments and `M0..M24` mirroring on the 5x5 RGB matrix
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
