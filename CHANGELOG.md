# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Entry labels**

- `Release`: Package/version metadata and publishing preparation.
- `Library`: Runtime behavior, public API, protocol handling, or validation in the distributed library.
- `Docs`: README, user guides, generated API docs, or other documentation-only changes.
- `Samples`: Examples, sample flows, sample scripts, or sample applications.
- `Tests`: Test suites, test fixtures, golden vectors, or verification data.
- `Tooling`: Developer/operator command-line tools and helper utilities.
- `CI`: Release checks, workflow scripts, or automation-only changes.

## [Unreleased]

## [3.0.0] - 2026-07-10

### Changed
- Release: Bumped `library.json` and synchronized `library.properties` metadata to `3.0.0`.

### BREAKING
- Library: Breaking: `SlmpClient::setPlcProfile`, `setManualProfile`, and `highlevel::configureClientForPlcProfile` now return `Error` so invalid profiles are reported without silently resetting state.
- Migration: Check for `Error::Ok` after each profile-setting call. Invalid profiles leave the prior client configuration unchanged; callers must handle `Error::InvalidArgument` before starting communication.

### Added
- Library: Added `plcProfileCanonicalName`, `parsePlcProfile`, and `availablePlcProfiles` to the high-level profile API.

### Docs
- Docs: Clarified canonical profile names, display names, device-range labels, and connection-selectable profiles.

## [2.0.0] - 2026-07-06

### BREAKING
- Library: Removed short `slmp::module_io` aliases in favor of the canonical module I/O vocabulary.

| Removed name | Use instead |
| --- | --- |
| `ControlCpu`, `ConnectedCpu`, `Default` | `OwnStation` |
| `ActiveCpu` | `ControlSystemCpu` |
| `StandbyCpu` | `StandbySystemCpu` |
| `TypeACpu` | `SystemACpu` |
| `TypeBCpu` | `SystemBCpu` |
| `Cpu1` to `Cpu4` | `MultipleCpu1` to `MultipleCpu4` |

### Changed
- Release: Bumped PlatformIO and Arduino metadata to `2.0.0`; the PlatformIO package name remains `fa-yoshinobu/slmp-connect-cpp-minimal` because it is already published under that name.
- Library: Added `slmp::module_io` named constants for multi-CPU target routing while keeping `TargetAddress{}` on the default own station.
- Library: Synced the embedded SLMP capability fixture to `plc-comm-slmp-profiles` `v1.2.2`.
- Docs: Added the plc-comm family package matrix link to the README and updated PlatformIO install examples to `@^2.0.0`.
- Tooling: Changed the canonical profile update script default ref to `v1.2.2`.

## [1.2.1] - 2026-07-05

### Changed
- Release: Bumped package metadata to `1.2.1`.
- Tooling: Added release metadata synchronization from `library.json` so mirrored Arduino metadata stays aligned before release checks.
- CI: Added PlatformIO package packing and content checks to prevent development files or build outputs from entering published packages.
- CI: Added an optional PlatformIO registry publish path for release workflow dispatches using `PLATFORMIO_AUTH_TOKEN`.

## [1.2.0] - 2026-07-05

### Changed
- Release: Bumped package metadata to `1.2.0`.
- Tooling: Normalized line-ending handling in the canonical profile JSON update script so `-SourceRoot` runs no longer report false changes.
- Library: Synced the embedded SLMP capability fixture to `plc-comm-slmp-profiles` `v1.2.1`, including `display_name` labels and Ethernet unit profiles for RJ71EN71, LJ71E71-100, and QJ71E71-100 variants.
- Library: Added Ethernet-unit `PlcProfile` enum values and `slmp::highlevel::plcProfileDisplayName(profile)` for UI labels while keeping stored PLC profile values canonical.
- Docs: Documented the profile display-name helper and canonical-ID storage guidance.
- Tests: Added canonical fixture parity coverage for profile `display_name` values.
- Library: Added non-breaking SLMP specification-audit updates for point-limit guards and PLC error diagnostics.
- Library: Exposed structured PLC error information through `hasLastErrorInfo()` and `lastErrorInfo()` when a non-zero end-code response carries the 9-byte error information block.
- Library: Embedded the `plc-comm-slmp-profiles` `v1.1.0` built-in Ethernet capability table as static arrays and added strict profile guards to implemented high-level feature routes.
- Library: Added `Error::ProfileFeatureBlocked`, `setStrictProfile()`, `strictProfile()`, `hasLastProfileFeatureErrorInfo()`, and `lastProfileFeatureErrorInfo()` for profile guard diagnostics without using PLC end-code errors.
- Library: Enforced documented point limits before transport: iQ-F direct bit access is limited to 3584 points, and 008x extended random/monitor routes use the 96-point / weighted-960 / 94-bit limits.
- Library: Replaced series-only direct/random/monitor point limits with profile capability limits where available; limits remain enforced when strict profile is disabled.
- Library: Added canonical weighted random-word write limits for `melsec:iq-l` and `melsec:iq-f`, so mixed word/dword random writes are guarded before transport.
- Library: Added SLMP `S` step relay device-code support for reads and profile-specific write policy enforcement.
- Library: Enforced capability write policies independently of strict profile; `S` is read-only on iQ-R/iQ-L/MX/Q/L profiles and read-write on iQ-F.
- Library: Rejected profile-unsupported device families before transport while leaving device address upper-bound checks to application/live-probe code.
- Tooling: Changed the canonical profile update script default ref from `v1.0.0` to `v1.1.0`.
- Library: Rejected `G/HG` extended random bit writes; callers should use U-qualified word access for buffer-memory devices.
- Library: Aligned high-level long counter state metadata so `LCS/LCC` remain long-helper entries while using their direct bit-read route internally.
- Library: Applied the same read-only and qualified-only device guards to low-level link-direct writes.
- Library: Moved PLC profile selection into the core `SlmpClient` and moved Q/L profile block rejection to strict profile feature guards for all canonical Q/L profiles.
- Library: Rejected all transport sends when `SlmpClient` remains on `PlcProfile::Unspecified`; callers must select a canonical PLC profile before communication.
- Library: Made high-level communication overloads without an explicit profile use `client.plcProfile()` and reject unspecified clients before address parsing.
- Library: Made profile-less high-level read-plan compile overloads return `Error::InvalidArgument`; callers must compile plans with an explicit PLC profile.
- Library: Treat invalid `PlcProfile` enum values as unspecified instead of falling back to QnUDV defaults.
- Library: Batched named plain-bit reads through random word-read only for `SM/X/Y/M/L/F/V/B/SB`; `TS/TC/STS/STC/CS/CC/DX/DY` stay on direct bit reads.
- Docs: Documented profile-specific `S` write policy in supported-register, gotcha, and audit-reflection notes.
- Docs: Documented strict profile behavior, applied feature keys, and APIs intentionally outside the capability-feature guard.
- Docs: Removed the duplicated SLMP supported-register user page and linked users to the shared SLMP Profile Reference.
- Docs: Added a Usage Guide example showing how to read `lastEndCode()` and structured `lastErrorInfo()`.
- Docs: Added Usage Guide examples for `U...` module access, `U...HG` CPU-buffer access, and `J...` link direct extended devices.
- Docs: Removed the manual page-navigation block from Getting Started and rely on site navigation instead.
- Docs: Moved shared SLMP gotcha items to the common troubleshooting page and kept Gotchas focused on C++ minimal-specific behavior.
- Docs: Slimmed Gotchas to library-specific items and moved shared setup/end-code symptoms to the PLC Setup Guide.
- Docs: Standardized the Gotchas page structure with KV Host Link so library-specific caveats have the same destination across protocols.
- Docs: Clarified the ESP32/RP2040-class target focus while retaining Arduino-compatible transport naming for those cores.
- Docs: Added generated Doxygen-based API reference for the public C++ headers, with CI freshness validation.
- Docs: Fixed PowerShell placeholder text in maintainer publishing notes.
- Docs: Cleaned up maintainer notes and normalized the root TODO.
- Samples: Updated the low-level ESP32 sample to select `slmp::PlcProfile::IqR` instead of manually pairing frame type and compatibility mode.
- Tooling: Removed local absolute fallback tool paths from `run_ci.bat`.
- Tooling: Changed the canonical profile update script default ref from `main` to fixed tag `v1.0.0`; `SLMP_PROFILES_REF` can still override it.
- Tests: Added guard coverage for `S` read-only writes, high-level `S10:BIT` parsing, `G/HG` random bit write rejection, and low-level link-direct write rejection.
- Tests: Updated low-level C++ tests and socket integration tests to use canonical PLC profiles instead of manual frame/compatibility send paths.
- Tests: Added canonical capability fixture snapshot and capability profile guard coverage for block, type-name, monitor, link-direct, HG CPU-buffer, point-limit, and write-policy behavior.
- Tests: Added high-level unspecified-profile regression coverage for typed read/write, named read/write, and poller compile paths.
- Tests: Added invalid profile enum regression coverage for low-level sends and high-level defaults/configuration.
- Tests: Added named-read planning coverage for random-word-safe plain bit families versus the direct-bit-only families seen on R-series hardware.

## [1.1.1] - 2026-06-29

### Changed
- Release: Bumped PlatformIO and Arduino library metadata to `1.1.1`.
- Docs: Documented explicit address-string value-type requirements in user docs and public header comments.
- Samples: Updated high-level examples to use explicit value-type suffixes.

## [1.1.0] - 2026-06-29

### Fixed
- Library: Avoid overwriting the immediately previous `endCodeString()` result on consecutive calls, and add `formatEndCodeString()` for caller-owned output buffers.

### Changed
- Release: Bumped PlatformIO and Arduino library metadata to `1.1.0`.
- Library: Made high-level address parsing require explicit value type suffixes such as `:U`, `:S`, `:D`, `:L`, `:F`, or `:BIT`; bare devices no longer default to unsigned word, bit, or long-timer dword interpretations.
- Library: Removed embedded localized SLMP end-code message text; end-code helpers now return stable code-derived keys while message lookup hooks return `nullptr`.
- Docs: Updated maintainer notes and build configuration for the end-code key helper contract.

### Tests
- Tests: Updated generated shared-spec vectors and high-level tests for explicit dtype requirements.
- Tests: Updated SLMP end-code helper coverage for code-derived keys and non-embedded messages.

## [1.0.0] - 2026-06-24

### Added
- Tests: Added 4 missing RD device encoding entries (`rd0_iqr`, `rd0_legacy`, `rd524287_iqr`, `rd524287_legacy`) to `tests/generated_shared_spec.h`.
- Tests: Added `read_words_rd524286_2_iqr` frame golden vector to `tests/generated_shared_spec.h`.

### Changed
- Release: Bumped PlatformIO and Arduino library metadata to `1.0.0` for the first stable release line.

### Fixed
- Tests: Fixed `testSharedGoldenFrames` dispatch in `tests/slmp_minimal_tests.cpp` to use `RD(524286)` for the new frame case instead of hardcoded `D(100)`.
