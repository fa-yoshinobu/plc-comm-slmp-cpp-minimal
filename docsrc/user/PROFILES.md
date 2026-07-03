# PLC profiles

The high-level API requires one explicit PLC profile. The selected profile controls frame type, low-level compatibility mode, string `X` and `Y` numbering rules, device-range catalog selection, and target-specific command guards.

Profile selection is intentionally not inferred from `ReadTypeName`, model
text, or model code. Some PLCs and network paths cannot return reliable type
name data, and a wrong inference can select the wrong address grammar or range
catalog. Keep the final profile choice in the application, configuration UI, or
operator workflow.

Profile selection is the supported way to apply target-specific behavior. Use
`slmp::highlevel::configureClientForPlcProfile` in normal applications. If an
application uses only `slmp::SlmpClient` directly, call `setPlcProfile` instead
of inferring a PLC from manual frame/compatibility settings. Manual frame and
compatibility setters are raw protocol controls and do not imply a PLC model.

## Profiles table

| Canonical profile | Human label | C++ selector | Frame type | Compatibility mode | Notes |
| --- | --- | --- | --- | --- | --- |
| `melsec:iq-f` | MELSEC iQ-F | `slmp::highlevel::PlcProfile::IqF` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | iQ-F string `X` and `Y` addresses use octal notation; `DX` and `DY` are rejected. |
| `melsec:iq-r` | MELSEC iQ-R | `slmp::highlevel::PlcProfile::IqR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for modern iQ-R targets. |
| `melsec:iq-l` | MELSEC iQ-L | `slmp::highlevel::PlcProfile::IqL` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC iQ-L targets. |
| `melsec:mx-f` | MELSEC MX-F | `slmp::highlevel::PlcProfile::MxF` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC MX-F targets. |
| `melsec:mx-r` | MELSEC MX-R | `slmp::highlevel::PlcProfile::MxR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC MX-R targets. |
| `melsec:qcpu` | MELSEC QCPU | `slmp::highlevel::PlcProfile::QCpu` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Q CPU profile. Strict profile rejects unavailable block routes; use direct or random device commands. |
| `melsec:lcpu` | MELSEC LCPU | `slmp::highlevel::PlcProfile::LCpu` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy L CPU profile. |
| `melsec:qnu` | MELSEC QnU | `slmp::highlevel::PlcProfile::QnU` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnU profile. Strict profile rejects unavailable block routes; use direct or random device commands. |
| `melsec:qnudv` | MELSEC QnUDV | `slmp::highlevel::PlcProfile::QnUDV` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnUDV profile. Built-in capability data marks Read Block (`0x0406`) and Write Block (`0x1406`) as blocked on the measured built-in Ethernet route. |

## How to select

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <slmp_high_level.h>
#include <slmp_minimal.h>

class NoopTransport final : public slmp::ITransport {
  public:
    bool connect(const char*, uint16_t) override { return false; }
    void close() override {}
    bool connected() const override { return false; }
    bool writeAll(const uint8_t*, size_t) override { return false; }
    bool readExact(uint8_t*, size_t, uint32_t) override { return false; }
    size_t write(const uint8_t*, size_t) override { return 0U; }
    size_t read(uint8_t*, size_t) override { return 0U; }
    size_t available() override { return 0U; }
};

int main() {
    NoopTransport transport;
    uint8_t tx[64] = {};
    uint8_t rx[64] = {};
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));

    constexpr auto profile = slmp::highlevel::PlcProfile::IqR;
    slmp::highlevel::configureClientForPlcProfile(plc, profile);
    std::printf("%s\n", slmp::highlevel::plcProfileLabel(profile));

    return plc.frameType() == slmp::FrameType::Frame4E ? 0 : 1;
}
```

## Strict profile capability guard

`SlmpClient` enables strict profile guards by default. When the selected profile has built-in capability data and a high-level route is marked `blocked` or `unverified`, the request returns `slmp::Error::ProfileFeatureBlocked` before transport. Use `lastProfileFeatureErrorInfo()` to inspect the profile, feature key, state, evidence, and bypass hint.

Call `setStrictProfile(false)` only when you intentionally want to send the request and inspect the PLC response yourself. Point limits and write policy still apply when strict profile is disabled.

The guard covers the implemented high-level feature keys `type_name`, `direct`, `random`, `block`, `monitor`, `ext_module_access`, `ext_link_direct`, `hg_cpu_buffer`, `long_device_path`, and `lz_32bit_path`. Generic `readExtendUnit*` / `writeExtendUnit*`, memory, label, remote-control, password, self-test, and clear-error APIs are raw protocol routes and are not capability-feature guarded.

All canonical Q/L profiles are in the built-in capability table. Their unavailable block-command routes return `slmp::Error::ProfileFeatureBlocked` while strict profile is enabled.

## Profile-specific cautions

| Canonical profile | Human label | Caution |
| --- | --- | --- |
| `melsec:iq-f` | MELSEC iQ-F | Frame 3E only; do not use Frame 4E. |
| `melsec:iq-r` | MELSEC iQ-R | Frame 4E with iQR compatibility mode. |
| `melsec:iq-l` | MELSEC iQ-L | Frame 4E with iQR compatibility mode. |
| `melsec:mx-f` | MELSEC MX-F | Frame 4E with iQR compatibility mode. |
| `melsec:mx-r` | MELSEC MX-R | Frame 4E with iQR compatibility mode. |
| `melsec:qcpu` | MELSEC QCPU | Frame 3E with Legacy compatibility mode. Strict profile rejects block commands `0x0406` / `0x1406`. |
| `melsec:lcpu` | MELSEC LCPU | Frame 3E with Legacy compatibility mode. Built-in capability data marks type-name and block routes as blocked on the measured built-in Ethernet route. |
| `melsec:qnu` | MELSEC QnU | Frame 3E with Legacy compatibility mode. Strict profile rejects block commands `0x0406` / `0x1406`. |
| `melsec:qnudv` | MELSEC QnUDV | Frame 3E with Legacy compatibility mode. Built-in capability data marks type-name and block routes as blocked on the measured built-in Ethernet route. |
