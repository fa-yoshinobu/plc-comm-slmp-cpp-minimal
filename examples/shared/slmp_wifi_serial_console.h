#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <slmp_arduino_transport.h>

#ifndef SLMP_WIFI_SERIAL_CONSOLE_CONFIG_HEADER
#define SLMP_WIFI_SERIAL_CONSOLE_CONFIG_HEADER "config.h"
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE
#define SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE slmp_wifi_serial_console
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_BANNER
#define SLMP_WIFI_SERIAL_CONSOLE_BANNER "SLMP Wi-Fi serial debug console"
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS
#define SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS 1500
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_EXTRA_HELP
#define SLMP_WIFI_SERIAL_CONSOLE_EXTRA_HELP()
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_COMMAND_HANDLER
#define SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_COMMAND_HANDLER(tokens, token_count) false
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_HELP_HANDLER
#define SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_HELP_HANDLER(tokens, token_count) false
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_PRINT_PENDING_JUDGE_HINT
#define SLMP_WIFI_SERIAL_CONSOLE_PRINT_PENDING_JUDGE_HINT() \
    do { \
        Serial.println("observe machine/hmi and enter: judge ok [note] or judge ng [note]"); \
    } while (0)
#endif

#ifndef SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
#define SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH 0
#endif

#include SLMP_WIFI_SERIAL_CONSOLE_CONFIG_HEADER

namespace SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE {
constexpr const char* kPlcHost = example_config::kPlcHost;
constexpr uint16_t kPlcPort = example_config::kPlcPort;
constexpr size_t kSerialLineCapacity = 384;
constexpr size_t kMaxTokens = 48;
constexpr size_t kMaxWordPoints = 8;
constexpr size_t kVerificationNoteCapacity = 80;
constexpr size_t kMaxRandomWordDevices = 8;
constexpr size_t kMaxRandomDWordDevices = 8;
constexpr size_t kMaxRandomBitDevices = 8;
constexpr size_t kMaxBlockCount = 4;
constexpr size_t kMaxBlockPoints = 16;
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
constexpr uint32_t kBenchReportIntervalMs = 3000;
constexpr uint32_t kBenchDefaultCycles = 1000;
constexpr size_t kBenchWordPoints = kMaxWordPoints;
constexpr size_t kBenchBlockPoints = kMaxBlockPoints;
constexpr slmp::DeviceAddress kBenchOneWordDevice = {slmp::DeviceCode::D, 800};
constexpr slmp::DeviceAddress kBenchWordArrayDevice = {slmp::DeviceCode::D, 820};
constexpr slmp::DeviceAddress kBenchBlockWordDevice = {slmp::DeviceCode::D, 900};
#endif
struct DeviceSpec {
    const char* name;
    slmp::DeviceCode code;
    bool hex_address;
};
enum class VerificationKind : uint8_t {
    None = 0,
    WordWrite,
    BitWrite,
};
struct VerificationRecord {
    bool active = false;
    bool judged = false;
    bool pass = false;
    bool readback_match = false;
    VerificationKind kind = VerificationKind::None;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 0};
    uint16_t points = 0;
    uint16_t before_words[kMaxWordPoints] = {};
    uint16_t written_words[kMaxWordPoints] = {};
    uint16_t readback_words[kMaxWordPoints] = {};
    bool before_bit = false;
    bool written_bit = false;
    bool readback_bit = false;
    uint32_t started_ms = 0;
    char note[kVerificationNoteCapacity] = {};
};
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
enum class BenchMode : uint8_t {
    None = 0,
    Row,
    Wow,
    Pair,
    Rw,
    Ww,
    Block,
};
struct BenchSummary {
    uint32_t cycles = 0;
    uint32_t requests = 0;
    uint32_t fail = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    size_t last_request_bytes = 0;
    size_t last_response_bytes = 0;
};
#endif
const DeviceSpec kDeviceSpecs[] = {
    {"SM", slmp::DeviceCode::SM, false},
    {"SD", slmp::DeviceCode::SD, false},
    {"X", slmp::DeviceCode::X, true},
    {"Y", slmp::DeviceCode::Y, true},
    {"M", slmp::DeviceCode::M, false},
    {"L", slmp::DeviceCode::L, false},
    {"F", slmp::DeviceCode::F, false},
    {"V", slmp::DeviceCode::V, false},
    {"B", slmp::DeviceCode::B, true},
    {"D", slmp::DeviceCode::D, false},
    {"W", slmp::DeviceCode::W, true},
    {"TS", slmp::DeviceCode::TS, false},
    {"TC", slmp::DeviceCode::TC, false},
    {"TN", slmp::DeviceCode::TN, false},
    {"LTS", slmp::DeviceCode::LTS, false},
    {"LTC", slmp::DeviceCode::LTC, false},
    {"LTN", slmp::DeviceCode::LTN, false},
    {"STS", slmp::DeviceCode::STS, false},
    {"STC", slmp::DeviceCode::STC, false},
    {"STN", slmp::DeviceCode::STN, false},
    {"LSTS", slmp::DeviceCode::LSTS, false},
    {"LSTC", slmp::DeviceCode::LSTC, false},
    {"LSTN", slmp::DeviceCode::LSTN, false},
    {"CS", slmp::DeviceCode::CS, false},
    {"CC", slmp::DeviceCode::CC, false},
    {"CN", slmp::DeviceCode::CN, false},
    {"LCS", slmp::DeviceCode::LCS, false},
    {"LCC", slmp::DeviceCode::LCC, false},
    {"LCN", slmp::DeviceCode::LCN, false},
    {"SB", slmp::DeviceCode::SB, true},
    {"SW", slmp::DeviceCode::SW, true},
    {"S", slmp::DeviceCode::S, false},
    {"DX", slmp::DeviceCode::DX, true},
    {"DY", slmp::DeviceCode::DY, true},
    {"Z", slmp::DeviceCode::Z, false},
    {"LZ", slmp::DeviceCode::LZ, false},
    {"R", slmp::DeviceCode::R, false},
    {"ZR", slmp::DeviceCode::ZR, false},
    {"RD", slmp::DeviceCode::RD, false},
    {"G", slmp::DeviceCode::G, false},
    {"HG", slmp::DeviceCode::HG, false},
};
WiFiClient tcp_client;
slmp::ArduinoClientTransport transport(tcp_client);
uint8_t tx_buffer[example_config::kTxBufferSize];
uint8_t rx_buffer[example_config::kRxBufferSize];
slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
char serial_line[kSerialLineCapacity] = {};
size_t serial_line_length = 0;
bool wifi_ready = false;
VerificationRecord verification = {};
bool serial_last_was_cr = false;
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
uint16_t bench_word_values[kBenchWordPoints] = {};
uint16_t bench_word_readback[kBenchWordPoints] = {};
uint16_t bench_block_values[kBenchBlockPoints] = {};
uint16_t bench_block_readback[kBenchBlockPoints] = {};
#endif
void handleCommand(char* line);
void executeCommandLine(char* line, bool print_prompt);
bool connectPlc(bool verbose);
void printDeviceAddress(const slmp::DeviceAddress& device, uint32_t offset);
void printLastPlcError(const char* label);
const DeviceSpec* findDeviceSpecByName(const char* token) {
    const DeviceSpec* match = nullptr;
    size_t match_length = 0;
    for (size_t i = 0; i < sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]); ++i) {
        const size_t name_length = strlen(kDeviceSpecs[i].name);
        if (name_length < match_length) {
            continue;
        }
        if (strncmp(token, kDeviceSpecs[i].name, name_length) == 0) {
            match = &kDeviceSpecs[i];
            match_length = name_length;
        }
    }
    return match;
}
const DeviceSpec* findDeviceSpecByCode(slmp::DeviceCode code) {
    for (size_t i = 0; i < sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]); ++i) {
        if (kDeviceSpecs[i].code == code) {
            return &kDeviceSpecs[i];
        }
    }
    return nullptr;
}
void uppercaseInPlace(char* text) {
    if (text == nullptr) {
        return;
    }
    for (; *text != '\0'; ++text) {
        *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
    }
}
bool parseUnsignedValue(const char* token, unsigned long& value, int base) {
    if (token == nullptr || *token == '\0') {
        return false;
    }
    char* end = nullptr;
    value = strtoul(token, &end, base);
    return end != token && *end == '\0';
}
bool parseWordValue(const char* token, uint16_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFFFUL) {
        return false;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}
