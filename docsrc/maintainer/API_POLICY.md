# API Policy

This library has a stable `3.x` public API and follows Semantic Versioning.

Public API surface:

- `src/slmp_minimal.h`
- `src/slmp_high_level.h`
- `src/slmp_arduino_transport.h`
- `src/slmp_error_codes.h`
- `src/slmp_utility.h`

Stability intent:

- technically correct, explicit, and validated behavior takes priority over source compatibility
- additive or source-compatible changes are preferred only when they do not preserve ambiguity, silent fallback, or unsafe behavior
- behavior changes should be accompanied by host tests and changelog entries
- deprecated names remain only when an explicit review finds the compatibility path technically safe and worthwhile
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
- use a major-version release
- do not retain an alias or ignored option merely to avoid migration work
