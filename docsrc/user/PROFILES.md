# PLC profiles

The high-level API requires one explicit PLC profile. The selected profile controls frame type, low-level compatibility mode, string `X` and `Y` numbering rules, device-range catalog selection, and target-specific command guards.

For cross-profile capability and device-range details, see the [SLMP Profile Reference](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/).

Profile selection is intentionally not inferred from `ReadTypeName`, model
text, or model code. Some PLCs and network paths cannot return reliable type
name data, and a wrong inference can select the wrong address grammar or range
catalog. Keep the final profile choice in the application, configuration UI, or
operator workflow.

Profile selection is the supported way to apply target-specific behavior. Use
`slmp::highlevel::configureClientForPlcProfile` in normal applications. If an
application uses only `slmp::SlmpClient` directly, call `setPlcProfile` instead
of inferring a PLC from manual frame/compatibility settings. Manual frame and
compatibility settings do not imply a PLC model.

## Profiles table

| Canonical profile | Human label | C++ selector | Frame type | Compatibility mode | Notes |
| --- | --- | --- | --- | --- | --- |
| `melsec:iq-f` | MELSEC iQ-F | `slmp::highlevel::PlcProfile::IqF` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | iQ-F string `X` and `Y` addresses use octal notation; `DX` and `DY` are rejected. |
| `melsec:iq-r` | MELSEC iQ-R | `slmp::highlevel::PlcProfile::IqR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for modern iQ-R targets. |
| `melsec:iq-r:rj71en71` | MELSEC iQ-R via RJ71EN71 | `slmp::highlevel::PlcProfile::IqRRj71En71` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Ethernet-unit profile using iQ-R compatibility. |
| `melsec:iq-l` | MELSEC iQ-L | `slmp::highlevel::PlcProfile::IqL` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC iQ-L targets. |
| `melsec:mx-f` | MELSEC MX-F | `slmp::highlevel::PlcProfile::MxF` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC MX-F targets. |
| `melsec:mx-r` | MELSEC MX-R | `slmp::highlevel::PlcProfile::MxR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for MELSEC MX-R targets. |
| `melsec:lcpu` | MELSEC LCPU | `slmp::highlevel::PlcProfile::LCpu` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy L CPU profile. |
| `melsec:lcpu:lj71e71-100` | MELSEC LCPU via LJ71E71-100 | `slmp::highlevel::PlcProfile::LCpuLj71E71100` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::Legacy` | Ethernet unit profile using Q/L compatibility. |
| `melsec:qcpu:qj71e71-100` | MELSEC QCPU via QJ71E71-100 | `slmp::highlevel::PlcProfile::QCpuQj71E71100` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::Legacy` | Ethernet unit profile for QCPU connections. |
| `melsec:qnu` | MELSEC QnU | `slmp::highlevel::PlcProfile::QnU` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnU profile. Use direct or random device commands for normal access. |
| `melsec:qnu:qj71e71-100` | MELSEC QnU via QJ71E71-100 | `slmp::highlevel::PlcProfile::QnUQj71E71100` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::Legacy` | Ethernet unit profile using Q/L compatibility. |
| `melsec:qnudv` | MELSEC QnUDV | `slmp::highlevel::PlcProfile::QnUDV` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnUDV profile. Use direct or random device commands for normal access. |
| `melsec:qnudv:qj71e71-100` | MELSEC QnUDV via QJ71E71-100 | `slmp::highlevel::PlcProfile::QnUDVQj71E71100` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::Legacy` | Ethernet unit profile using Q/L compatibility. |

`melsec:qcpu` is base-only and remains for inherited address and range rules.
Use `melsec:qcpu:qj71e71-100` for QCPU Ethernet unit connections.

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

## Strict profile

`SlmpClient` enables strict profile checks by default. With a selected profile, operations known to be unavailable for that PLC are rejected before sending. Leave this enabled for normal applications.

Call `setStrictProfile(false)` only for deliberate verification where you want the PLC to answer directly. Point limits and write policy still apply.