bool parseDWordValue(const char* token, uint32_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0)) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}
bool parseByteValue(const char* token, uint8_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFUL) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}
bool parseUint16Value(const char* token, uint16_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFFFUL) {
        return false;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}
bool parseListCount(const char* token, size_t max_value, uint8_t& count) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 10) || parsed > max_value) {
        return false;
    }
    count = static_cast<uint8_t>(parsed);
    return true;
}
bool parsePointCount(const char* token, uint16_t& points) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 10) || parsed == 0 || parsed > kMaxWordPoints) {
        return false;
    }
    points = static_cast<uint16_t>(parsed);
    return true;
}
bool parseBoolValue(char* token, bool& value) {
    if (token == nullptr) {
        return false;
    }
    uppercaseInPlace(token);
    if (strcmp(token, "1") == 0 || strcmp(token, "ON") == 0 || strcmp(token, "TRUE") == 0) {
        value = true;
        return true;
    }
    if (strcmp(token, "0") == 0 || strcmp(token, "OFF") == 0 || strcmp(token, "FALSE") == 0) {
        value = false;
        return true;
    }
    return false;
}
void resetVerificationRecord() {
    verification = VerificationRecord();
}
bool wordArraysEqual(const uint16_t* lhs, const uint16_t* rhs, uint16_t points) {
    for (uint16_t i = 0; i < points; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}
void joinTokens(char* tokens[], int start_index, int token_count, char* out, size_t out_capacity) {
    if (out == nullptr || out_capacity == 0) {
        return;
    }
    out[0] = '\0';
    size_t used = 0;
    for (int i = start_index; i < token_count; ++i) {
        const char* token = tokens[i];
        if (token == nullptr || *token == '\0') {
            continue;
        }
        if (used != 0) {
            if (used + 1 >= out_capacity) {
                break;
            }
            out[used++] = ' ';
            out[used] = '\0';
        }
        const size_t available = out_capacity - used - 1;
        const size_t token_length = strlen(token);
        const size_t copy_length = token_length < available ? token_length : available;
        memcpy(out + used, token, copy_length);
        used += copy_length;
        out[used] = '\0';
        if (copy_length < token_length) {
            break;
        }
    }
}
bool parseDeviceAddress(char* token, slmp::DeviceAddress& device) {
    if (token == nullptr || *token == '\0') {
        return false;
    }
    uppercaseInPlace(token);
    const DeviceSpec* spec = findDeviceSpecByName(token);
    if (spec == nullptr) {
        return false;
    }
    const size_t prefix_length = strlen(spec->name);
    const char* number_text = token + prefix_length;
    unsigned long number = 0;
    if (!parseUnsignedValue(number_text, number, spec->hex_address ? 16 : 10)) {
        return false;
    }
    device.code = spec->code;
    device.number = static_cast<uint32_t>(number);
    return true;
}
void printDeviceAddress(const slmp::DeviceAddress& device, uint32_t offset = 0) {
    const DeviceSpec* spec = findDeviceSpecByCode(device.code);
    if (spec == nullptr) {
        Serial.print("?");
        Serial.print(device.number + offset);
        return;
    }
    Serial.print(spec->name);
    if (spec->hex_address) {
        Serial.print(device.number + offset, HEX);
    } else {
        Serial.print(device.number + offset);
    }
}
void printTargetAddress() {
    const slmp::TargetAddress& target = plc.target();
    Serial.print("target_network=");
    Serial.println(target.network);
    Serial.print("target_station=");
    Serial.println(target.station);
    Serial.print("target_module_io=0x");
    Serial.println(target.module_io, HEX);
    Serial.print("target_multidrop=");
    Serial.println(target.multidrop);
}
const char* wifiStatusText(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:
            return "idle";
        case WL_NO_SSID_AVAIL:
            return "no_ssid";
        case WL_SCAN_COMPLETED:
            return "scan_completed";
        case WL_CONNECTED:
            return "connected";
        case WL_CONNECT_FAILED:
            return "connect_failed";
        case WL_CONNECTION_LOST:
            return "connection_lost";
        case WL_DISCONNECTED:
            return "disconnected";
        default:
            return "unknown";
    }
}

#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
bool parseBenchModeToken(char* token, BenchMode& mode) {
    if (token == nullptr) {
        return false;
    }
    uppercaseInPlace(token);
    if (strcmp(token, "ROW") == 0 || strcmp(token, "READ") == 0) {
        mode = BenchMode::Row;
        return true;
    }
    if (strcmp(token, "WOW") == 0 || strcmp(token, "WRITE") == 0) {
        mode = BenchMode::Wow;
        return true;
    }
    if (strcmp(token, "PAIR") == 0) {
        mode = BenchMode::Pair;
        return true;
    }
    if (strcmp(token, "RW") == 0) {
        mode = BenchMode::Rw;
        return true;
    }
    if (strcmp(token, "WW") == 0) {
        mode = BenchMode::Ww;
        return true;
    }
    if (strcmp(token, "BLOCK") == 0) {
        mode = BenchMode::Block;
        return true;
    }
    return false;
}

const char* benchModeText(BenchMode mode) {
    switch (mode) {
        case BenchMode::Row:
            return "row";
        case BenchMode::Wow:
            return "wow";
        case BenchMode::Pair:
            return "pair";
        case BenchMode::Rw:
            return "rw";
        case BenchMode::Ww:
            return "ww";
        case BenchMode::Block:
            return "block";
        default:
            return "none";
    }
}

void fillBenchWords(uint16_t* values, size_t count, uint16_t seed) {
    for (size_t i = 0; i < count; ++i) {
        values[i] = static_cast<uint16_t>(seed + i);
    }
}

void clearBenchWords(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || !connectPlc(false)) {
        return;
    }
    uint16_t zeros[kMaxWordPoints] = {};
    size_t offset = 0;
    while (offset < count) {
        const size_t chunk = (count - offset) > kMaxWordPoints ? kMaxWordPoints : (count - offset);
        const slmp::DeviceAddress chunk_device = {device.code, device.number + static_cast<uint32_t>(offset)};
        (void)plc.writeWords(chunk_device, zeros, chunk);
        offset += chunk;
    }
}

void clearBenchTargets() {
    clearBenchWords(kBenchOneWordDevice, 1);
    clearBenchWords(kBenchWordArrayDevice, kBenchWordPoints);
    clearBenchWords(kBenchBlockWordDevice, kBenchBlockPoints);
}

void printBenchList() {
    Serial.println("bench modes:");
    Serial.println("  bench");
    Serial.println("    runs pair benchmark with default cycles");
    Serial.println("  bench row [cycles]");
    Serial.println("  bench wow [cycles]");
    Serial.println("  bench pair [cycles]");
    Serial.println("  bench rw [cycles]");
    Serial.println("  bench ww [cycles]");
    Serial.println("  bench block [cycles]");
    Serial.println("bench devices:");
    Serial.print("  row/wow/pair: ");
    printDeviceAddress(kBenchOneWordDevice, 0);
    Serial.println();
    Serial.print("  rw/ww: ");
    printDeviceAddress(kBenchWordArrayDevice, 0);
    Serial.print("..");
    printDeviceAddress(kBenchWordArrayDevice, static_cast<uint32_t>(kBenchWordPoints - 1U));
    Serial.println();
    Serial.print("  block: ");
    printDeviceAddress(kBenchBlockWordDevice, 0);
    Serial.print("..");
    printDeviceAddress(kBenchBlockWordDevice, static_cast<uint32_t>(kBenchBlockPoints - 1U));
    Serial.println();
}

