# TODO: SLMP C++ Minimal

This file tracks the remaining tasks and issues for the SLMP C++ Minimal library.

## 1. Validation and Platform Coverage
- [ ] **UDP Transport Validation**: Validate `ArduinoUdpTransport` on real boards and capture the binary-size impact of `SLMP_ENABLE_UDP_TRANSPORT`.

## 2. Testing and CI
- None. Host tests, PlatformIO smoke builds, and documentation generation are already in place for the current scope.

## 3. Packaging and Documentation
- None. User docs and generated API comments now cover both the low-level core and the optional high-level facade.

## 4. Known Limitations
- None in the current public scope.

## 5. Cross-Stack API Alignment
- [x] **Keep the optional high-level facade aligned by concept**: The public helper vocabulary now stays centered on `readTyped`, `writeTyped`, `writeBitInWord`, `readNamed`, and the reusable poll-plan flow.
- [x] **Keep the minimal core minimal**: The user-facing helper layer remains in `slmp_high_level.h`; `slmp_minimal.h` stays buffer-oriented and allocation-free.
- [x] **Expose lightweight device parse/format helpers**: `normalizeAddress()` and `formatAddressSpec()` now round-trip user-facing addresses into one canonical uppercase spelling without forcing application code to duplicate device tables.
- [x] **Keep semantic-boundary rules explicit**: README, usage guide, examples, and header comments now keep the rule explicit that typed and named helpers preserve logical-value semantics, while segmentation belongs only to explicit chunked APIs or protocol-defined boundaries.


