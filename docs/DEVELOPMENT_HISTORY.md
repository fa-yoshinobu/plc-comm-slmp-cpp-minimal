# Development History

## 2026-06-11 Archived Refactor Plan

The previous `refactor-instructions.md` was archived into this history file.

### Scope

- Library: minimal C++ SLMP implementation for host and embedded builds.
- Primary task: make only small file-local internal improvements while preserving tests and embedded constraints.
- File splitting was explicitly forbidden for implementation and kept as a proposal-only topic.

### Contracts To Preserve

- All public headers, including `slmp_minimal.h`, `slmp_high_level.h`, and transport-facing APIs.
- Exact transmitted SLMP frame bytes.
- Core-layer no-heap-allocation design.
- Build flags such as `SLMP_ENABLE_UDP_TRANSPORT`.
- Existing `src/` file list, CI expectations, PlatformIO metadata, version `0.4.10`, and changelog.

### Debt Notes

- D1: long functions and duplicated helpers existed inside `slmp_minimal.cpp` and `slmp_high_level.cpp`; only same-file internal helper extraction was allowed.
- D2: file splitting was prohibited in this task and recorded as a future proposal only.

### Recorded Result

- `git status --short`: clean at the start of the recorded work.
- Host build with `g++ -std=c++17 -Wall -Wextra ...`: succeeded.
- Host tests: `slmp_minimal_tests: ok`; 33 named test functions.
- Socket integration via `python scripts/run_socket_integration.py --compiler g++`: succeeded for `normal`, `plc_error`, `disconnect`, `delay`, and `malformed`.
- PlatformIO was not run because `pio` was unavailable locally.
- ASan/UBSan build was not completed because `-lasan` and `-lubsan` were unavailable.

### Implemented Change

- Added internal helper `readAddressSpecValue` inside `src/slmp_high_level.cpp`.
- Consolidated duplicated parsed-address read handling from two `readTypedImpl` paths.
- Changed only `src/slmp_high_level.cpp`; no public headers, build files, metadata, or frame-generation behavior were changed.

### Follow-Up Notes

- Direct/random/block payload encoding duplication in `src/slmp_minimal.cpp` remains a possible future improvement, but it is close to wire bytes and no-allocation constraints.
- File splitting would require coordinated CI, PlatformIO, and README design updates.