void printBenchProgress(const char* label, BenchMode mode, const BenchSummary& summary, uint32_t started_ms) {
    Serial.print(label);
    Serial.print(" mode=");
    Serial.print(benchModeText(mode));
    Serial.print(" cycles=");
    Serial.print(summary.cycles);
    Serial.print(" requests=");
    Serial.print(summary.requests);
    Serial.print(" fail=");
    Serial.print(summary.fail);
    Serial.print(" elapsed_ms=");
    const uint32_t elapsed_ms = millis() - started_ms;
    Serial.print(elapsed_ms);
    Serial.print(" last_cycle_ms=");
    Serial.print(summary.last_cycle_ms);
    Serial.print(" avg_cycle_ms=");
    if (summary.cycles == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(summary.total_cycle_ms / summary.cycles));
    }
    Serial.print(" avg_req_ms=");
    if (summary.requests == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(summary.total_cycle_ms / summary.requests));
    }
    Serial.print(" min_ms=");
    Serial.print(summary.cycles == 0 ? 0U : summary.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.print(summary.max_cycle_ms);
    Serial.print(" req_per_sec=");
    if (elapsed_ms == 0 || summary.requests == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>((static_cast<uint64_t>(summary.requests) * 1000U) / elapsed_ms));
    }
    Serial.print(" last_req_bytes=");
    Serial.print(summary.last_request_bytes);
    Serial.print(" last_resp_bytes=");
    Serial.println(summary.last_response_bytes);
}

