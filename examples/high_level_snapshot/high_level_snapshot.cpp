#include <string>
#include <vector>

#include "slmp_high_level.h"
#include "slmp_minimal.h"

namespace {

using slmp::highlevel::Poller;
using slmp::highlevel::PlcProfile;
using slmp::highlevel::Snapshot;
using slmp::highlevel::Value;

// Explicit profile selection avoids ambiguous X/Y numbering and frame defaults.
constexpr PlcProfile kPlcProfile = PlcProfile::IqR;

[[maybe_unused]] slmp::Error readBasicValues(slmp::SlmpClient& plc) {
    Value d100;
    // Named high-level addresses include the logical type explicitly.
    if (slmp::highlevel::readTyped(plc, kPlcProfile, "D100:U", d100) != slmp::Error::Ok) {
        return plc.lastError();
    }

    Value temperature;
    // :F reads two words and decodes them as IEEE-754 float32.
    if (slmp::highlevel::readTyped(plc, kPlcProfile, "D200:F", temperature) != slmp::Error::Ok) {
        return plc.lastError();
    }

    return slmp::Error::Ok;
}

[[maybe_unused]] slmp::Error readMixedSnapshot(slmp::SlmpClient& plc, Snapshot& out) {
    // readNamed preserves caller order and batches compatible word/dword reads.
    const std::vector<std::string> addresses = {
        "SM400:BIT",
        "D100:U",
        "D200:S",
        "D300:F",
        "D50.3",
    };
    return slmp::highlevel::readNamed(plc, kPlcProfile, addresses, out);
}

[[maybe_unused]] slmp::Error writeMixedValues(slmp::SlmpClient& plc) {
    Snapshot updates;
    // Each update is independent; use safe test addresses in real projects.
    updates.push_back({"D100:U", Value::u16Value(1234U)});
    updates.push_back({"D200:L", Value::s32Value(-123456)});
    updates.push_back({"D300:F", Value::float32Value(1.5f)});
    updates.push_back({"D50.3", Value::bitValue(true)});
    return slmp::highlevel::writeNamed(plc, kPlcProfile, updates);
}

[[maybe_unused]] slmp::Error pollCompiledPlan(slmp::SlmpClient& plc, Snapshot& out) {
    Poller poller;
    const std::vector<std::string> addresses = {
        "D100:U",
        "D101:S",
        "D200:F",
        "M1000:BIT",
    };
    slmp::Error err = poller.compile(addresses, kPlcProfile);
    if (err != slmp::Error::Ok) {
        return err;
    }
    // Poller keeps the compiled plan; callers control timing outside this helper.
    return poller.readOnce(plc, out);
}

}  // namespace

int main() {
    // Compile-only smoke sample.
    // Integrators are expected to provide a real transport, buffers, and a
    // connected slmp::SlmpClient instance before calling the helper functions.
    return 0;
}
