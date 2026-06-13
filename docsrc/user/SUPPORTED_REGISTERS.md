# Supported registers

This page lists the public device families used by the low-level and high-level APIs. Actual availability still depends on your PLC model, profile, and project settings.

## Bit device families

| Family | Meaning | Notes |
| --- | --- | --- |
| `SM` | Special relay | Decimal numbering. |
| `X` | Input | Profile-aware string parsing is required; iQ-F uses octal, other supported profiles use hexadecimal. |
| `Y` | Output | Profile-aware string parsing is required; iQ-F uses octal, other supported profiles use hexadecimal. |
| `M` | Internal relay | Decimal numbering. |
| `L` | Latch relay | Decimal numbering. |
| `F` | Annunciator | Low-level helper name is `slmp::dev::FDevice`. |
| `V` | Edge relay | Decimal numbering. |
| `B` | Link relay | Hexadecimal numbering. |
| `TS` | Timer contact | Decimal numbering. |
| `TC` | Timer coil | Decimal numbering. |
| `LTS` | Long timer contact | Long timer state family. |
| `LTC` | Long timer coil | Long timer state family. |
| `STS` | Retentive timer contact | Decimal numbering. |
| `STC` | Retentive timer coil | Decimal numbering. |
| `LSTS` | Long retentive timer contact | Long retentive timer state family. |
| `LSTC` | Long retentive timer coil | Long retentive timer state family. |
| `CS` | Counter contact | Decimal numbering. |
| `CC` | Counter coil | Decimal numbering. |
| `LCS` | Long counter contact | Long counter state family. |
| `LCC` | Long counter coil | Long counter state family. |
| `SB` | Link special relay | Hexadecimal numbering. |
| `DX` | Direct input | Not valid for `slmp::highlevel::PlcProfile::IqF`. |
| `DY` | Direct output | Not valid for `slmp::highlevel::PlcProfile::IqF`. |

## Word device families

| Family | Meaning | Notes |
| --- | --- | --- |
| `SD` | Special register | Decimal numbering. |
| `D` | Data register | Common 16-bit, 32-bit, and float storage family. |
| `W` | Link register | Hexadecimal numbering. |
| `TN` | Timer current value | Decimal numbering. |
| `LTN` | Long timer current value | Use `slmp::highlevel::ValueType::U32` or `slmp::highlevel::ValueType::S32`. |
| `STN` | Retentive timer current value | Decimal numbering. |
| `LSTN` | Long retentive timer current value | Use `slmp::highlevel::ValueType::U32` or `slmp::highlevel::ValueType::S32`. |
| `CN` | Counter current value | Decimal numbering. |
| `LCN` | Long counter current value | Use `slmp::highlevel::ValueType::U32` or `slmp::highlevel::ValueType::S32`. |
| `SW` | Link special register | Hexadecimal numbering. |
| `Z` | Index register | Decimal numbering. |
| `LZ` | Long index register | Use `slmp::highlevel::ValueType::U32` or `slmp::highlevel::ValueType::S32`. |
| `R` | File register | Decimal numbering. |
| `ZR` | Continuous file register | Decimal numbering. |
| `RD` | Refresh data register | Decimal numbering. |

## Value types

| Type | C++ name | Size | Meaning |
| --- | --- | --- | --- |
| `BIT` | `slmp::highlevel::ValueType::Bit` | 1 logical bit | Boolean value in `slmp::highlevel::Value::bit`. |
| `U` | `slmp::highlevel::ValueType::U16` | 16 bits | Unsigned word in `slmp::highlevel::Value::u16`. |
| `S` | `slmp::highlevel::ValueType::S16` | 16 bits | Signed word in `slmp::highlevel::Value::s16`. |
| `D` | `slmp::highlevel::ValueType::U32` | 32 bits | Unsigned double word in `slmp::highlevel::Value::u32`. |
| `L` | `slmp::highlevel::ValueType::S32` | 32 bits | Signed double word in `slmp::highlevel::Value::s32`. |
| `F` | `slmp::highlevel::ValueType::Float32` | 32 bits | IEEE-754 float in `slmp::highlevel::Value::f32`. |

## Addressing notes

| Topic | Rule |
| --- | --- |
| Long families | `LTN`, `LSTN`, `LCN`, and `LZ` require `U32` or `S32`; 16-bit access yields wrong data. |
| Module buffer memory | `G` and `HG` are not in the public high-level surface for normal direct device access. Use raw `slmp::SlmpClient` module-buffer APIs such as `readWordsModuleBuf`. |
| Step relay | `S` is not exposed publicly by the current headers. |
| Direct input/output | `DX` and `DY` are not valid for the iQ-F profile. |
| `X` and `Y` strings | Use profile-aware overloads; iQ-F uses octal notation while other supported profiles use hexadecimal notation. |
| Bit-in-word | Use `.n` with `n` from `0` to `F`, for example `D50.3`. |

## PLC profiles

See [PLC profiles](PROFILES.md) for profile identifiers, frame types, compatibility modes, and profile-specific cautions.
