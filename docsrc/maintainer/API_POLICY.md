# API Policy

This library is still in the `0.x` phase, but the public API is being stabilized deliberately.

Public API surface:

- `src/slmp_minimal.h`
- `src/slmp_arduino_transport.h`
- `src/slmp_utility.h`

Stability intent:

- additive changes are preferred
- source-compatible changes are preferred over renames
- behavior changes should be accompanied by host tests and changelog entries

What may still change before `1.0.0`:

- naming of newer helper APIs
- example sketch structure
- CI and test utilities
- library metadata and packaging details

What should remain stable unless there is a strong reason:

- `slmp::Error`
- `slmp::DeviceCode`
- `slmp::DeviceAddress`
- `slmp::SlmpClient` core read/write API
- `slmp::endCodeString()`
- `slmp::formatHexBytes()`

Breaking changes policy:

- document them in `CHANGELOG.md`
- update examples and host tests in the same change
- keep the old form for at least one `0.x` release when practical
