# Changelog

## Unreleased

## 0.4.5

- add device-range catalog helpers and CPU operation-state decoding helpers to the optional high-level facade
- expand host regression coverage and README guidance for the new device-range and CPU-state helpers

## 0.4.4

- add guard logic for unsupported long-timer direct reads and unsupported `LCS/LCC` random, block, and monitor-registration paths so the minimal client matches the shared cross-library consistency rules
- expand host regression coverage for the guarded paths and long-device command routing

## 0.4.3

- regenerate `tests/generated_shared_spec.h` from `plc-comm-slmp-cross-verify/specs/shared` so host tests and release builds consume the same canonical parity vectors as the cross-library verification harness

## 0.4.2

- add public `normalizeAddress()` and `formatAddressSpec()` helpers to the optional high-level facade so application code can canonicalize string device addresses without duplicating parser logic
- refresh README, usage guide, example guide, and API policy notes to describe the public address-normalization helpers and the no-hidden-split rule more directly

## 0.4.1

- republish the PlatformIO package after the console split so the registry metadata and package contents match the library-only layout
- keep console applications in the companion repository while the core package stays focused on the minimal library and maintained small examples

## 0.4.0

- add ESP32-DevKitC low-level and high-level Wi-Fi samples so binary size differences can be compared on the same board
- expand user docs with clearer high-level vs low-level guidance, address syntax notes, design philosophy, and ESP32 sample selection
- expand Doxygen comments for both `slmp_high_level.h` and `slmp_minimal.h` so generated API docs explain the intended split between convenience helpers and the fixed-buffer core
- add `docsrc/validation/reports/ESP32_DEVKITC_SIZE_COMPARISON.md` with measured flash/RAM deltas for the ESP32 low-level and high-level samples
- clean Doxygen-facing docs and console comments so generated documentation stays ASCII-only and avoids unresolved markdown references
- add optional `slmp_high_level.h` facade with string-address parsing, `readTyped`, `writeTyped`, `readNamed`, `writeNamed`, `Poller`, and compile-checked high-level sample coverage
- remove automatic profile recommendation and probing helpers; callers must choose frame type and compatibility mode explicitly before `connect()`
- remove Step Relay `S` from the public device enum, parser tables, and example device pickers; `TS/LTS/STS/LSTS/CS/LCS` remain supported
- add `release_check.bat` so release-preflight runs `run_ci.bat` and `build_docs.bat` together
- expand Doxygen comments for the public `SlmpClient` connection and synchronous helper APIs
- remake `w6300_evb_pico2_serial_console` using the high-level `SlmpClient` API; split into five files (`console_hw.h`, `console_watch.h`, `console_commands.h`, `console_tests.h`) for readability
- expand W6300 console limits: TX/RX buffers 512-2048 bytes, max points 8-64, random devices 8-16, block count 4-8, block points 16-64
- add watch mode to W6300 console: `watch [b|d] <dev> [pts] [ms]` polls devices continuously and marks changed values with `*`; `watch off` / `watch status` control the session
- add `loopback [text]` command to W6300 console (self-test 0619); aliased as `selftest`
- add short command aliases to W6300 console: `r`/`w`/`b` for word/word-write/bits, `rd`/`wd`/`wb` for dword variants, `scan` for fullscan
- add `readBitsModuleBuf` / `writeBitsModuleBuf` for contiguous bit access to module buffer devices (`U\G`); verified in cross-platform cross-verify suite (Python + .NET + C++)
- add `readRandomExt` / `writeRandomWordsExt` / `writeRandomBitsExt` and their `begin*` async variants for ExtendedDevice extended random access (module buffer `U\G`/`U\HG` and link direct `J\device`, subcommands `0x0403`/`0x1402`)
- add `registerMonitorDevices` / `registerMonitorDevicesExt` / `runMonitorCycle` and their `begin*` async variants for PLC monitor registration and execution (commands `0x0801`/`0x0802`)
- add `ExtDeviceSpec` struct with `moduleBuf()` and `linkDirect()` static factory methods for unified ExtendedDevice extended device addressing
- add `readExtendUnitWord` / `readExtendUnitDWord` / `writeExtendUnitWord` / `writeExtendUnitDWord` convenience helpers (extend unit 0x0601/0x1601)
- add `readCpuBufferWord` / `readCpuBufferDWord` / `writeCpuBufferWord` / `writeCpuBufferDWord` convenience helpers (CPU buffer via module `0x03E0`)
- add W6300-EVB-Pico2 console mode switching for TCP/UDP and SLMP 3E/4E from both serial commands and BOOTSEL button shortcuts
- add `BlockWriteOptions` for synchronous `writeBlock()` so mixed word+bit block writes can be split up front or retried automatically after known PLC rejection end codes
- add async mixed-block fallback parity through `beginWriteBlock(..., options, now_ms)`
- add typed remote control helpers for `1001/1002/1003/1005`
- add typed helpers for `1006` remote reset, `0619` self-test loopback, and `1617` clear error
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
