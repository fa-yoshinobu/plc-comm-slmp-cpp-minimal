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

## [1.0.0] - 2026-06-24

### Added
- Tests: Added 4 missing RD device encoding entries (`rd0_iqr`, `rd0_legacy`, `rd524287_iqr`, `rd524287_legacy`) to `tests/generated_shared_spec.h`.
- Tests: Added `read_words_rd524286_2_iqr` frame golden vector to `tests/generated_shared_spec.h`.

### Changed
- Release: Bumped PlatformIO and Arduino library metadata to `1.0.0` for the first stable release line.

### Fixed
- Tests: Fixed `testSharedGoldenFrames` dispatch in `tests/slmp_minimal_tests.cpp` to use `RD(524286)` for the new frame case instead of hardcoded `D(100)`.
