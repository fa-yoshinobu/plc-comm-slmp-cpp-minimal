# PLC profiles

The high-level API requires one explicit PLC profile. The selected profile controls frame type, low-level compatibility mode, string `X` and `Y` numbering rules, and device-range catalog selection.

Profile selection is intentionally not inferred from `ReadTypeName`, model
text, or model code. Some PLCs and network paths cannot return reliable type
name data, and a wrong inference can select the wrong address grammar or range
catalog. Keep the final profile choice in the application, configuration UI, or
operator workflow.

## Profiles table

| Canonical profile | Human label | C++ selector | Frame type | Compatibility mode | Notes |
| --- | --- | --- | --- | --- | --- |
| `melsec:iq-f` | MELSEC iQ-F | `slmp::highlevel::PlcProfile::IqF` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | iQ-F string `X` and `Y` addresses use octal notation; `DX` and `DY` are rejected. |
| `melsec:iq-r` | MELSEC iQ-R | `slmp::highlevel::PlcProfile::IqR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Use for modern iQ-R targets. |
| `melsec:iq-l` | MELSEC iQ-L | `slmp::highlevel::PlcProfile::IqL` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses iQ-R string-address rules with the iQ-L range profile. |
| `melsec:mx-f` | MELSEC MX-F | `slmp::highlevel::PlcProfile::MxF` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses the MX-F address and range profiles. |
| `melsec:mx-r` | MELSEC MX-R | `slmp::highlevel::PlcProfile::MxR` | `slmp::FrameType::Frame4E` | `slmp::CompatibilityMode::iQR` | Uses the MX-R address and range profiles. |
| `melsec:qcpu` | MELSEC QCPU | `slmp::highlevel::PlcProfile::QCpu` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy Q CPU profile. |
| `melsec:lcpu` | MELSEC LCPU | `slmp::highlevel::PlcProfile::LCpu` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | Legacy L CPU profile. |
| `melsec:qnu` | MELSEC QnU | `slmp::highlevel::PlcProfile::QnU` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnU profile. |
| `melsec:qnudv` | MELSEC QnUDV | `slmp::highlevel::PlcProfile::QnUDV` | `slmp::FrameType::Frame3E` | `slmp::CompatibilityMode::Legacy` | QnUDV profile. |

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

| Canonical profile | Human label | Caution |
| --- | --- | --- |
| `melsec:iq-f` | MELSEC iQ-F | Frame 3E only; do not use Frame 4E. |
| `melsec:iq-r` | MELSEC iQ-R | Frame 4E with iQR compatibility mode. |
| `melsec:iq-l` | MELSEC iQ-L | Frame 4E with iQR compatibility mode. |
| `melsec:mx-f` | MELSEC MX-F | Frame 4E with iQR compatibility mode. |
| `melsec:mx-r` | MELSEC MX-R | Frame 4E with iQR compatibility mode. |
| `melsec:qcpu` | MELSEC QCPU | Frame 3E with Legacy compatibility mode. |
| `melsec:lcpu` | MELSEC LCPU | Frame 3E with Legacy compatibility mode. |
| `melsec:qnu` | MELSEC QnU | Frame 3E with Legacy compatibility mode. |
| `melsec:qnudv` | MELSEC QnUDV | Frame 3E with Legacy compatibility mode. |
