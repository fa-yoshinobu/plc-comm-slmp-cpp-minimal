#include <string>
#include <vector>

#include "slmp_high_level.h"
#include "slmp_minimal.h"

namespace {

using slmp::highlevel::Poller;
using slmp::highlevel::Snapshot;
using slmp::highlevel::Value;

[[maybe_unused]] slmp::Error readBasicValues(slmp::SlmpClient& plc) {
    Value d100;
    if (slmp::highlevel::readTyped(plc, "D100", d100) != slmp::Error::Ok) {
        return plc.lastError();
    }

    Value temperature;
    if (slmp::highlevel::readTyped(plc, "D200:F", temperature) != slmp::Error::Ok) {
        return plc.lastError();
    }

    return slmp::Error::Ok;
}

[[maybe_unused]] slmp::Error readMixedSnapshot(slmp::SlmpClient& plc, Snapshot& out) {
    const std::vector<std::string> addresses = {
        "SM400",
        "D100",
        "D200:S",
        "D300:F",
        "D50.3",
    };
    return slmp::highlevel::readNamed(plc, addresses, out);
}

[[maybe_unused]] slmp::Error writeMixedValues(slmp::SlmpClient& plc) {
    Snapshot updates;
    updates.push_back({"D100", Value::u16Value(1234U)});
    updates.push_back({"D200:L", Value::s32Value(-123456)});
    updates.push_back({"D300:F", Value::float32Value(1.5f)});
    updates.push_back({"D50.3", Value::bitValue(true)});
    return slmp::highlevel::writeNamed(plc, updates);
}

[[maybe_unused]] slmp::Error pollCompiledPlan(slmp::SlmpClient& plc, Snapshot& out) {
    Poller poller;
    const std::vector<std::string> addresses = {
        "D100",
        "D101:S",
        "D200:F",
        "M1000",
    };
    slmp::Error err = poller.compile(addresses);
    if (err != slmp::Error::Ok) {
        return err;
    }
    return poller.readOnce(plc, out);
}

}  // namespace

int main() {
    // Compile-only smoke sample.
    // Integrators are expected to provide a real transport, buffers, and a
    // connected slmp::SlmpClient instance before calling the helper functions.
    return 0;
}