bool runBench(BenchMode mode, uint32_t cycle_limit) {
    if (!connectPlc(false)) {
        Serial.println("bench failed: plc not connected");
        return false;
    }

    clearBenchTargets();
    BenchSummary summary = {};
    const uint32_t started_ms = millis();
    uint32_t last_report_ms = started_ms;
    uint16_t seed = 1000;

    for (uint32_t cycle = 0; cycle < cycle_limit; ++cycle) {
        const uint32_t cycle_started_ms = millis();
        bool ok = false;
        size_t request_count = 0;

        if (mode == BenchMode::Row) {
            uint16_t value = 0;
            ok = plc.readOneWord(kBenchOneWordDevice, value) == slmp::Error::Ok;
            request_count = 1;
        } else if (mode == BenchMode::Wow) {
            ok = plc.writeOneWord(kBenchOneWordDevice, seed++) == slmp::Error::Ok;
            request_count = 1;
        } else if (mode == BenchMode::Pair) {
            const uint16_t expected = seed++;
            uint16_t readback = 0;
            ok = plc.writeOneWord(kBenchOneWordDevice, expected) == slmp::Error::Ok &&
                 plc.readOneWord(kBenchOneWordDevice, readback) == slmp::Error::Ok &&
                 readback == expected;
            request_count = 2;
        } else if (mode == BenchMode::Rw) {
            ok = plc.readWords(
                     kBenchWordArrayDevice,
                     static_cast<uint16_t>(kBenchWordPoints),
                     bench_word_readback,
                     kBenchWordPoints) == slmp::Error::Ok;
            request_count = 1;
        } else if (mode == BenchMode::Ww) {
            fillBenchWords(bench_word_values, kBenchWordPoints, seed);
            seed = static_cast<uint16_t>(seed + kBenchWordPoints);
            ok = plc.writeWords(kBenchWordArrayDevice, bench_word_values, kBenchWordPoints) == slmp::Error::Ok;
            request_count = 1;
        } else if (mode == BenchMode::Block) {
            fillBenchWords(bench_block_values, kBenchBlockPoints, seed);
            seed = static_cast<uint16_t>(seed + kBenchBlockPoints);
            const slmp::DeviceBlockWrite block = {kBenchBlockWordDevice, bench_block_values, static_cast<uint16_t>(kBenchBlockPoints)};
            const slmp::DeviceBlockRead read_block = {kBenchBlockWordDevice, static_cast<uint16_t>(kBenchBlockPoints)};
            ok = plc.writeBlock(&block, 1, nullptr, 0) == slmp::Error::Ok &&
                 plc.readBlock(&read_block, 1, nullptr, 0, bench_block_readback, kBenchBlockPoints, nullptr, 0) == slmp::Error::Ok &&
                 wordArraysEqual(bench_block_values, bench_block_readback, static_cast<uint16_t>(kBenchBlockPoints));
            request_count = 2;
        }

        summary.last_cycle_ms = millis() - cycle_started_ms;
        summary.last_request_bytes = plc.lastRequestFrameLength();
        summary.last_response_bytes = plc.lastResponseFrameLength();
        if (summary.cycles == 0 || summary.last_cycle_ms < summary.min_cycle_ms) {
            summary.min_cycle_ms = summary.last_cycle_ms;
        }
        if (summary.last_cycle_ms > summary.max_cycle_ms) {
            summary.max_cycle_ms = summary.last_cycle_ms;
        }
        summary.total_cycle_ms += summary.last_cycle_ms;
        ++summary.cycles;
        summary.requests += static_cast<uint32_t>(request_count);

        if (!ok) {
            ++summary.fail;
            Serial.print("bench failed: mode=");
            Serial.print(benchModeText(mode));
            Serial.print(" cycle=");
            Serial.println(cycle + 1U);
            printLastPlcError("bench");
            printBenchProgress("bench summary:", mode, summary, started_ms);
            clearBenchTargets();
            return false;
        }

        if (millis() - last_report_ms >= kBenchReportIntervalMs) {
            printBenchProgress("bench progress:", mode, summary, started_ms);
            last_report_ms = millis();
        }
    }

    printBenchProgress("bench summary:", mode, summary, started_ms);
    clearBenchTargets();
    return true;
}
#endif
bool wifiConfigLooksUnset() {
    return example_config::kWifiSsid[0] == '\0' ||
           example_config::kWifiPassword[0] == '\0';
}
bool waitForWifiConnection() {
    const uint32_t started_ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - started_ms >= example_config::kWifiConnectTimeoutMs) {
            return false;
        }
        delay(250);
    }
    return true;
}
void printWifiSummary() {
    Serial.print("wifi_status=");
    Serial.println(wifiStatusText(WiFi.status()));
    Serial.print("wifi_target_ssid=");
    Serial.println(example_config::kWifiSsid);
    Serial.print("wifi_ssid=");
    Serial.println(WiFi.SSID());
    Serial.print("local ip=");
    Serial.println(WiFi.localIP());
}
void printLastFrames() {
    char request_hex[256] = {};
    char response_hex[256] = {};
    if (plc.lastRequestFrameLength() == 0) {
        Serial.println("last request: <none>");
    } else {
        slmp::formatHexBytes(
            plc.lastRequestFrame(),
            plc.lastRequestFrameLength(),
            request_hex,
            sizeof(request_hex)
        );
        Serial.print("last request: ");
        Serial.println(request_hex);
    }
    if (plc.lastResponseFrameLength() == 0) {
        Serial.println("last response: <none>");
    } else {
        slmp::formatHexBytes(
            plc.lastResponseFrame(),
            plc.lastResponseFrameLength(),
            response_hex,
            sizeof(response_hex)
        );
        Serial.print("last response: ");
        Serial.println(response_hex);
    }
}
void printLastPlcError(const char* label) {
    Serial.print(label);
    Serial.print(" failed: ");
    Serial.print(slmp::errorString(plc.lastError()));
    Serial.print(" end=0x");
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(" (");
    Serial.print(slmp::endCodeString(plc.lastEndCode()));
    Serial.println(")");
    printLastFrames();
}
void printPrompt() {
    Serial.print("> ");
}
void submitSerialLine() {
    serial_line[serial_line_length] = '\0';
    executeCommandLine(serial_line, true);
    serial_line_length = 0;
    serial_line[0] = '\0';
}
bool bringUpWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    if (wifiConfigLooksUnset()) {
        Serial.println("wifi config looks unset; edit config.h");
    }
    WiFi.begin(example_config::kWifiSsid, example_config::kWifiPassword);
    wifi_ready = waitForWifiConnection();
    printWifiSummary();
    if (!wifi_ready) {
        Serial.println("wifi connect timeout");
    }
    return wifi_ready;
}
bool ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        wifi_ready = true;
        return true;
    }
    wifi_ready = false;
    return bringUpWiFi();
}
bool connectPlc(bool verbose) {
    if (!ensureWiFi()) {
        return false;
    }
    if (plc.connected()) {
        if (verbose) {
            Serial.println("plc already connected");
        }
        return true;
    }
    if (!plc.connect(kPlcHost, kPlcPort)) {
        if (verbose) {
            Serial.print("connect failed: ");
            Serial.println(slmp::errorString(plc.lastError()));
        }
        return false;
    }
    if (verbose) {
        Serial.println("plc connected");
    }
    return true;
}
void closePlc() {
    plc.close();
    Serial.println("plc closed");
}
void reinitializeWiFi() {
    plc.close();
    WiFi.disconnect(true, true);
    delay(100);
    wifi_ready = false;
    (void)bringUpWiFi();
}
void printStatus() {
    printWifiSummary();
    Serial.print("plc_connected=");
    Serial.println(plc.connected() ? "yes" : "no");
    Serial.print("tx_buffer_bytes=");
    Serial.println(sizeof(tx_buffer));
    Serial.print("rx_buffer_bytes=");
    Serial.println(sizeof(rx_buffer));
    Serial.print("timeout_ms=");
    Serial.println(plc.timeoutMs());
    Serial.print("monitoring_timer=");
    Serial.println(plc.monitoringTimer());
    printTargetAddress();
    Serial.print("last_error=");
    Serial.println(slmp::errorString(plc.lastError()));
    Serial.print("last_end_code=0x");
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(" (");
    Serial.print(slmp::endCodeString(plc.lastEndCode()));
    Serial.println(")");
    Serial.print("verification_active=");
    Serial.println(verification.active ? "yes" : "no");
    if (verification.active) {
        Serial.print("verification_judged=");
        Serial.println(verification.judged ? "yes" : "no");
        if (verification.judged) {
            Serial.print("verification_result=");
            Serial.println(verification.pass ? "ok" : "ng");
        }
    }
}
void printVerificationSummary() {
    if (!verification.active) {
        Serial.println("verification: none");
        return;
    }
    Serial.print("verification kind=");
    Serial.println(verification.kind == VerificationKind::WordWrite ? "word_write" : "bit_write");
    Serial.print("device=");
    printDeviceAddress(verification.device);
    Serial.println();
    if (verification.kind == VerificationKind::WordWrite) {
        for (uint16_t i = 0; i < verification.points; ++i) {
            printDeviceAddress(verification.device, i);
            Serial.print(" before=");
            Serial.print(verification.before_words[i]);
            Serial.print(" written=");
            Serial.print(verification.written_words[i]);
            Serial.print(" readback=");
            Serial.println(verification.readback_words[i]);
        }
    } else if (verification.kind == VerificationKind::BitWrite) {
        printDeviceAddress(verification.device);
        Serial.print(" before=");
        Serial.print(verification.before_bit ? 1 : 0);
        Serial.print(" written=");
        Serial.print(verification.written_bit ? 1 : 0);
        Serial.print(" readback=");
        Serial.println(verification.readback_bit ? 1 : 0);
    }
    Serial.print("auto_readback_match=");
    Serial.println(verification.readback_match ? "yes" : "no");
    if (!verification.judged) {
        Serial.println("manual_judgement=pending");
        SLMP_WIFI_SERIAL_CONSOLE_PRINT_PENDING_JUDGE_HINT();
        return;
    }
    Serial.print("manual_judgement=");
    Serial.println(verification.pass ? "ok" : "ng");
    if (verification.note[0] != '\0') {
        Serial.print("note=");
        Serial.println(verification.note);
    }
}
void printHelp() {
    Serial.println("commands:");
    Serial.println("  help");
    Serial.println("  status");
    Serial.println("  connect | close | reinit | type | dump");
    Serial.println("  target [network station module_io multidrop]");
    Serial.println("  monitor [value]");
    Serial.println("  timeout <ms>");
    Serial.println("  rw <device> [points]");
    Serial.println("  ww <device> <value...>");
    Serial.println("  row <device>");
    Serial.println("  wow <device> <value>");
    Serial.println("  rb <device>");
    Serial.println("  wb <device> <0|1>");
    Serial.println("  rbits <device> <points>");
    Serial.println("  wbits <device> <0|1...>");
    Serial.println("  rdw <device> [points]");
    Serial.println("  wdw <device> <value...>");
    Serial.println("  rod <device>");
    Serial.println("  wod <device> <value>");
    Serial.println("  rr <word_count> <word_devices...> <dword_count> <dword_devices...>");
    Serial.println("  wrand <word_count> <device value...> <dword_count> <device value...>");
    Serial.println("  wrandb <count> <device value...>");
    Serial.println("  rblk <word_blocks> <device points...> <bit_blocks> <device points...>");
    Serial.println("  wblk <word_blocks> <device points values...> <bit_blocks> <device points packed...>");
    Serial.println("  unlock <password>");
    Serial.println("  lock <password>");
    Serial.println("  verifyw <device> <value...>");
    Serial.println("  verifyb <device> <0|1>");
    Serial.println("  pending | judge <ok|ng> [note]");
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
    Serial.println("  bench [row|wow|pair|rw|ww|block] [cycles]");
    Serial.println("  bench list");
#endif
    SLMP_WIFI_SERIAL_CONSOLE_EXTRA_HELP();
    Serial.println("examples:");
    Serial.println("  target 0 255 0x03FF 0");
    Serial.println("  row D100");
    Serial.println("  rdw D200 2");
    Serial.println("  rbits M100 4");
    Serial.println("  rr 2 D100 D101 1 D200");
    Serial.println("  wrand 1 D120 123 1 D200 0x12345678");
    Serial.println("  rblk 1 D300 2 1 M200 1");
    Serial.println("  wblk 1 D300 2 10 20 1 M200 1 0x0005");
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
    Serial.println("  bench");
    Serial.println("  bench row 1000");
#endif
    Serial.println("hex-address devices: X, Y, B, W, SB, SW, DX, DY");
    Serial.println("bit block points use packed 16-bit words");
}
void printTypeName() {
    if (!connectPlc(false)) {
        Serial.println("type failed: plc not connected");
        return;
    }
    slmp::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp::Error::Ok) {
        printLastPlcError("readTypeName");
        return;
    }
    Serial.print("model=");
    Serial.print(type_name.model);
    if (type_name.has_model_code) {
        Serial.print(" code=0x");
        Serial.println(type_name.model_code, HEX);
    } else {
        Serial.println();
    }
}
void readWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rw usage: rw <device> [points]");
        return;
    }
    uint16_t points = 1;
    if (points_token != nullptr && !parsePointCount(points_token, points)) {
        Serial.println("rw points must be 1..8");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("rw failed: plc not connected");
        return;
    }
    uint16_t values[kMaxWordPoints] = {};
    if (plc.readWords(device, points, values, kMaxWordPoints) != slmp::Error::Ok) {
        printLastPlcError("readWords");
        return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i);
        Serial.print("=");
        Serial.print(values[i]);
        Serial.print(" (0x");
        Serial.print(values[i], HEX);
        Serial.println(")");
    }
}
void writeWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("ww usage: ww <device> <value...>");
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("ww device parse failed");
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.println("ww accepts 1..8 values");
        return;
    }
    uint16_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseWordValue(tokens[i + 2], values[i])) {
            Serial.println("ww values must fit in 16 bits");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("ww failed: plc not connected");
        return;
    }
    if (plc.writeWords(device, values, static_cast<size_t>(value_count)) != slmp::Error::Ok) {
        printLastPlcError("writeWords");
        return;
    }
    Serial.println("writeWords ok");
}
void verifyWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("verifyw usage: verifyw <device> <value...>");
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("verifyw device parse failed");
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.println("verifyw accepts 1..8 values");
        return;
    }
    uint16_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseWordValue(tokens[i + 2], values[i])) {
            Serial.println("verifyw values must fit in 16 bits");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("verifyw failed: plc not connected");
        return;
    }
    uint16_t before[kMaxWordPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(value_count), before, kMaxWordPoints) != slmp::Error::Ok) {
        printLastPlcError("verifyw before read");
        return;
    }
    if (plc.writeWords(device, values, static_cast<size_t>(value_count)) != slmp::Error::Ok) {
        printLastPlcError("verifyw write");
        return;
    }
    uint16_t readback[kMaxWordPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(value_count), readback, kMaxWordPoints) != slmp::Error::Ok) {
        printLastPlcError("verifyw readback");
        return;
    }
    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::WordWrite;
    verification.device = device;
    verification.points = static_cast<uint16_t>(value_count);
    verification.started_ms = millis();
    memcpy(verification.before_words, before, sizeof(uint16_t) * static_cast<size_t>(value_count));
    memcpy(verification.written_words, values, sizeof(uint16_t) * static_cast<size_t>(value_count));
    memcpy(verification.readback_words, readback, sizeof(uint16_t) * static_cast<size_t>(value_count));
    verification.readback_match = wordArraysEqual(values, readback, static_cast<uint16_t>(value_count));
    printVerificationSummary();
}
void readBitCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rb usage: rb <device>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("rb failed: plc not connected");
        return;
    }
    bool value = false;
    if (plc.readOneBit(device, value) != slmp::Error::Ok) {
        printLastPlcError("readOneBit");
        return;
    }
    printDeviceAddress(device);
    Serial.print("=");
    Serial.println(value ? 1 : 0);
}
void writeBitCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("wb usage: wb <device> <0|1>");
        return;
    }
    bool value = false;
    if (!parseBoolValue(value_token, value)) {
        Serial.println("wb value must be 0/1, on/off, or true/false");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("wb failed: plc not connected");
        return;
    }
    if (plc.writeOneBit(device, value) != slmp::Error::Ok) {
        printLastPlcError("writeOneBit");
        return;
    }
    Serial.println("writeOneBit ok");
}
void verifyBitCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("verifyb usage: verifyb <device> <0|1>");
        return;
    }
    bool value = false;
    if (!parseBoolValue(value_token, value)) {
        Serial.println("verifyb value must be 0/1, on/off, or true/false");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("verifyb failed: plc not connected");
        return;
    }
    bool before = false;
    if (plc.readOneBit(device, before) != slmp::Error::Ok) {
        printLastPlcError("verifyb before read");
        return;
    }
    if (plc.writeOneBit(device, value) != slmp::Error::Ok) {
        printLastPlcError("verifyb write");
        return;
    }
    bool readback = false;
    if (plc.readOneBit(device, readback) != slmp::Error::Ok) {
        printLastPlcError("verifyb readback");
        return;
    }
    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::BitWrite;
    verification.device = device;
    verification.points = 1;
    verification.started_ms = millis();
    verification.before_bit = before;
    verification.written_bit = value;
    verification.readback_bit = readback;
    verification.readback_match = value == readback;
    printVerificationSummary();
}
bool parsePositiveCount(const char* token, uint16_t max_value, uint16_t& count) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 10) || parsed == 0 || parsed > max_value) {
        return false;
    }
    count = static_cast<uint16_t>(parsed);
    return true;
}
void targetCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        printTargetAddress();
        return;
    }
    if (token_count != 5) {
        Serial.println("target usage: target <network> <station> <module_io> <multidrop>");
        return;
    }
    uint8_t network = 0;
    uint8_t station = 0;
    uint8_t multidrop = 0;
    uint16_t module_io = 0;
    if (!parseByteValue(tokens[1], network) || !parseByteValue(tokens[2], station) ||
        !parseUint16Value(tokens[3], module_io) || !parseByteValue(tokens[4], multidrop)) {
        Serial.println("target values must fit network/station/multidrop=0..255 and module_io=0..65535");
        return;
    }
    slmp::TargetAddress target = plc.target();
    target.network = network;
    target.station = station;
    target.module_io = module_io;
    target.multidrop = multidrop;
    plc.setTarget(target);
    printTargetAddress();
}
void monitorCommand(char* value_token) {
    if (value_token == nullptr) {
        Serial.print("monitoring_timer=");
        Serial.println(plc.monitoringTimer());
        return;
    }
    uint16_t monitoring_timer = 0;
    if (!parseUint16Value(value_token, monitoring_timer)) {
        Serial.println("monitor usage: monitor [value]");
        return;
    }
    plc.setMonitoringTimer(monitoring_timer);
    Serial.print("monitoring_timer=");
    Serial.println(plc.monitoringTimer());
}
void readOneWordCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("row usage: row <device>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("row failed: plc not connected");
        return;
    }
    uint16_t value = 0;
    if (plc.readOneWord(device, value) != slmp::Error::Ok) {
        printLastPlcError("readOneWord");
        return;
    }
    printDeviceAddress(device);
    Serial.print("=");
    Serial.print(value);
    Serial.print(" (0x");
    Serial.print(value, HEX);
    Serial.println(")");
}
void writeOneWordCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("wow usage: wow <device> <value>");
        return;
    }
    uint16_t value = 0;
    if (!parseWordValue(value_token, value)) {
        Serial.println("wow value must fit in 16 bits");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("wow failed: plc not connected");
        return;
    }
    if (plc.writeOneWord(device, value) != slmp::Error::Ok) {
        printLastPlcError("writeOneWord");
        return;
    }
    Serial.println("writeOneWord ok");
}
void readBitsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rbits usage: rbits <device> <points>");
        return;
    }
    uint16_t points = 0;
    if (!parsePositiveCount(points_token, kMaxWordPoints, points)) {
        Serial.println("rbits points must be 1..8");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("rbits failed: plc not connected");
        return;
    }
    bool values[kMaxWordPoints] = {};
    if (plc.readBits(device, points, values, kMaxWordPoints) != slmp::Error::Ok) {
        printLastPlcError("readBits");
        return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i);
        Serial.print("=");
        Serial.println(values[i] ? 1 : 0);
    }
}
void writeBitsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("wbits usage: wbits <device> <0|1...>");
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("wbits device parse failed");
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.println("wbits accepts 1..8 values");
        return;
    }
    bool values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseBoolValue(tokens[i + 2], values[i])) {
            Serial.println("wbits values must be 0/1, on/off, or true/false");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("wbits failed: plc not connected");
        return;
    }
    if (plc.writeBits(device, values, static_cast<size_t>(value_count)) != slmp::Error::Ok) {
        printLastPlcError("writeBits");
        return;
    }
    Serial.println("writeBits ok");
}
void readDWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rdw usage: rdw <device> [points]");
        return;
    }
    uint16_t points = 1;
    if (points_token != nullptr && !parsePositiveCount(points_token, kMaxWordPoints, points)) {
        Serial.println("rdw points must be 1..8");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("rdw failed: plc not connected");
        return;
    }
    uint32_t values[kMaxWordPoints] = {};
    if (plc.readDWords(device, points, values, kMaxWordPoints) != slmp::Error::Ok) {
        printLastPlcError("readDWords");
        return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i * 2U);
        Serial.print("=");
        Serial.print(static_cast<unsigned long>(values[i]));
        Serial.print(" (0x");
        Serial.print(static_cast<unsigned long>(values[i]), HEX);
        Serial.println(")");
    }
}
void writeDWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("wdw usage: wdw <device> <value...>");
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("wdw device parse failed");
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.println("wdw accepts 1..8 values");
        return;
    }
    uint32_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseDWordValue(tokens[i + 2], values[i])) {
            Serial.println("wdw values must fit in 32 bits");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("wdw failed: plc not connected");
        return;
    }
    if (plc.writeDWords(device, values, static_cast<size_t>(value_count)) != slmp::Error::Ok) {
        printLastPlcError("writeDWords");
        return;
    }
    Serial.println("writeDWords ok");
}
void readOneDWordCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rod usage: rod <device>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("rod failed: plc not connected");
        return;
    }
    uint32_t value = 0;
    if (plc.readOneDWord(device, value) != slmp::Error::Ok) {
        printLastPlcError("readOneDWord");
        return;
    }
    printDeviceAddress(device);
    Serial.print("=");
    Serial.print(static_cast<unsigned long>(value));
    Serial.print(" (0x");
    Serial.print(static_cast<unsigned long>(value), HEX);
    Serial.println(")");
}
void writeOneDWordCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("wod usage: wod <device> <value>");
        return;
    }
    uint32_t value = 0;
    if (!parseDWordValue(value_token, value)) {
        Serial.println("wod value must fit in 32 bits");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("wod failed: plc not connected");
        return;
    }
    if (plc.writeOneDWord(device, value) != slmp::Error::Ok) {
        printLastPlcError("writeOneDWord");
        return;
    }
    Serial.println("writeOneDWord ok");
}
void readRandomCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("rr usage: rr <word_count> <word_devices...> <dword_count> <dword_devices...>");
        return;
    }
    int index = 1;
    uint8_t word_count = 0;
    if (!parseListCount(tokens[index++], kMaxRandomWordDevices, word_count)) {
        Serial.println("rr word_count must be 0..8");
        return;
    }
    if (token_count < index + word_count + 1) {
        Serial.println("rr usage: rr <word_count> <word_devices...> <dword_count> <dword_devices...>");
        return;
    }
    slmp::DeviceAddress word_devices[kMaxRandomWordDevices] = {};
    for (uint8_t i = 0; i < word_count; ++i) {
        if (!parseDeviceAddress(tokens[index++], word_devices[i])) {
            Serial.println("rr word device parse failed");
            return;
        }
    }
    uint8_t dword_count = 0;
    if (!parseListCount(tokens[index++], kMaxRandomDWordDevices, dword_count)) {
        Serial.println("rr dword_count must be 0..8");
        return;
    }
    if (word_count == 0 && dword_count == 0) {
        Serial.println("rr requires at least one device");
        return;
    }
    if (token_count != index + dword_count) {
        Serial.println("rr usage: rr <word_count> <word_devices...> <dword_count> <dword_devices...>");
        return;
    }
    slmp::DeviceAddress dword_devices[kMaxRandomDWordDevices] = {};
    for (uint8_t i = 0; i < dword_count; ++i) {
        if (!parseDeviceAddress(tokens[index++], dword_devices[i])) {
            Serial.println("rr dword device parse failed");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("rr failed: plc not connected");
        return;
    }
    uint16_t word_values[kMaxRandomWordDevices] = {};
    uint32_t dword_values[kMaxRandomDWordDevices] = {};
    if (plc.readRandom(
            word_devices,
            word_count,
            word_values,
            kMaxRandomWordDevices,
            dword_devices,
            dword_count,
            dword_values,
            kMaxRandomDWordDevices
        ) != slmp::Error::Ok) {
        printLastPlcError("readRandom");
        return;
    }
    for (uint8_t i = 0; i < word_count; ++i) {
        printDeviceAddress(word_devices[i]);
        Serial.print("=");
        Serial.print(word_values[i]);
        Serial.print(" (0x");
        Serial.print(word_values[i], HEX);
        Serial.println(")");
    }
    for (uint8_t i = 0; i < dword_count; ++i) {
        printDeviceAddress(dword_devices[i]);
        Serial.print("=");
        Serial.print(static_cast<unsigned long>(dword_values[i]));
        Serial.print(" (0x");
        Serial.print(static_cast<unsigned long>(dword_values[i]), HEX);
        Serial.println(")");
    }
}
void writeRandomWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("wrand usage: wrand <word_count> <device value...> <dword_count> <device value...>");
        return;
    }
    int index = 1;
    uint8_t word_count = 0;
    if (!parseListCount(tokens[index++], kMaxRandomWordDevices, word_count)) {
        Serial.println("wrand word_count must be 0..8");
        return;
    }
    if (token_count < index + (word_count * 2) + 1) {
        Serial.println("wrand usage: wrand <word_count> <device value...> <dword_count> <device value...>");
        return;
    }
    slmp::DeviceAddress word_devices[kMaxRandomWordDevices] = {};
    uint16_t word_values[kMaxRandomWordDevices] = {};
    for (uint8_t i = 0; i < word_count; ++i) {
        if (!parseDeviceAddress(tokens[index++], word_devices[i])) {
            Serial.println("wrand word device parse failed");
            return;
        }
        if (!parseWordValue(tokens[index++], word_values[i])) {
            Serial.println("wrand word values must fit in 16 bits");
            return;
        }
    }
    uint8_t dword_count = 0;
    if (!parseListCount(tokens[index++], kMaxRandomDWordDevices, dword_count)) {
        Serial.println("wrand dword_count must be 0..8");
        return;
    }
    if (word_count == 0 && dword_count == 0) {
        Serial.println("wrand requires at least one device");
        return;
    }
    if (token_count != index + (dword_count * 2)) {
        Serial.println("wrand usage: wrand <word_count> <device value...> <dword_count> <device value...>");
        return;
    }
    slmp::DeviceAddress dword_devices[kMaxRandomDWordDevices] = {};
    uint32_t dword_values[kMaxRandomDWordDevices] = {};
    for (uint8_t i = 0; i < dword_count; ++i) {
        if (!parseDeviceAddress(tokens[index++], dword_devices[i])) {
            Serial.println("wrand dword device parse failed");
            return;
        }
        if (!parseDWordValue(tokens[index++], dword_values[i])) {
            Serial.println("wrand dword values must fit in 32 bits");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("wrand failed: plc not connected");
        return;
    }
    if (plc.writeRandomWords(
            word_devices,
            word_values,
            word_count,
            dword_devices,
            dword_values,
            dword_count
        ) != slmp::Error::Ok) {
        printLastPlcError("writeRandomWords");
        return;
    }
    Serial.println("writeRandomWords ok");
}
void writeRandomBitsCommand(char* tokens[], int token_count) {
    if (token_count < 4) {
        Serial.println("wrandb usage: wrandb <count> <device value...>");
        return;
    }
    int index = 1;
    uint8_t bit_count = 0;
    if (!parseListCount(tokens[index++], kMaxRandomBitDevices, bit_count) || bit_count == 0) {
        Serial.println("wrandb count must be 1..8");
        return;
    }
    if (token_count != index + (bit_count * 2)) {
        Serial.println("wrandb usage: wrandb <count> <device value...>");
        return;
    }
    slmp::DeviceAddress bit_devices[kMaxRandomBitDevices] = {};
    bool bit_values[kMaxRandomBitDevices] = {};
    for (uint8_t i = 0; i < bit_count; ++i) {
        if (!parseDeviceAddress(tokens[index++], bit_devices[i])) {
            Serial.println("wrandb device parse failed");
            return;
        }
        if (!parseBoolValue(tokens[index++], bit_values[i])) {
            Serial.println("wrandb values must be 0/1, on/off, or true/false");
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println("wrandb failed: plc not connected");
        return;
    }
    if (plc.writeRandomBits(bit_devices, bit_values, bit_count) != slmp::Error::Ok) {
        printLastPlcError("writeRandomBits");
        return;
    }
    Serial.println("writeRandomBits ok");
}
void readBlockCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("rblk usage: rblk <word_blocks> <device points...> <bit_blocks> <device points...>");
        return;
    }
    int index = 1;
    uint8_t word_block_count = 0;
    if (!parseListCount(tokens[index++], kMaxBlockCount, word_block_count)) {
        Serial.println("rblk word_blocks must be 0..4");
        return;
    }
    if (token_count < index + (word_block_count * 2) + 1) {
        Serial.println("rblk usage: rblk <word_blocks> <device points...> <bit_blocks> <device points...>");
        return;
    }
    slmp::DeviceBlockRead word_blocks[kMaxBlockCount] = {};
    uint16_t total_word_points = 0;
    for (uint8_t i = 0; i < word_block_count; ++i) {
        slmp::DeviceAddress device = {};
        uint16_t points = 0;
        if (!parseDeviceAddress(tokens[index++], device)) {
            Serial.println("rblk word block device parse failed");
            return;
        }
        if (!parsePositiveCount(tokens[index++], kMaxBlockPoints, points) ||
            total_word_points + points > kMaxBlockPoints) {
            Serial.println("rblk word block points must keep total within 1..16");
            return;
        }
        word_blocks[i] = {device, points};
        total_word_points = static_cast<uint16_t>(total_word_points + points);
    }
    uint8_t bit_block_count = 0;
    if (!parseListCount(tokens[index++], kMaxBlockCount, bit_block_count)) {
        Serial.println("rblk bit_blocks must be 0..4");
        return;
    }
    if (word_block_count == 0 && bit_block_count == 0) {
        Serial.println("rblk requires at least one block");
        return;
    }
    if (token_count != index + (bit_block_count * 2)) {
        Serial.println("rblk usage: rblk <word_blocks> <device points...> <bit_blocks> <device points...>");
        return;
    }
    slmp::DeviceBlockRead bit_blocks[kMaxBlockCount] = {};
    uint16_t total_bit_points = 0;
    for (uint8_t i = 0; i < bit_block_count; ++i) {
        slmp::DeviceAddress device = {};
        uint16_t points = 0;
        if (!parseDeviceAddress(tokens[index++], device)) {
            Serial.println("rblk bit block device parse failed");
            return;
        }
        if (!parsePositiveCount(tokens[index++], kMaxBlockPoints, points) ||
            total_bit_points + points > kMaxBlockPoints) {
            Serial.println("rblk bit block points must keep total within 1..16 packed words");
            return;
        }
        bit_blocks[i] = {device, points};
        total_bit_points = static_cast<uint16_t>(total_bit_points + points);
    }
    if (!connectPlc(false)) {
        Serial.println("rblk failed: plc not connected");
        return;
    }
    uint16_t word_values[kMaxBlockPoints] = {};
    uint16_t bit_values[kMaxBlockPoints] = {};
    if (plc.readBlock(
            word_blocks,
            word_block_count,
            bit_blocks,
            bit_block_count,
            word_values,
            kMaxBlockPoints,
            bit_values,
            kMaxBlockPoints
        ) != slmp::Error::Ok) {
        printLastPlcError("readBlock");
        return;
    }
    uint16_t word_offset = 0;
    for (uint8_t i = 0; i < word_block_count; ++i) {
        for (uint16_t j = 0; j < word_blocks[i].points; ++j) {
            printDeviceAddress(word_blocks[i].device, j);
            Serial.print("=");
            Serial.print(word_values[word_offset + j]);
            Serial.print(" (0x");
            Serial.print(word_values[word_offset + j], HEX);
            Serial.println(")");
        }
        word_offset = static_cast<uint16_t>(word_offset + word_blocks[i].points);
    }
    uint16_t bit_offset = 0;
    for (uint8_t i = 0; i < bit_block_count; ++i) {
        for (uint16_t j = 0; j < bit_blocks[i].points; ++j) {
            printDeviceAddress(bit_blocks[i].device, static_cast<uint32_t>(j) * 16U);
            Serial.print(" packed=0x");
            Serial.println(bit_values[bit_offset + j], HEX);
        }
        bit_offset = static_cast<uint16_t>(bit_offset + bit_blocks[i].points);
    }
}
void writeBlockCommand(char* tokens[], int token_count) {
    if (token_count < 4) {
        Serial.println("wblk usage: wblk <word_blocks> <device points values...> <bit_blocks> <device points packed...>");
        return;
    }
    int index = 1;
    uint8_t word_block_count = 0;
    if (!parseListCount(tokens[index++], kMaxBlockCount, word_block_count)) {
        Serial.println("wblk word_blocks must be 0..4");
        return;
    }
    slmp::DeviceBlockWrite word_blocks[kMaxBlockCount] = {};
    uint16_t word_storage[kMaxBlockPoints] = {};
    uint16_t word_offset = 0;
    for (uint8_t i = 0; i < word_block_count; ++i) {
        if (token_count <= index + 1) {
            Serial.println("wblk word block definition incomplete");
            return;
        }
        slmp::DeviceAddress device = {};
        uint16_t points = 0;
        if (!parseDeviceAddress(tokens[index++], device)) {
            Serial.println("wblk word block device parse failed");
            return;
        }
        if (!parsePositiveCount(tokens[index++], kMaxBlockPoints, points) || word_offset + points > kMaxBlockPoints) {
            Serial.println("wblk word block points must keep total within 1..16");
            return;
        }
        if (token_count < index + points) {
            Serial.println("wblk word block values missing");
            return;
        }
        word_blocks[i].device = device;
        word_blocks[i].values = word_storage + word_offset;
        word_blocks[i].points = points;
        for (uint16_t j = 0; j < points; ++j) {
            if (!parseWordValue(tokens[index++], word_storage[word_offset + j])) {
                Serial.println("wblk word values must fit in 16 bits");
                return;
            }
        }
        word_offset = static_cast<uint16_t>(word_offset + points);
    }
    if (token_count <= index) {
        Serial.println("wblk bit block count missing");
        return;
    }
    uint8_t bit_block_count = 0;
    if (!parseListCount(tokens[index++], kMaxBlockCount, bit_block_count)) {
        Serial.println("wblk bit_blocks must be 0..4");
        return;
    }
    if (word_block_count == 0 && bit_block_count == 0) {
        Serial.println("wblk requires at least one block");
        return;
    }
    slmp::DeviceBlockWrite bit_blocks[kMaxBlockCount] = {};
    uint16_t bit_storage[kMaxBlockPoints] = {};
    uint16_t bit_offset = 0;
    for (uint8_t i = 0; i < bit_block_count; ++i) {
        if (token_count <= index + 1) {
            Serial.println("wblk bit block definition incomplete");
            return;
        }
        slmp::DeviceAddress device = {};
        uint16_t points = 0;
        if (!parseDeviceAddress(tokens[index++], device)) {
            Serial.println("wblk bit block device parse failed");
            return;
        }
        if (!parsePositiveCount(tokens[index++], kMaxBlockPoints, points) || bit_offset + points > kMaxBlockPoints) {
            Serial.println("wblk bit block points must keep total within 1..16 packed words");
            return;
        }
        if (token_count < index + points) {
            Serial.println("wblk bit block packed values missing");
            return;
        }
        bit_blocks[i].device = device;
        bit_blocks[i].values = bit_storage + bit_offset;
        bit_blocks[i].points = points;
        for (uint16_t j = 0; j < points; ++j) {
            if (!parseWordValue(tokens[index++], bit_storage[bit_offset + j])) {
                Serial.println("wblk packed bit values must fit in 16 bits");
                return;
            }
        }
        bit_offset = static_cast<uint16_t>(bit_offset + points);
    }
    if (token_count != index) {
        Serial.println("wblk usage: wblk <word_blocks> <device points values...> <bit_blocks> <device points packed...>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("wblk failed: plc not connected");
        return;
    }
    if (plc.writeBlock(word_blocks, word_block_count, bit_blocks, bit_block_count) != slmp::Error::Ok) {
        printLastPlcError("writeBlock");
        return;
    }
    Serial.println("writeBlock ok");
}
void remotePasswordUnlockCommand(char* password_token) {
    if (password_token == nullptr) {
        Serial.println("unlock usage: unlock <password>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("unlock failed: plc not connected");
        return;
    }
    if (plc.remotePasswordUnlock(password_token) != slmp::Error::Ok) {
        printLastPlcError("remotePasswordUnlock");
        return;
    }
    Serial.println("remotePasswordUnlock ok");
}
void remotePasswordLockCommand(char* password_token) {
    if (password_token == nullptr) {
        Serial.println("lock usage: lock <password>");
        return;
    }
    if (!connectPlc(false)) {
        Serial.println("lock failed: plc not connected");
        return;
    }
    if (plc.remotePasswordLock(password_token) != slmp::Error::Ok) {
        printLastPlcError("remotePasswordLock");
        return;
    }
    Serial.println("remotePasswordLock ok");
}
void judgeCommand(char* tokens[], int token_count) {
    if (!verification.active) {
        Serial.println("judge failed: no active verification");
        return;
    }
    if (token_count < 2 || tokens[1] == nullptr) {
        Serial.println("judge usage: judge <ok|ng> [note]");
        return;
    }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "OK") == 0 || strcmp(tokens[1], "PASS") == 0) {
        verification.pass = true;
    } else if (strcmp(tokens[1], "NG") == 0 || strcmp(tokens[1], "FAIL") == 0) {
        verification.pass = false;
    } else {
        Serial.println("judge usage: judge <ok|ng> [note]");
        return;
    }
    verification.judged = true;
    joinTokens(tokens, 2, token_count, verification.note, sizeof(verification.note));
    printVerificationSummary();
}
void setTimeoutCommand(char* value_token) {
    unsigned long timeout_ms = 0;
    if (!parseUnsignedValue(value_token, timeout_ms, 10)) {
        Serial.println("timeout usage: timeout <ms>");
        return;
    }
    plc.setTimeoutMs(static_cast<uint32_t>(timeout_ms));
    Serial.print("timeout_ms=");
    Serial.println(plc.timeoutMs());
}

