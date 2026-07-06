# v2.0.0

## BREAKING

Short `slmp::module_io` aliases were removed in favor of canonical module I/O names.

| Old name | New name |
| --- | --- |
| `ControlCpu`, `ConnectedCpu`, `Default` | `OwnStation` |
| `ActiveCpu` | `ControlSystemCpu` |
| `StandbyCpu` | `StandbySystemCpu` |
| `TypeACpu` | `SystemACpu` |
| `TypeBCpu` | `SystemBCpu` |
| `Cpu1` to `Cpu4` | `MultipleCpu1` to `MultipleCpu4` |

## Package Name

| Registry | Package |
| --- | --- |
| PlatformIO | `fa-yoshinobu/slmp-connect-cpp-minimal` unchanged |

The PlatformIO name is retained because it is already published under that registry identity.

## Highlights

- PlatformIO and Arduino metadata bumped to 2.0.0.
- Install examples now use `fa-yoshinobu/slmp-connect-cpp-minimal@^2.0.0`.
- README links to the plc-comm package matrix.

Package matrix: https://fa-yoshinobu.github.io/plc-comm-docs-site/package-matrix/
