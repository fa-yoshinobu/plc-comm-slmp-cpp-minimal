# PLC profiles

The high-level API requires one explicit PLC profile. The selected profile controls frame type, low-level compatibility mode, string `X` and `Y` numbering rules, and device-range catalog selection.

Profile selection is intentionally not inferred from `ReadTypeName`, model
text, or model code. Some PLCs and network paths cannot return reliable type
name data, and a wrong inference can select the wrong address grammar or range
catalog. Keep the final profile choice in the application, configuration UI, or
operator workflow.

## Profiles table

| Enum value | Hardware | Frame type | Compatibility mode | Notes |
| --- | --- | --- | --- | --- |
| `slmp::highlevel::PlcProfile::IqF` | MELSEC iQ-F | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | iQ-F string `X` and `Y` addresses use octal notation; `DX` and `DY` are rejected. |
| `slmp::highlevel::PlcProfile::IqR` | MELSEC iQ-R | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for modern iQ-R targets. |
| `slmp::highlevel::PlcProfile::IqL` | MELSEC iQ-L | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses iQ-R string-address rules with the iQ-L range profile. |
| `slmp::highlevel::PlcProfile::MxF` | MELSEC MX-F profile | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses the MX-F address and range profiles. |
| `slmp::highlevel::PlcProfile::MxR` | MELSEC MX-R profile | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses the MX-R address and range profiles. |
| `slmp::highlevel::PlcProfile::QCpu` | MELSEC Q CPU | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy Q CPU profile. |
| `slmp::highlevel::PlcProfile::LCpu` | MELSEC L CPU | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy L CPU profile. |
| `slmp::highlevel::PlcProfile::QnU` | MELSEC QnU | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnU profile. |
| `slmp::highlevel::PlcProfile::QnUDV` | MELSEC QnU(DV) | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnUDV profile. |

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

## Profile-specific cautions

| Profile | Caution |
| --- | --- |
| iQ-F | Frame 3E only; do not use Frame 4E. |
| iQ-R and iQ-L | Frame 4E with iQR compatibility mode. |
| Q/L legacy | Frame 3E with Legacy compatibility mode. |
| MX-F and MX-R | Frame 4E with iQR compatibility mode. |