#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
void benchCommand(char* tokens[], int token_count) {
    if (token_count >= 2) {
        uppercaseInPlace(tokens[1]);
        if (strcmp(tokens[1], "LIST") == 0) {
            printBenchList();
            return;
        }
    }

    BenchMode mode = BenchMode::Pair;
    uint32_t cycles = kBenchDefaultCycles;
    if (token_count >= 2 && !parseBenchModeToken(tokens[1], mode)) {
        Serial.println("bench usage: bench [row|wow|pair|rw|ww|block] [cycles]");
        return;
    }
    if (token_count >= 3) {
        unsigned long parsed_cycles = 0;
        if (!parseUnsignedValue(tokens[2], parsed_cycles, 10) || parsed_cycles == 0) {
            Serial.println("bench usage: bench [row|wow|pair|rw|ww|block] [cycles]");
            return;
        }
        cycles = static_cast<uint32_t>(parsed_cycles);
    }

    Serial.print("bench start mode=");
    Serial.print(benchModeText(mode));
    Serial.print(" cycles=");
    Serial.println(cycles);
    (void)runBench(mode, cycles);
}
#endif
int splitTokens(char* line, char* tokens[], int token_capacity) {
    int count = 0;
    char* cursor = line;
    while (*cursor != '\0' && count < token_capacity) {
        while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor)) != 0) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        tokens[count++] = cursor;
        while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor)) == 0) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor++ = '\0';
    }
    return count;
}
void executeCommandLine(char* line, bool print_prompt) {
    char* tokens[kMaxTokens] = {};
    const int token_count = splitTokens(line, tokens, static_cast<int>(kMaxTokens));
    if (token_count == 0) {
        if (print_prompt) {
            printPrompt();
        }
        return;
    }
    uppercaseInPlace(tokens[0]);
    if (strcmp(tokens[0], "HELP") == 0 || strcmp(tokens[0], "?") == 0) {
        if (!SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_HELP_HANDLER(tokens, token_count)) {
            printHelp();
        }
    } else if (strcmp(tokens[0], "STATUS") == 0) {
        printStatus();
    } else if (strcmp(tokens[0], "CONNECT") == 0) {
        (void)connectPlc(true);
    } else if (strcmp(tokens[0], "CLOSE") == 0) {
        closePlc();
    } else if (strcmp(tokens[0], "REINIT") == 0) {
        reinitializeWiFi();
    } else if (strcmp(tokens[0], "TYPE") == 0) {
        printTypeName();
    } else if (strcmp(tokens[0], "TARGET") == 0) {
        targetCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "MONITOR") == 0 || strcmp(tokens[0], "MON") == 0) {
        monitorCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "RW") == 0) {
        readWordsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "WW") == 0) {
        writeWordsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "ROW") == 0) {
        readOneWordCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "WOW") == 0) {
        writeOneWordCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "RB") == 0) {
        readBitCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "WB") == 0) {
        writeBitCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "RBITS") == 0 || strcmp(tokens[0], "RBS") == 0) {
        readBitsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "WBITS") == 0 || strcmp(tokens[0], "WBS") == 0) {
        writeBitsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "RDW") == 0) {
        readDWordsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "WDW") == 0) {
        writeDWordsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "ROD") == 0) {
        readOneDWordCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "WOD") == 0) {
        writeOneDWordCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "RR") == 0) {
        readRandomCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "WRAND") == 0) {
        writeRandomWordsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "WRANDB") == 0) {
        writeRandomBitsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "RBLK") == 0) {
        readBlockCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "WBLK") == 0) {
        writeBlockCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "UNLOCK") == 0) {
        remotePasswordUnlockCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "LOCK") == 0) {
        remotePasswordLockCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "VERIFYW") == 0 || strcmp(tokens[0], "VW") == 0) {
        verifyWordsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "VERIFYB") == 0 || strcmp(tokens[0], "VB") == 0) {
        verifyBitCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "PENDING") == 0) {
        printVerificationSummary();
    } else if (strcmp(tokens[0], "JUDGE") == 0) {
        judgeCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "TIMEOUT") == 0) {
        setTimeoutCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "DUMP") == 0) {
        printLastFrames();
#if !SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH
    } else if (strcmp(tokens[0], "BENCH") == 0 || strcmp(tokens[0], "PERF") == 0) {
        benchCommand(tokens, token_count);
#endif
    } else if (SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_COMMAND_HANDLER(tokens, token_count)) {
    } else {
        Serial.print("unknown command: ");
        Serial.println(tokens[0]);
        printHelp();
    }
    if (print_prompt) {
        printPrompt();
    }
}
void handleCommand(char* line) {
    executeCommandLine(line, true);
}
void runStartupDemo() {
    if (!connectPlc(false)) {
        Serial.println("startup read skipped: plc not connected");
        return;
    }
    printTypeName();
    readWordsCommand(const_cast<char*>("D100"), const_cast<char*>("2"));
}
void setup() {
    Serial.begin(115200);
    const uint32_t serial_wait_started_ms = millis();
    while (!Serial) {
        if (SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS == 0 ||
            millis() - serial_wait_started_ms >= SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS) {
            break;
        }
        delay(10);
    }
    Serial.println(SLMP_WIFI_SERIAL_CONSOLE_BANNER);
    plc.setTimeoutMs(2000);
    (void)bringUpWiFi();
    printHelp();
    runStartupDemo();
    printPrompt();
}
void loop() {
    while (Serial.available() > 0) {
        const int raw = Serial.read();
        if (raw < 0) {
            break;
        }
        const char ch = static_cast<char>(raw);
        if (ch == '\r' || ch == '\n') {
            if (ch == '\n' && serial_last_was_cr) {
                serial_last_was_cr = false;
                continue;
            }
            serial_last_was_cr = (ch == '\r');
            Serial.println();
            submitSerialLine();
            continue;
        }
        serial_last_was_cr = false;
        if (ch == '\b' || ch == 127) {
            if (serial_line_length > 0) {
                --serial_line_length;
                serial_line[serial_line_length] = '\0';
                Serial.print("\b \b");
            }
            continue;
        }
        if (!isprint(static_cast<unsigned char>(ch)) && ch != '\t') {
            continue;
        }
        if (serial_line_length + 1 >= sizeof(serial_line)) {
            Serial.println();
            Serial.println("command too long");
            serial_line_length = 0;
            serial_line[0] = '\0';
            printPrompt();
            continue;
        }
        Serial.print(ch);
        serial_line[serial_line_length++] = ch;
    }
}
}  // namespace SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE
