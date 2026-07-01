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

### Changed
- Library: Added SLMP `S` step relay device-code support for reads and rejected direct, random, block, and extended writes to `S` as read-only.
- Library: Rejected `G/HG` extended random bit writes; callers should use U-qualified word access for buffer-memory devices.
- Library: Aligned high-level long counter state metadata so `LCS/LCC` remain long-helper entries while using their direct bit-read route internally.
- Library: Applied the same read-only and qualified-only device guards to low-level link-direct writes.
- Docs: Documented `S` as a read-only bit device in supported-register, gotcha, and audit-reflection notes.
- Docs: Fixed PowerShell placeholder text in maintainer publishing notes.
- Tests: Added guard coverage for `S` read-only writes, high-level `S10:BIT` parsing, `G/HG` random bit write rejection, and low-level link-direct write rejection.

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
