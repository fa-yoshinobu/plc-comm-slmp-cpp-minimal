# API Policy

This library has a stable `3.x` public API and follows Semantic Versioning.

Public API surface:

- `src/slmp_minimal.h`
- `src/slmp_high_level.h`
- `src/slmp_arduino_transport.h`
- `src/slmp_error_codes.h`
- `src/slmp_utility.h`

Stability intent:

- additive changes are preferred
- source-compatible changes are preferred over renames
- behavior changes should be accompanied by host tests and changelog entries
- deprecated public names remain available for at least one minor release when practical
- breaking public API changes require a new major version

The example sketch structure, CI/test utilities, maintainer documentation, and packaging implementation are not public API. Changes to them still require the relevant build or packaging checks.

What should remain stable unless there is a strong reason:

- `slmp::Error`
- `slmp::DeviceCode`
- `slmp::DeviceAddress`
- `slmp::SlmpClient` core read/write API
- `slmp::endCodeString()`
- `slmp::isRemotePasswordEndCode()`
- `slmp::formatHexBytes()`

Breaking changes policy:

- document them in `CHANGELOG.md`
- update examples and host tests in the same change
- use a major-version release unless the old form can remain as a source-compatible deprecation
