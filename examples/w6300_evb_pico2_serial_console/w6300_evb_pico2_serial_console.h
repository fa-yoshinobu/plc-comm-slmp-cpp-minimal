#pragma once

#include <Arduino.h>

#if __has_include(<W6300lwIP.h>)
#include <W6300lwIP.h>
#else
#error "This example requires Arduino-Pico with W6300lwIP support."
#endif

#if __has_include(<WiFiClient.h>)
#include <WiFiClient.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <slmp_arduino_transport.h>

namespace w6300_evb_pico2_serial_console {

constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;
const IPAddress kLocalIp(192, 168, 250, 52);
const IPAddress kDns(192, 168, 250, 1);
const IPAddress kGateway(192, 168, 250, 1);
const IPAddress kSubnet(255, 255, 255, 0);

constexpr uint8_t kEthernetCsPin = 16;
constexpr uint8_t kEthernetResetPin = 22;

constexpr size_t kSerialLineCapacity = 384;
constexpr size_t kMaxTokens = 48;
constexpr size_t kMaxWordPoints = 8;
constexpr size_t kVerificationNoteCapacity = 80;
constexpr size_t kMaxRandomWordDevices = 8;
constexpr size_t kMaxRandomDWordDevices = 8;
constexpr size_t kMaxRandomBitDevices = 8;
constexpr size_t kMaxBlockCount = 4;
constexpr size_t kMaxBlockPoints = 16;
constexpr size_t kTxBufferSize = 512;
constexpr size_t kRxBufferSize = 512;
constexpr uint32_t kEnduranceCycleGapMs = 20;
constexpr uint32_t kEnduranceReportIntervalMs = 5000;
constexpr uint32_t kReconnectCycleGapMs = 250;
constexpr uint32_t kReconnectMaxCycleGapMs = 5000;
constexpr uint32_t kReconnectReportIntervalMs = 5000;
constexpr size_t kSlmpRequestHeaderSize = 19;
constexpr size_t kTxPayloadBudget = kTxBufferSize > kSlmpRequestHeaderSize
                                        ? (kTxBufferSize - kSlmpRequestHeaderSize)
                                        : 0;
constexpr uint32_t kBenchReportIntervalMs = 3000;
constexpr uint32_t kBenchDefaultCycles = 1000;
constexpr size_t kBenchWordPoints = kMaxWordPoints;
constexpr size_t kBenchBlockPoints = kMaxBlockPoints;
constexpr slmp::DeviceAddress kFuncheckOneWordDevice = {slmp::DeviceCode::D, 120};
constexpr slmp::DeviceAddress kFuncheckWordArrayDevice = {slmp::DeviceCode::D, 130};
constexpr slmp::DeviceAddress kFuncheckOneBitDevice = {slmp::DeviceCode::M, 120};
constexpr slmp::DeviceAddress kFuncheckBitArrayDevice = {slmp::DeviceCode::M, 130};
constexpr slmp::DeviceAddress kFuncheckOneDWordDevice = {slmp::DeviceCode::D, 220};
constexpr slmp::DeviceAddress kFuncheckDWordArrayDevice = {slmp::DeviceCode::D, 230};
constexpr slmp::DeviceAddress kFuncheckRandomWordDevices[] = {
    {slmp::DeviceCode::D, 140},
    {slmp::DeviceCode::D, 141},
};
constexpr slmp::DeviceAddress kFuncheckRandomDWordDevices[] = {
    {slmp::DeviceCode::D, 240},
};
constexpr slmp::DeviceAddress kFuncheckRandomBitDevices[] = {
    {slmp::DeviceCode::M, 140},
    {slmp::DeviceCode::M, 141},
};
constexpr slmp::DeviceAddress kFuncheckBlockWordDevice = {slmp::DeviceCode::D, 300};
constexpr slmp::DeviceAddress kFuncheckBlockBitDevice = {slmp::DeviceCode::M, 200};
constexpr slmp::DeviceAddress kEnduranceOneWordDevice = {slmp::DeviceCode::D, 500};
constexpr slmp::DeviceAddress kEnduranceWordArrayDevice = {slmp::DeviceCode::D, 510};
constexpr slmp::DeviceAddress kEnduranceOneBitDevice = {slmp::DeviceCode::M, 500};
constexpr slmp::DeviceAddress kEnduranceBitArrayDevice = {slmp::DeviceCode::M, 510};
constexpr slmp::DeviceAddress kEnduranceOneDWordDevice = {slmp::DeviceCode::D, 600};
constexpr slmp::DeviceAddress kEnduranceDWordArrayDevice = {slmp::DeviceCode::D, 610};
constexpr slmp::DeviceAddress kEnduranceRandomWordDevices[] = {
    {slmp::DeviceCode::D, 520},
    {slmp::DeviceCode::D, 521},
};
constexpr slmp::DeviceAddress kEnduranceRandomDWordDevices[] = {
    {slmp::DeviceCode::D, 620},
};
constexpr slmp::DeviceAddress kEnduranceRandomBitDevices[] = {
    {slmp::DeviceCode::M, 520},
    {slmp::DeviceCode::M, 521},
};
constexpr slmp::DeviceAddress kEnduranceBlockWordDevice = {slmp::DeviceCode::D, 540};
constexpr slmp::DeviceAddress kEnduranceBlockBitDevice = {slmp::DeviceCode::M, 540};
constexpr slmp::DeviceAddress kTxLimitWordDevice = {slmp::DeviceCode::D, 700};
constexpr slmp::DeviceAddress kTxLimitBlockWordDevice = {slmp::DeviceCode::D, 1100};
constexpr slmp::DeviceAddress kBenchOneWordDevice = {slmp::DeviceCode::D, 800};
constexpr slmp::DeviceAddress kBenchWordArrayDevice = {slmp::DeviceCode::D, 820};
constexpr slmp::DeviceAddress kBenchBlockWordDevice = {slmp::DeviceCode::D, 900};
constexpr size_t kTxLimitWriteWordsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 2U) : 0U;
constexpr size_t kTxLimitWriteDWordsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 4U) : 0U;
constexpr size_t kTxLimitWriteBitsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) * 2U) : 0U;
constexpr size_t kTxLimitWriteRandomBitsMaxCount = kTxPayloadBudget >= 1U ? ((kTxPayloadBudget - 1U) / 8U) : 0U;
constexpr size_t kTxLimitWriteRandomWordsMaxCount = kTxPayloadBudget >= 2U ? ((kTxPayloadBudget - 2U) / 8U) : 0U;
constexpr size_t kTxLimitWriteBlockWordMaxPoints = kTxPayloadBudget >= 10U ? ((kTxPayloadBudget - 10U) / 2U) : 0U;
static_assert(kTxLimitWriteWordsMaxCount > 0, "txlimit writeWords max count must be positive");
static_assert(kTxLimitWriteBlockWordMaxPoints > 0, "txlimit block max points must be positive");

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

enum class FuncheckResult : uint8_t {
    Ok = 0,
    Fail,
};

struct FuncheckSummary {
    uint32_t ok = 0;
    uint32_t fail = 0;
};

struct EnduranceSession {
    bool active = false;
    uint32_t started_ms = 0;
    uint32_t next_cycle_due_ms = 0;
    uint32_t last_report_ms = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    uint32_t cycle_limit = 0;
    uint32_t attempts = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    char last_step[48] = {};
    char last_issue[64] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};

struct ReconnectSession {
    bool active = false;
    uint32_t started_ms = 0;
    uint32_t next_cycle_due_ms = 0;
    uint32_t last_report_ms = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    uint32_t cycle_limit = 0;
    uint32_t attempts = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    uint32_t recoveries = 0;
    uint32_t consecutive_failures = 0;
    uint32_t max_consecutive_failures = 0;
    char last_step[48] = {};
    char last_issue[64] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};

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

Wiznet6300lwIP Ethernet(kEthernetCsPin);
WiFiClient tcp_client;
slmp::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[kTxBufferSize];
uint8_t rx_buffer[kRxBufferSize];
slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

char serial_line[kSerialLineCapacity] = {};
size_t serial_line_length = 0;
bool ethernet_ready = false;
VerificationRecord verification = {};
EnduranceSession endurance = {};
ReconnectSession reconnect = {};
uint32_t funcheck_prng_state = 0x13579BDFU;
uint16_t txlimit_word_values[kTxLimitWriteWordsMaxCount + 1U] = {};
uint16_t txlimit_word_readback[kTxLimitWriteWordsMaxCount + 1U] = {};
uint16_t txlimit_block_values[kTxLimitWriteBlockWordMaxPoints + 1U] = {};
uint16_t txlimit_block_readback[kTxLimitWriteBlockWordMaxPoints + 1U] = {};
uint16_t bench_word_values[kBenchWordPoints] = {};
uint16_t bench_word_readback[kBenchWordPoints] = {};
uint16_t bench_block_values[kBenchBlockPoints] = {};
uint16_t bench_block_readback[kBenchBlockPoints] = {};

void stopEndurance(bool print_summary, bool failed);
void stopReconnect(bool print_summary);

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

bool dwordArraysEqual(const uint32_t* lhs, const uint32_t* rhs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

bool bitArraysEqual(const bool* lhs, const bool* rhs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

uint16_t readFrameLe16(const uint8_t* bytes) {
    if (bytes == nullptr) {
        return 0;
    }
    return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8U);
}

void copyText(char* out, size_t out_capacity, const char* text) {
    if (out == nullptr || out_capacity == 0) {
        return;
    }
    size_t index = 0;
    if (text != nullptr) {
        for (; index + 1 < out_capacity && text[index] != '\0'; ++index) {
            out[index] = text[index];
        }
    }
    out[index] = '\0';
}

uint32_t nextFuncheckRandom() {
    if (funcheck_prng_state == 0U) {
        funcheck_prng_state = static_cast<uint32_t>(micros()) ^ 0xA341316CU;
    }
    funcheck_prng_state ^= funcheck_prng_state << 13U;
    funcheck_prng_state ^= funcheck_prng_state >> 17U;
    funcheck_prng_state ^= funcheck_prng_state << 5U;
    return funcheck_prng_state;
}

uint16_t nextFuncheckWordValue() {
    const uint16_t value = static_cast<uint16_t>((nextFuncheckRandom() % 60000U) + 1U);
    return value == 0U ? 1U : value;
}

uint32_t nextFuncheckDWordValue() {
    const uint32_t value = (nextFuncheckRandom() & 0x7FFFFFFFU) + 1U;
    return value == 0U ? 1U : value;
}

void printFuncheckWordValues(const char* label, const uint16_t* values, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        Serial.print(values[i]);
    }
    Serial.println();
}

void printFuncheckDWordValues(const char* label, const uint32_t* values, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        Serial.print(static_cast<unsigned long>(values[i]));
    }
    Serial.println();
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

void pulseEthernetReset() {
    pinMode(kEthernetResetPin, OUTPUT);
    digitalWrite(kEthernetResetPin, LOW);
    delay(100);
    digitalWrite(kEthernetResetPin, HIGH);
    delay(100);
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

bool bringUpEthernet() {
    pulseEthernetReset();
    Ethernet.config(kLocalIp, kGateway, kSubnet, kDns);

    if (!Ethernet.begin()) {
        ethernet_ready = false;
        Serial.println("ethernet begin failed");
        return false;
    }

    const IPAddress local_ip = Ethernet.localIP();
    ethernet_ready = local_ip == kLocalIp;

    Serial.println("network_mode=static");
    Serial.print("local ip=");
    Serial.println(local_ip);
    Serial.print("gateway=");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("subnet=");
    Serial.println(Ethernet.subnetMask());
    Serial.print("dns=");
    Serial.println(Ethernet.dnsIP());
    Serial.print("ethernet_connected=");
    Serial.println(Ethernet.connected() ? "yes" : "no");
    if (!ethernet_ready) {
        Serial.println("static IP apply failed");
    }
    return ethernet_ready;
}

bool ensureEthernet() {
    if (ethernet_ready) {
        return true;
    }
    return bringUpEthernet();
}

bool connectPlc(bool verbose) {
    if (!ensureEthernet()) {
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

void reinitializeEthernet() {
    plc.close();
    ethernet_ready = false;
    (void)bringUpEthernet();
}

void printStatus() {
    Serial.print("local ip=");
    Serial.println(Ethernet.localIP());
    Serial.print("gateway=");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("subnet=");
    Serial.println(Ethernet.subnetMask());
    Serial.print("dns=");
    Serial.println(Ethernet.dnsIP());
    Serial.print("ethernet_connected=");
    Serial.println(Ethernet.connected() ? "yes" : "no");
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
    Serial.print("endurance_active=");
    Serial.println(endurance.active ? "yes" : "no");
    Serial.print("reconnect_active=");
    Serial.println(reconnect.active ? "yes" : "no");
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
        Serial.println("observe machine/hmi and enter: judge ok [note] or judge ng [note]");
        return;
    }

    Serial.print("manual_judgement=");
    Serial.println(verification.pass ? "ok" : "ng");
    if (verification.note[0] != '\0') {
        Serial.print("note=");
        Serial.println(verification.note);
    }
}

void recordFuncheckResult(FuncheckSummary& summary, FuncheckResult result) {
    if (result == FuncheckResult::Ok) {
        ++summary.ok;
        return;
    }
    ++summary.fail;
}

const char* funcheckResultText(FuncheckResult result) {
    return result == FuncheckResult::Ok ? "ok" : "fail";
}

void clearWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    uint16_t zeros[kMaxWordPoints] = {};
    (void)plc.writeWords(device, zeros, count);
}

void clearWordRangeSilently(const slmp::DeviceAddress& device, size_t count) {
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

void clearBitsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    if (count == 1) {
        (void)plc.writeOneBit(device, false);
        return;
    }
    bool zeros[kMaxWordPoints] = {};
    (void)plc.writeBits(device, zeros, count);
}

void clearDWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    if (count == 1) {
        (void)plc.writeOneDWord(device, 0U);
        return;
    }
    uint32_t zeros[kMaxWordPoints] = {};
    (void)plc.writeDWords(device, zeros, count);
}

void clearRandomWordsSilently(
    const slmp::DeviceAddress* word_devices,
    size_t word_count,
    const slmp::DeviceAddress* dword_devices,
    size_t dword_count
) {
    if (!connectPlc(false)) {
        return;
    }
    uint16_t zero_words[kMaxRandomWordDevices] = {};
    uint32_t zero_dwords[kMaxRandomDWordDevices] = {};
    (void)plc.writeRandomWords(word_devices, zero_words, word_count, dword_devices, zero_dwords, dword_count);
}

void clearRandomBitsSilently(const slmp::DeviceAddress* bit_devices, size_t bit_count) {
    if (!connectPlc(false)) {
        return;
    }
    bool zero_bits[kMaxRandomBitDevices] = {};
    (void)plc.writeRandomBits(bit_devices, zero_bits, bit_count);
}

void clearPackedBitWordsSilently(const slmp::DeviceAddress& device, uint16_t packed_word_count) {
    if (!connectPlc(false)) {
        return;
    }
    uint16_t zero_words[kMaxBlockPoints] = {};
    if (packed_word_count <= kMaxBlockPoints) {
        const slmp::DeviceBlockWrite bit_block = {device, zero_words, packed_word_count};
        if (plc.writeBlock(nullptr, 0, &bit_block, 1) == slmp::Error::Ok) {
            return;
        }
    }
    const uint32_t bit_count = static_cast<uint32_t>(packed_word_count) * 16U;
    for (uint32_t offset = 0; offset < bit_count; ++offset) {
        const slmp::DeviceAddress bit_device = {device.code, device.number + offset};
        (void)plc.writeOneBit(bit_device, false);
    }
}

void clearBlockSilently(
    const slmp::DeviceAddress& word_device,
    uint16_t word_points,
    const slmp::DeviceAddress& bit_device,
    uint16_t bit_points
) {
    if (word_points > 0) {
        clearWordRangeSilently(word_device, word_points);
    }
    if (bit_points > 0) {
        clearPackedBitWordsSilently(bit_device, bit_points);
    }
}

void clearVerificationTargetSilently() {
    if (!verification.active) {
        return;
    }
    if (verification.kind == VerificationKind::WordWrite) {
        clearWordsSilently(verification.device, verification.points);
        return;
    }
    clearBitsSilently(verification.device, 1);
}

void printFuncheckApiDevices(const char* label, const slmp::DeviceAddress* devices, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        printDeviceAddress(devices[i]);
    }
    Serial.println();
}

void resetEnduranceSession() {
    endurance = EnduranceSession();
}

void resetReconnectSession() {
    reconnect = ReconnectSession();
}

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

void printFuncheckList() {
    Serial.println("funcheck modes:");
    Serial.println("  funcheck");
    Serial.println("    runs direct + api suites");
    Serial.println("  funcheck direct");
    Serial.println("    verifies console-level verifyw / verifyb flow on fixed devices");
    Serial.println("  funcheck api");
    Serial.println("    runs representative SlmpClient API checks and clears writes to 0");
    Serial.println("funcheck direct devices:");
    Serial.print("  verifyw: ");
    printDeviceAddress(kFuncheckWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckWordArrayDevice, 1);
    Serial.println();
    Serial.print("  verifyb: ");
    printDeviceAddress(kFuncheckOneBitDevice);
    Serial.println();
    Serial.println("funcheck api devices:");
    Serial.print("  row/wow: ");
    printDeviceAddress(kFuncheckOneWordDevice);
    Serial.println();
    Serial.print("  rw/ww: ");
    printDeviceAddress(kFuncheckWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckWordArrayDevice, 1);
    Serial.println();
    Serial.print("  rb/wb: ");
    printDeviceAddress(kFuncheckOneBitDevice);
    Serial.println();
    Serial.print("  rbits/wbits: ");
    printDeviceAddress(kFuncheckBitArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckBitArrayDevice, 3);
    Serial.println();
    Serial.print("  rod/wod: ");
    printDeviceAddress(kFuncheckOneDWordDevice);
    Serial.println();
    Serial.print("  rdw/wdw: ");
    printDeviceAddress(kFuncheckDWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckDWordArrayDevice, 3);
    Serial.println();
    printFuncheckApiDevices(
        "  wrand/rr words: ",
        kFuncheckRandomWordDevices,
        sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0])
    );
    printFuncheckApiDevices(
        "  wrand/rr dwords: ",
        kFuncheckRandomDWordDevices,
        sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0])
    );
    printFuncheckApiDevices(
        "  wrandb bits: ",
        kFuncheckRandomBitDevices,
        sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0])
    );
    Serial.print("  rblk/wblk words: ");
    printDeviceAddress(kFuncheckBlockWordDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckBlockWordDevice, 1);
    Serial.println();
    Serial.print("  rblk/wblk bits: ");
    printDeviceAddress(kFuncheckBlockBitDevice);
    Serial.println(" packed 1 word");
}

void printFuncheckSummary(const char* label, const FuncheckSummary& summary) {
    Serial.print("funcheck ");
    Serial.print(label);
    Serial.print(" summary: ok=");
    Serial.print(summary.ok);
    Serial.print(" fail=");
    Serial.println(summary.fail);
}

FuncheckResult runFuncheckDirectWords() {
    if (!connectPlc(false)) {
        Serial.println("funcheck direct failed: plc not connected");
        return FuncheckResult::Fail;
    }

    const uint16_t expected[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_values=", expected, 2);
    uint16_t before[2] = {};
    uint16_t readback[2] = {};
    if (plc.readWords(kFuncheckWordArrayDevice, 2, before, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck verifyw before read");
        return FuncheckResult::Fail;
    }
    if (plc.writeWords(kFuncheckWordArrayDevice, expected, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck verifyw write");
        return FuncheckResult::Fail;
    }
    if (plc.readWords(kFuncheckWordArrayDevice, 2, readback, 2) != slmp::Error::Ok) {
        clearWordsSilently(kFuncheckWordArrayDevice, 2);
        printLastPlcError("funcheck verifyw readback");
        return FuncheckResult::Fail;
    }

    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::WordWrite;
    verification.device = kFuncheckWordArrayDevice;
    verification.points = 2;
    verification.started_ms = millis();
    memcpy(verification.before_words, before, sizeof(before));
    memcpy(verification.written_words, expected, sizeof(expected));
    memcpy(verification.readback_words, readback, sizeof(readback));
    verification.readback_match = wordArraysEqual(expected, readback, 2);
    verification.judged = true;
    verification.pass = verification.readback_match;
    printVerificationSummary();
    const FuncheckResult result = verification.readback_match ? FuncheckResult::Ok : FuncheckResult::Fail;
    clearVerificationTargetSilently();
    resetVerificationRecord();
    return result;
}

FuncheckResult runFuncheckDirectBit() {
    if (!connectPlc(false)) {
        Serial.println("funcheck direct failed: plc not connected");
        return FuncheckResult::Fail;
    }

    bool before = false;
    bool readback = false;
    if (plc.readOneBit(kFuncheckOneBitDevice, before) != slmp::Error::Ok) {
        printLastPlcError("funcheck verifyb before read");
        return FuncheckResult::Fail;
    }
    if (plc.writeOneBit(kFuncheckOneBitDevice, true) != slmp::Error::Ok) {
        printLastPlcError("funcheck verifyb write");
        return FuncheckResult::Fail;
    }
    if (plc.readOneBit(kFuncheckOneBitDevice, readback) != slmp::Error::Ok) {
        clearBitsSilently(kFuncheckOneBitDevice, 1);
        printLastPlcError("funcheck verifyb readback");
        return FuncheckResult::Fail;
    }

    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::BitWrite;
    verification.device = kFuncheckOneBitDevice;
    verification.points = 1;
    verification.started_ms = millis();
    verification.before_bit = before;
    verification.written_bit = true;
    verification.readback_bit = readback;
    verification.readback_match = readback;
    verification.judged = true;
    verification.pass = verification.readback_match;
    printVerificationSummary();
    const FuncheckResult result = verification.readback_match ? FuncheckResult::Ok : FuncheckResult::Fail;
    clearVerificationTargetSilently();
    resetVerificationRecord();
    return result;
}

FuncheckSummary runFuncheckDirectSuite() {
    struct DirectCase {
        const char* name;
        FuncheckResult (*run)();
    };

    const DirectCase cases[] = {
        {"verifyw", runFuncheckDirectWords},
        {"verifyb", runFuncheckDirectBit},
    };

    FuncheckSummary summary = {};
    Serial.println("funcheck suite=direct");
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Serial.println("-------------------");
        Serial.print("funcheck direct step ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(sizeof(cases) / sizeof(cases[0]));
        Serial.print("funcheck direct: ");
        Serial.println(cases[i].name);
        const FuncheckResult result = cases[i].run();
        recordFuncheckResult(summary, result);
        Serial.print("funcheck result=");
        Serial.println(funcheckResultText(result));
    }
    printFuncheckSummary("direct", summary);
    return summary;
}

FuncheckResult runFuncheckApiTypeAndFrames() {
    slmp::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp::Error::Ok) {
        printLastPlcError("funcheck readTypeName");
        return FuncheckResult::Fail;
    }
    if (plc.lastRequestFrameLength() == 0 || plc.lastResponseFrameLength() == 0) {
        Serial.println("funcheck readTypeName failed: dump frames are empty");
        return FuncheckResult::Fail;
    }
    Serial.print("funcheck model=");
    Serial.println(type_name.model);
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiTargetHeader() {
    const slmp::TargetAddress current_target = plc.target();
    plc.setTarget(current_target);
    slmp::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp::Error::Ok) {
        printLastPlcError("funcheck target header");
        return FuncheckResult::Fail;
    }
    const uint8_t* frame = plc.lastRequestFrame();
    if (plc.lastRequestFrameLength() < 15 || frame == nullptr) {
        Serial.println("funcheck target header failed: last request too short");
        return FuncheckResult::Fail;
    }
    if (frame[6] != current_target.network || frame[7] != current_target.station ||
        readFrameLe16(frame + 8) != current_target.module_io || frame[10] != current_target.multidrop) {
        Serial.println("funcheck target header failed: request header mismatch");
        return FuncheckResult::Fail;
    }
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiMonitorAndTimeout() {
    const uint16_t original_monitor = plc.monitoringTimer();
    const uint32_t original_timeout = plc.timeoutMs();
    const uint16_t temporary_monitor = original_monitor == 16U ? 17U : 16U;
    const uint32_t temporary_timeout = original_timeout == 3210U ? 3211U : 3210U;

    plc.setMonitoringTimer(temporary_monitor);
    plc.setTimeoutMs(temporary_timeout);

    slmp::TypeNameInfo type_name = {};
    const slmp::Error error = plc.readTypeName(type_name);
    plc.setMonitoringTimer(original_monitor);
    plc.setTimeoutMs(original_timeout);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck monitor/timeout");
        return FuncheckResult::Fail;
    }
    const uint8_t* frame = plc.lastRequestFrame();
    if (plc.lastRequestFrameLength() < 15 || frame == nullptr) {
        Serial.println("funcheck monitor/timeout failed: last request too short");
        return FuncheckResult::Fail;
    }
    if (readFrameLe16(frame + 13) != temporary_monitor || plc.timeoutMs() != original_timeout) {
        Serial.println("funcheck monitor/timeout failed: state mismatch");
        return FuncheckResult::Fail;
    }
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiOneWord() {
    const uint16_t expected = nextFuncheckWordValue();
    Serial.print("funcheck randomized_value=");
    Serial.println(expected);
    if (plc.writeOneWord(kFuncheckOneWordDevice, expected) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneWord");
        return FuncheckResult::Fail;
    }
    uint16_t readback = 0;
    const slmp::Error error = plc.readOneWord(kFuncheckOneWordDevice, readback);
    clearWordsSilently(kFuncheckOneWordDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneWord");
        return FuncheckResult::Fail;
    }
    return readback == expected ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiWords() {
    const uint16_t expected[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_values=", expected, 2);
    if (plc.writeWords(kFuncheckWordArrayDevice, expected, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeWords");
        return FuncheckResult::Fail;
    }
    uint16_t readback[2] = {};
    const slmp::Error error = plc.readWords(kFuncheckWordArrayDevice, 2, readback, 2);
    clearWordsSilently(kFuncheckWordArrayDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readWords");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected, readback, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiOneBit() {
    if (plc.writeOneBit(kFuncheckOneBitDevice, true) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneBit");
        return FuncheckResult::Fail;
    }
    bool readback = false;
    const slmp::Error error = plc.readOneBit(kFuncheckOneBitDevice, readback);
    clearBitsSilently(kFuncheckOneBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneBit");
        return FuncheckResult::Fail;
    }
    return readback ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiBits() {
    const bool expected[] = {true, false, true, false};
    if (plc.writeBits(kFuncheckBitArrayDevice, expected, 4) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBits");
        return FuncheckResult::Fail;
    }
    bool readback[4] = {};
    const slmp::Error error = plc.readBits(kFuncheckBitArrayDevice, 4, readback, 4);
    clearBitsSilently(kFuncheckBitArrayDevice, 4);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBits");
        return FuncheckResult::Fail;
    }
    return bitArraysEqual(expected, readback, 4) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiOneDWord() {
    const uint32_t expected = nextFuncheckDWordValue();
    Serial.print("funcheck randomized_value=");
    Serial.println(static_cast<unsigned long>(expected));
    if (plc.writeOneDWord(kFuncheckOneDWordDevice, expected) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneDWord");
        return FuncheckResult::Fail;
    }
    uint32_t readback = 0;
    const slmp::Error error = plc.readOneDWord(kFuncheckOneDWordDevice, readback);
    clearDWordsSilently(kFuncheckOneDWordDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneDWord");
        return FuncheckResult::Fail;
    }
    return readback == expected ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiDWords() {
    const uint32_t expected[] = {nextFuncheckDWordValue(), nextFuncheckDWordValue()};
    printFuncheckDWordValues("funcheck randomized_values=", expected, 2);
    if (plc.writeDWords(kFuncheckDWordArrayDevice, expected, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeDWords");
        return FuncheckResult::Fail;
    }
    uint32_t readback[2] = {};
    const slmp::Error error = plc.readDWords(kFuncheckDWordArrayDevice, 2, readback, 2);
    clearDWordsSilently(kFuncheckDWordArrayDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readDWords");
        return FuncheckResult::Fail;
    }
    return dwordArraysEqual(expected, readback, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiRandomWords() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const uint32_t expected_dwords[] = {nextFuncheckDWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    printFuncheckDWordValues("funcheck randomized_dword_values=", expected_dwords, 1);
    if (plc.writeRandomWords(
            kFuncheckRandomWordDevices,
            expected_words,
            sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
            kFuncheckRandomDWordDevices,
            expected_dwords,
            sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0])) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeRandomWords");
        return FuncheckResult::Fail;
    }

    uint16_t readback_words[2] = {};
    uint32_t readback_dwords[1] = {};
    const slmp::Error error = plc.readRandom(
        kFuncheckRandomWordDevices,
        sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
        readback_words,
        2,
        kFuncheckRandomDWordDevices,
        sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0]),
        readback_dwords,
        1
    );
    clearRandomWordsSilently(
        kFuncheckRandomWordDevices,
        sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
        kFuncheckRandomDWordDevices,
        sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0])
    );
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandom");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_words, readback_words, 2) &&
                   dwordArraysEqual(expected_dwords, readback_dwords, 1)
               ? FuncheckResult::Ok
               : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiRandomBits() {
    const bool expected[] = {true, false};
    if (plc.writeRandomBits(
            kFuncheckRandomBitDevices,
            expected,
            sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0])) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeRandomBits");
        return FuncheckResult::Fail;
    }
    bool first = false;
    bool second = false;
    const slmp::Error first_error = plc.readOneBit(kFuncheckRandomBitDevices[0], first);
    const slmp::Error second_error = plc.readOneBit(kFuncheckRandomBitDevices[1], second);
    clearRandomBitsSilently(
        kFuncheckRandomBitDevices,
        sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0])
    );
    if (first_error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandomBits first");
        return FuncheckResult::Fail;
    }
    if (second_error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandomBits second");
        return FuncheckResult::Fail;
    }
    return (first == expected[0] && second == expected[1]) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiWordBlock() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    const slmp::DeviceBlockWrite word_blocks[] = {
        {kFuncheckBlockWordDevice, expected_words, 2},
    };
    if (plc.writeBlock(word_blocks, 1, nullptr, 0) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock words");
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_word_blocks[] = {
        {kFuncheckBlockWordDevice, 2},
    };
    uint16_t readback_words[2] = {};
    const slmp::Error error = plc.readBlock(read_word_blocks, 1, nullptr, 0, readback_words, 2, nullptr, 0);
    clearWordsSilently(kFuncheckBlockWordDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock words");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_words, readback_words, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiBitBlock() {
    const uint16_t expected_bits[] = {nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_packed_bits=", expected_bits, 1);
    const slmp::DeviceBlockWrite bit_blocks[] = {
        {kFuncheckBlockBitDevice, expected_bits, 1},
    };
    if (plc.writeBlock(nullptr, 0, bit_blocks, 1) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock bits");
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_bit_blocks[] = {
        {kFuncheckBlockBitDevice, 1},
    };
    uint16_t readback_bits[1] = {};
    const slmp::Error error = plc.readBlock(nullptr, 0, read_bit_blocks, 1, nullptr, 0, readback_bits, 1);
    clearPackedBitWordsSilently(kFuncheckBlockBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock bits");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_bits, readback_bits, 1) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiMixedBlock() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const uint16_t expected_bits[] = {nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    printFuncheckWordValues("funcheck randomized_packed_bits=", expected_bits, 1);
    const slmp::DeviceBlockWrite word_blocks[] = {
        {kFuncheckBlockWordDevice, expected_words, 2},
    };
    const slmp::DeviceBlockWrite bit_blocks[] = {
        {kFuncheckBlockBitDevice, expected_bits, 1},
    };
    if (plc.writeBlock(word_blocks, 1, bit_blocks, 1) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock mixed");
        clearBlockSilently(kFuncheckBlockWordDevice, 2, kFuncheckBlockBitDevice, 1);
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_word_blocks[] = {
        {kFuncheckBlockWordDevice, 2},
    };
    const slmp::DeviceBlockRead read_bit_blocks[] = {
        {kFuncheckBlockBitDevice, 1},
    };
    uint16_t readback_words[2] = {};
    uint16_t readback_bits[1] = {};
    const slmp::Error error =
        plc.readBlock(read_word_blocks, 1, read_bit_blocks, 1, readback_words, 2, readback_bits, 1);
    clearBlockSilently(kFuncheckBlockWordDevice, 2, kFuncheckBlockBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock mixed");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_words, readback_words, 2) &&
                   wordArraysEqual(expected_bits, readback_bits, 1)
               ? FuncheckResult::Ok
               : FuncheckResult::Fail;
}

FuncheckSummary runFuncheckApiSuite() {
    struct ApiCase {
        const char* name;
        FuncheckResult (*run)();
    };

    const ApiCase cases[] = {
        {"readTypeName + dump", runFuncheckApiTypeAndFrames},
        {"target header", runFuncheckApiTargetHeader},
        {"monitor + timeout", runFuncheckApiMonitorAndTimeout},
        {"readOneWord/writeOneWord", runFuncheckApiOneWord},
        {"readWords/writeWords", runFuncheckApiWords},
        {"readOneBit/writeOneBit", runFuncheckApiOneBit},
        {"readBits/writeBits", runFuncheckApiBits},
        {"readOneDWord/writeOneDWord", runFuncheckApiOneDWord},
        {"readDWords/writeDWords", runFuncheckApiDWords},
        {"readRandom/writeRandomWords", runFuncheckApiRandomWords},
        {"writeRandomBits", runFuncheckApiRandomBits},
        {"readBlock/writeBlock words", runFuncheckApiWordBlock},
        {"readBlock/writeBlock bits", runFuncheckApiBitBlock},
        {"readBlock/writeBlock mixed", runFuncheckApiMixedBlock},
    };

    FuncheckSummary summary = {};
    Serial.println("funcheck suite=api");
    if (!connectPlc(false)) {
        Serial.println("funcheck api failed: plc not connected");
        summary.fail = 1;
        return summary;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Serial.println("-------------------");
        Serial.print("funcheck api step ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(sizeof(cases) / sizeof(cases[0]));
        Serial.print("funcheck api: ");
        Serial.println(cases[i].name);
        const FuncheckResult result = cases[i].run();
        recordFuncheckResult(summary, result);
        Serial.print("funcheck result=");
        Serial.println(funcheckResultText(result));
    }

    printFuncheckSummary("api", summary);
    return summary;
}

void runFuncheckAll() {
    stopEndurance(false, false);
    stopReconnect(false);
    resetVerificationRecord();
    const FuncheckSummary direct_summary = runFuncheckDirectSuite();
    const FuncheckSummary api_summary = runFuncheckApiSuite();
    FuncheckSummary total = {};
    total.ok = direct_summary.ok + api_summary.ok;
    total.fail = direct_summary.fail + api_summary.fail;
    printFuncheckSummary("all", total);
}

void runFuncheckDirectOnly() {
    stopEndurance(false, false);
    stopReconnect(false);
    resetVerificationRecord();
    (void)runFuncheckDirectSuite();
}

void runFuncheckApiOnly() {
    stopEndurance(false, false);
    stopReconnect(false);
    resetVerificationRecord();
    (void)runFuncheckApiSuite();
}

void funcheckCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        runFuncheckAll();
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "ALL") == 0 || strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        runFuncheckAll();
        return;
    }
    if (strcmp(tokens[1], "DIRECT") == 0 || strcmp(tokens[1], "DEVICES") == 0) {
        runFuncheckDirectOnly();
        return;
    }
    if (strcmp(tokens[1], "API") == 0 || strcmp(tokens[1], "FUNCTIONS") == 0) {
        runFuncheckApiOnly();
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printFuncheckList();
        return;
    }

    Serial.println("funcheck usage: funcheck [all|direct|api|list]");
}

void clearEnduranceTargetsSilently() {
    clearWordsSilently(kEnduranceOneWordDevice, 1);
    clearWordsSilently(kEnduranceWordArrayDevice, 2);
    clearBitsSilently(kEnduranceOneBitDevice, 1);
    clearBitsSilently(kEnduranceBitArrayDevice, 4);
    clearDWordsSilently(kEnduranceOneDWordDevice, 1);
    clearDWordsSilently(kEnduranceDWordArrayDevice, 2);
    clearRandomWordsSilently(
        kEnduranceRandomWordDevices,
        sizeof(kEnduranceRandomWordDevices) / sizeof(kEnduranceRandomWordDevices[0]),
        kEnduranceRandomDWordDevices,
        sizeof(kEnduranceRandomDWordDevices) / sizeof(kEnduranceRandomDWordDevices[0])
    );
    clearRandomBitsSilently(
        kEnduranceRandomBitDevices,
        sizeof(kEnduranceRandomBitDevices) / sizeof(kEnduranceRandomBitDevices[0])
    );
    clearBlockSilently(kEnduranceBlockWordDevice, 2, kEnduranceBlockBitDevice, 1);
}

void printEnduranceList() {
    Serial.println("endurance modes:");
    Serial.println("  endurance");
    Serial.println("    starts continuous read/write durability test");
    Serial.println("  endurance 1000");
    Serial.println("    runs 1000 cycles, then stops");
    Serial.println("  endurance status");
    Serial.println("  endurance stop");
}

void printEnduranceSummary(const char* label) {
    Serial.print(label);
    Serial.print(" attempts=");
    Serial.print(endurance.attempts);
    Serial.print(" ok=");
    Serial.print(endurance.ok);
    Serial.print(" fail=");
    Serial.print(endurance.fail);
    Serial.print(" elapsed_ms=");
    Serial.print(endurance.started_ms == 0 ? 0U : (millis() - endurance.started_ms));
    Serial.print(" last_cycle_ms=");
    Serial.print(endurance.last_cycle_ms);
    Serial.print(" avg_ms=");
    if (endurance.attempts == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(endurance.total_cycle_ms / endurance.attempts));
    }
    Serial.print(" min_ms=");
    Serial.print(endurance.attempts == 0 ? 0U : endurance.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.println(endurance.max_cycle_ms);
}

void printEnduranceStatus() {
    Serial.print("endurance_active=");
    Serial.println(endurance.active ? "yes" : "no");
    Serial.print("endurance_cycle_limit=");
    Serial.println(endurance.cycle_limit);
    Serial.print("endurance_last_step=");
    Serial.println(endurance.last_step[0] == '\0' ? "<none>" : endurance.last_step);
    Serial.print("endurance_last_issue=");
    Serial.println(endurance.last_issue[0] == '\0' ? "<none>" : endurance.last_issue);
    Serial.print("endurance_last_error=");
    Serial.println(slmp::errorString(endurance.last_error));
    Serial.print("endurance_last_end_code=0x");
    Serial.println(endurance.last_end_code, HEX);
    printEnduranceSummary("endurance status:");
}

void stopEndurance(bool print_summary, bool /*failed*/) {
    if (!endurance.active) {
        return;
    }
    endurance.active = false;
    clearEnduranceTargetsSilently();
    if (print_summary) {
        printEnduranceSummary("endurance summary:");
    }
}

bool failEnduranceCycle(const char* step, const char* issue, const char* plc_label, bool use_plc_error) {
    copyText(endurance.last_step, sizeof(endurance.last_step), step);
    copyText(endurance.last_issue, sizeof(endurance.last_issue), issue);
    endurance.last_error = use_plc_error ? plc.lastError() : slmp::Error::Ok;
    endurance.last_end_code = use_plc_error ? plc.lastEndCode() : 0;
    ++endurance.attempts;
    ++endurance.fail;
    Serial.print("endurance failed at ");
    Serial.print(step);
    Serial.print(": ");
    Serial.println(issue);
    if (use_plc_error && plc_label != nullptr) {
        printLastPlcError(plc_label);
    }
    stopEndurance(true, true);
    return false;
}

bool recordEnduranceCycleTiming(uint32_t started_ms) {
    endurance.last_cycle_ms = millis() - started_ms;
    if (endurance.attempts == 0 || endurance.last_cycle_ms < endurance.min_cycle_ms) {
        endurance.min_cycle_ms = endurance.last_cycle_ms;
    }
    if (endurance.last_cycle_ms > endurance.max_cycle_ms) {
        endurance.max_cycle_ms = endurance.last_cycle_ms;
    }
    endurance.total_cycle_ms += endurance.last_cycle_ms;
    if (millis() - endurance.last_report_ms >= kEnduranceReportIntervalMs) {
        printEnduranceSummary("endurance progress:");
        endurance.last_report_ms = millis();
    }
    return true;
}

bool runEnduranceCycle() {
    if (!connectPlc(false)) {
        return failEnduranceCycle("connect", "plc not connected", nullptr, false);
    }

    const uint32_t started_ms = millis();
    copyText(endurance.last_issue, sizeof(endurance.last_issue), "");
    endurance.last_error = slmp::Error::Ok;
    endurance.last_end_code = 0;

    const uint16_t one_word_value = nextFuncheckWordValue();
    uint16_t one_word_readback = 0;
    if (plc.writeOneWord(kEnduranceOneWordDevice, one_word_value) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneWord", "plc write failed", "endurance writeOneWord", true);
    }
    if (plc.readOneWord(kEnduranceOneWordDevice, one_word_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneWord", "plc read failed", "endurance readOneWord", true);
    }
    if (one_word_readback != one_word_value) {
        return failEnduranceCycle("readOneWord/writeOneWord", "readback mismatch", nullptr, false);
    }
    clearWordsSilently(kEnduranceOneWordDevice, 1);

    const uint16_t word_values[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    uint16_t word_readback[2] = {};
    if (plc.writeWords(kEnduranceWordArrayDevice, word_values, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("writeWords", "plc write failed", "endurance writeWords", true);
    }
    if (plc.readWords(kEnduranceWordArrayDevice, 2, word_readback, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("readWords", "plc read failed", "endurance readWords", true);
    }
    if (!wordArraysEqual(word_values, word_readback, 2)) {
        return failEnduranceCycle("readWords/writeWords", "readback mismatch", nullptr, false);
    }
    clearWordsSilently(kEnduranceWordArrayDevice, 2);

    bool one_bit_readback = false;
    if (plc.writeOneBit(kEnduranceOneBitDevice, true) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneBit", "plc write failed", "endurance writeOneBit", true);
    }
    if (plc.readOneBit(kEnduranceOneBitDevice, one_bit_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneBit", "plc read failed", "endurance readOneBit", true);
    }
    if (!one_bit_readback) {
        return failEnduranceCycle("readOneBit/writeOneBit", "readback mismatch", nullptr, false);
    }
    clearBitsSilently(kEnduranceOneBitDevice, 1);

    const bool bit_values[] = {true, false, true, true};
    bool bit_readback[4] = {};
    if (plc.writeBits(kEnduranceBitArrayDevice, bit_values, 4) != slmp::Error::Ok) {
        return failEnduranceCycle("writeBits", "plc write failed", "endurance writeBits", true);
    }
    if (plc.readBits(kEnduranceBitArrayDevice, 4, bit_readback, 4) != slmp::Error::Ok) {
        return failEnduranceCycle("readBits", "plc read failed", "endurance readBits", true);
    }
    if (!bitArraysEqual(bit_values, bit_readback, 4)) {
        return failEnduranceCycle("readBits/writeBits", "readback mismatch", nullptr, false);
    }
    clearBitsSilently(kEnduranceBitArrayDevice, 4);

    const uint32_t one_dword_value = nextFuncheckDWordValue();
    uint32_t one_dword_readback = 0;
    if (plc.writeOneDWord(kEnduranceOneDWordDevice, one_dword_value) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneDWord", "plc write failed", "endurance writeOneDWord", true);
    }
    if (plc.readOneDWord(kEnduranceOneDWordDevice, one_dword_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneDWord", "plc read failed", "endurance readOneDWord", true);
    }
    if (one_dword_readback != one_dword_value) {
        return failEnduranceCycle("readOneDWord/writeOneDWord", "readback mismatch", nullptr, false);
    }
    clearDWordsSilently(kEnduranceOneDWordDevice, 1);
    ++endurance.attempts;
    ++endurance.ok;
    copyText(endurance.last_step, sizeof(endurance.last_step), "cycle_ok");
    recordEnduranceCycleTiming(started_ms);
    return true;
}

void startEndurance(uint32_t cycle_limit) {
    stopReconnect(false);
    resetVerificationRecord();
    clearEnduranceTargetsSilently();
    resetEnduranceSession();
    endurance.active = true;
    endurance.started_ms = millis();
    endurance.next_cycle_due_ms = endurance.started_ms;
    endurance.last_report_ms = endurance.started_ms;
    endurance.cycle_limit = cycle_limit;
    copyText(endurance.last_step, sizeof(endurance.last_step), "starting");
    copyText(endurance.last_issue, sizeof(endurance.last_issue), "none");
    Serial.println("endurance=on");
    Serial.print("endurance_cycle_limit=");
    Serial.println(cycle_limit);
}

void enduranceCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        startEndurance(0);
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STATUS") == 0) {
        printEnduranceStatus();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
        Serial.println("endurance=off");
        stopEndurance(true, false);
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printEnduranceList();
        return;
    }
    if (strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        if (token_count >= 3) {
            unsigned long parsed_limit = 0;
            if (!parseUnsignedValue(tokens[2], parsed_limit, 10)) {
                Serial.println("endurance usage: endurance [start [cycles]|status|stop|list]");
                return;
            }
            startEndurance(static_cast<uint32_t>(parsed_limit));
            return;
        }
        startEndurance(0);
        return;
    }

    unsigned long cycle_limit = 0;
    if (parseUnsignedValue(tokens[1], cycle_limit, 10)) {
        startEndurance(static_cast<uint32_t>(cycle_limit));
        return;
    }

    Serial.println("endurance usage: endurance [start [cycles]|status|stop|list]");
}

void pollEnduranceTest() {
    if (!endurance.active) {
        return;
    }
    if (millis() < endurance.next_cycle_due_ms) {
        return;
    }
    if (!runEnduranceCycle()) {
        printPrompt();
        return;
    }
    if (endurance.cycle_limit > 0 && endurance.ok >= endurance.cycle_limit) {
        Serial.println("endurance limit reached");
        stopEndurance(true, false);
        printPrompt();
        return;
    }
    endurance.next_cycle_due_ms = millis() + kEnduranceCycleGapMs;
}

void printReconnectList() {
    Serial.println("reconnect modes:");
    Serial.println("  reconnect");
    Serial.println("    starts endless reconnect verification");
    Serial.println("  reconnect 1000");
    Serial.println("    runs 1000 attempts, then stops");
    Serial.println("  reconnect status");
    Serial.println("  reconnect stop");
    Serial.print("  probe device: ");
    printDeviceAddress(kEnduranceOneWordDevice);
    Serial.println();
}

uint32_t reconnectRetryGapMs() {
    uint32_t retry_gap_ms = kReconnectCycleGapMs;
    uint32_t remaining_failures = reconnect.consecutive_failures;
    while (remaining_failures > 1U && retry_gap_ms < kReconnectMaxCycleGapMs) {
        const uint32_t doubled_gap_ms = retry_gap_ms * 2U;
        retry_gap_ms = doubled_gap_ms > kReconnectMaxCycleGapMs ? kReconnectMaxCycleGapMs : doubled_gap_ms;
        --remaining_failures;
    }
    return retry_gap_ms;
}

void printReconnectSummary(const char* label) {
    Serial.print(label);
    Serial.print(" attempts=");
    Serial.print(reconnect.attempts);
    Serial.print(" ok=");
    Serial.print(reconnect.ok);
    Serial.print(" fail=");
    Serial.print(reconnect.fail);
    Serial.print(" recoveries=");
    Serial.print(reconnect.recoveries);
    Serial.print(" consecutive_failures=");
    Serial.print(reconnect.consecutive_failures);
    Serial.print(" max_consecutive_failures=");
    Serial.print(reconnect.max_consecutive_failures);
    Serial.print(" elapsed_ms=");
    Serial.print(reconnect.started_ms == 0 ? 0U : (millis() - reconnect.started_ms));
    Serial.print(" last_cycle_ms=");
    Serial.print(reconnect.last_cycle_ms);
    Serial.print(" avg_ms=");
    if (reconnect.attempts == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(reconnect.total_cycle_ms / reconnect.attempts));
    }
    Serial.print(" min_ms=");
    Serial.print(reconnect.attempts == 0 ? 0U : reconnect.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.print(reconnect.max_cycle_ms);
    Serial.print(" retry_gap_ms=");
    Serial.println(reconnectRetryGapMs());
}

void printReconnectStatus() {
    Serial.print("reconnect_active=");
    Serial.println(reconnect.active ? "yes" : "no");
    Serial.print("reconnect_cycle_limit=");
    Serial.println(reconnect.cycle_limit);
    Serial.print("reconnect_last_step=");
    Serial.println(reconnect.last_step[0] == '\0' ? "<none>" : reconnect.last_step);
    Serial.print("reconnect_last_issue=");
    Serial.println(reconnect.last_issue[0] == '\0' ? "<none>" : reconnect.last_issue);
    Serial.print("reconnect_last_error=");
    Serial.println(slmp::errorString(reconnect.last_error));
    Serial.print("reconnect_last_end_code=0x");
    Serial.println(reconnect.last_end_code, HEX);
    printReconnectSummary("reconnect status:");
}

void stopReconnect(bool print_summary) {
    if (!reconnect.active) {
        return;
    }
    reconnect.active = false;
    if (print_summary) {
        printReconnectSummary("reconnect summary:");
    }
}

void finishReconnectCycleTiming(uint32_t started_ms) {
    reconnect.last_cycle_ms = millis() - started_ms;
    if (reconnect.attempts == 0 || reconnect.last_cycle_ms < reconnect.min_cycle_ms) {
        reconnect.min_cycle_ms = reconnect.last_cycle_ms;
    }
    if (reconnect.last_cycle_ms > reconnect.max_cycle_ms) {
        reconnect.max_cycle_ms = reconnect.last_cycle_ms;
    }
    reconnect.total_cycle_ms += reconnect.last_cycle_ms;
    if (millis() - reconnect.last_report_ms >= kReconnectReportIntervalMs) {
        printReconnectSummary("reconnect progress:");
        reconnect.last_report_ms = millis();
    }
}

bool runReconnectCycle() {
    const uint32_t started_ms = millis();
    const uint32_t failure_streak_before = reconnect.consecutive_failures;
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), "none");
    reconnect.last_error = slmp::Error::Ok;
    reconnect.last_end_code = 0;

    uint16_t probe_value = 0;
    bool ok = false;
    const char* step = "connect";
    const char* issue = "plc not connected";
    bool use_plc_error = false;

    if (connectPlc(false)) {
        step = "readOneWord";
        issue = "plc read failed";
        use_plc_error = true;
        if (plc.readOneWord(kEnduranceOneWordDevice, probe_value) == slmp::Error::Ok) {
            ok = true;
        }
    }

    ++reconnect.attempts;
    copyText(reconnect.last_step, sizeof(reconnect.last_step), step);

    if (ok) {
        ++reconnect.ok;
        reconnect.consecutive_failures = 0;
        if (failure_streak_before > 0U) {
            ++reconnect.recoveries;
            Serial.print("reconnect recovered after ");
            Serial.print(failure_streak_before);
            Serial.print(" failed attempts; probe=");
            printDeviceAddress(kEnduranceOneWordDevice);
            Serial.print(" value=");
            Serial.println(probe_value);
        }
        finishReconnectCycleTiming(started_ms);
        return true;
    }

    ++reconnect.fail;
    ++reconnect.consecutive_failures;
    if (reconnect.consecutive_failures > reconnect.max_consecutive_failures) {
        reconnect.max_consecutive_failures = reconnect.consecutive_failures;
    }
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), issue);
    reconnect.last_error = use_plc_error ? plc.lastError() : slmp::Error::Ok;
    reconnect.last_end_code = use_plc_error ? plc.lastEndCode() : 0;
    plc.close();
    if (reconnect.consecutive_failures == 1U) {
        Serial.print("reconnect lost at ");
        Serial.print(step);
        Serial.print(": ");
        Serial.println(issue);
        if (use_plc_error) {
            Serial.print("reconnect last_error=");
            Serial.print(slmp::errorString(reconnect.last_error));
            Serial.print(" end=0x");
            Serial.println(reconnect.last_end_code, HEX);
        }
    }
    finishReconnectCycleTiming(started_ms);
    return true;
}

void startReconnect(uint32_t cycle_limit) {
    stopEndurance(false, false);
    resetVerificationRecord();
    resetReconnectSession();
    reconnect.active = true;
    reconnect.started_ms = millis();
    reconnect.next_cycle_due_ms = reconnect.started_ms;
    reconnect.last_report_ms = reconnect.started_ms;
    reconnect.cycle_limit = cycle_limit;
    copyText(reconnect.last_step, sizeof(reconnect.last_step), "starting");
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), "none");
    Serial.println("reconnect=on");
    Serial.print("reconnect_cycle_limit=");
    Serial.println(cycle_limit);
}

void reconnectCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        startReconnect(0);
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STATUS") == 0) {
        printReconnectStatus();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
        Serial.println("reconnect=off");
        stopReconnect(true);
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printReconnectList();
        return;
    }
    if (strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        if (token_count >= 3) {
            unsigned long parsed_limit = 0;
            if (!parseUnsignedValue(tokens[2], parsed_limit, 10)) {
                Serial.println("reconnect usage: reconnect [start [cycles]|status|stop|list]");
                return;
            }
            startReconnect(static_cast<uint32_t>(parsed_limit));
            return;
        }
        startReconnect(0);
        return;
    }

    unsigned long cycle_limit = 0;
    if (parseUnsignedValue(tokens[1], cycle_limit, 10)) {
        startReconnect(static_cast<uint32_t>(cycle_limit));
        return;
    }

    Serial.println("reconnect usage: reconnect [start [cycles]|status|stop|list]");
}

void pollReconnectTest() {
    if (!reconnect.active) {
        return;
    }
    if (millis() < reconnect.next_cycle_due_ms) {
        return;
    }
    (void)runReconnectCycle();
    if (reconnect.cycle_limit > 0 && reconnect.attempts >= reconnect.cycle_limit) {
        Serial.println("reconnect limit reached");
        stopReconnect(true);
        printPrompt();
        return;
    }
    reconnect.next_cycle_due_ms = millis() + reconnectRetryGapMs();
}

void fillTxlimitWords(uint16_t* values, size_t count, uint16_t seed) {
    for (size_t i = 0; i < count; ++i) {
        values[i] = static_cast<uint16_t>(seed + i);
    }
}

void printTxlimitSummary() {
    Serial.print("tx_buffer_size=");
    Serial.println(kTxBufferSize);
    Serial.print("request_header_size=");
    Serial.println(kSlmpRequestHeaderSize);
    Serial.print("max_payload_fit=");
    Serial.println(kTxPayloadBudget);
    Serial.print("writeWords max_count=");
    Serial.print(kTxLimitWriteWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + (kTxLimitWriteWordsMaxCount * 2U));
    Serial.print("writeDWords max_count=");
    Serial.print(kTxLimitWriteDWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + (kTxLimitWriteDWordsMaxCount * 4U));
    Serial.print("writeBits max_points=");
    Serial.print(kTxLimitWriteBitsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + ((kTxLimitWriteBitsMaxCount + 1U) / 2U));
    Serial.print("writeRandomWords max_word_devices=");
    Serial.print(kTxLimitWriteRandomWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 2U + (kTxLimitWriteRandomWordsMaxCount * 8U));
    Serial.print("writeRandomBits max_devices=");
    Serial.print(kTxLimitWriteRandomBitsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 1U + (kTxLimitWriteRandomBitsMaxCount * 8U));
    Serial.print("writeBlock words-only max_points=");
    Serial.print(kTxLimitWriteBlockWordMaxPoints);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 10U + (kTxLimitWriteBlockWordMaxPoints * 2U));
    Serial.println("note: library uses the same tx_buffer for payload and full request frame");
}

bool runTxlimitProbeWriteWords() {
    fillTxlimitWords(txlimit_word_values, kTxLimitWriteWordsMaxCount, 1000U);
    if (plc.writeWords(kTxLimitWordDevice, txlimit_word_values, kTxLimitWriteWordsMaxCount) != slmp::Error::Ok) {
        printLastPlcError("txlimit writeWords exact");
        return false;
    }
    if (plc.readWords(
            kTxLimitWordDevice,
            static_cast<uint16_t>(kTxLimitWriteWordsMaxCount),
            txlimit_word_readback,
            kTxLimitWriteWordsMaxCount) != slmp::Error::Ok) {
        printLastPlcError("txlimit readWords exact");
        return false;
    }
    if (!wordArraysEqual(txlimit_word_values, txlimit_word_readback, static_cast<uint16_t>(kTxLimitWriteWordsMaxCount))) {
        Serial.println("txlimit writeWords exact failed: readback mismatch");
        return false;
    }
    Serial.print("txlimit writeWords exact_fit=ok count=");
    Serial.print(kTxLimitWriteWordsMaxCount);
    Serial.print(" request_bytes=");
    Serial.println(plc.lastRequestFrameLength());
    clearWordRangeSilently(kTxLimitWordDevice, kTxLimitWriteWordsMaxCount);

    fillTxlimitWords(txlimit_word_values, kTxLimitWriteWordsMaxCount + 1U, 2000U);
    const slmp::Error error = plc.writeWords(kTxLimitWordDevice, txlimit_word_values, kTxLimitWriteWordsMaxCount + 1U);
    Serial.print("txlimit writeWords one_over=");
    Serial.println(slmp::errorString(error));
    return error == slmp::Error::BufferTooSmall;
}

bool runTxlimitProbeWordBlock() {
    fillTxlimitWords(txlimit_block_values, kTxLimitWriteBlockWordMaxPoints, 3000U);
    const slmp::DeviceBlockWrite exact_block = {
        kTxLimitBlockWordDevice,
        txlimit_block_values,
        static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints)
    };
    const slmp::DeviceBlockRead exact_read_block = {
        kTxLimitBlockWordDevice,
        static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints)
    };
    if (plc.writeBlock(&exact_block, 1, nullptr, 0) != slmp::Error::Ok) {
        printLastPlcError("txlimit writeBlock exact");
        return false;
    }
    if (plc.readBlock(
            &exact_read_block,
            1,
            nullptr,
            0,
            txlimit_block_readback,
            kTxLimitWriteBlockWordMaxPoints,
            nullptr,
            0) != slmp::Error::Ok) {
        printLastPlcError("txlimit readBlock exact");
        return false;
    }
    if (!wordArraysEqual(txlimit_block_values, txlimit_block_readback, static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints))) {
        Serial.println("txlimit writeBlock exact failed: readback mismatch");
        return false;
    }
    Serial.print("txlimit writeBlock words exact_fit=ok points=");
    Serial.print(kTxLimitWriteBlockWordMaxPoints);
    Serial.print(" request_bytes=");
    Serial.println(plc.lastRequestFrameLength());
    clearWordRangeSilently(kTxLimitBlockWordDevice, kTxLimitWriteBlockWordMaxPoints);

    fillTxlimitWords(txlimit_block_values, kTxLimitWriteBlockWordMaxPoints + 1U, 4000U);
    const slmp::DeviceBlockWrite over_block = {
        kTxLimitBlockWordDevice,
        txlimit_block_values,
        static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints + 1U)
    };
    const slmp::Error error = plc.writeBlock(&over_block, 1, nullptr, 0);
    Serial.print("txlimit writeBlock words one_over=");
    Serial.println(slmp::errorString(error));
    return error == slmp::Error::BufferTooSmall;
}

void runTxlimitProbe() {
    stopEndurance(false, false);
    stopReconnect(false);
    if (!connectPlc(false)) {
        Serial.println("txlimit probe failed: plc not connected");
        return;
    }
    printTxlimitSummary();
    const bool write_words_ok = runTxlimitProbeWriteWords();
    const bool block_ok = runTxlimitProbeWordBlock();
    Serial.print("txlimit probe summary: writeWords=");
    Serial.print(write_words_ok ? "ok" : "fail");
    Serial.print(" writeBlockWords=");
    Serial.println(block_ok ? "ok" : "fail");
}

void txlimitCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        printTxlimitSummary();
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "CALC") == 0 || strcmp(tokens[1], "INFO") == 0 || strcmp(tokens[1], "STATUS") == 0) {
        printTxlimitSummary();
        return;
    }
    if (strcmp(tokens[1], "PROBE") == 0 || strcmp(tokens[1], "TEST") == 0) {
        runTxlimitProbe();
        return;
    }

    Serial.println("txlimit usage: txlimit [calc|probe]");
}

void printHelp() {
    Serial.println("commands:");
    Serial.println("  help");
    Serial.println("  status");
    Serial.println("  connect | close | reinit | type | dump");
    Serial.println("  target [network station module_io multidrop]");
    Serial.println("  monitor [value]");
    Serial.println("  timeout <ms>");
    Serial.println("  funcheck [all|direct|api|list]");
    Serial.println("  endurance [start [cycles]|status|stop|list]");
    Serial.println("  reconnect [start [cycles]|status|stop|list]");
    Serial.println("  txlimit [calc|probe]");
    Serial.println("  bench [row|wow|pair|rw|ww|block] [cycles]");
    Serial.println("  bench list");
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
    Serial.println("examples:");
    Serial.println("  target 0 255 0x03FF 0");
    Serial.println("  row D100");
    Serial.println("  rdw D200 2");
    Serial.println("  rbits M100 4");
    Serial.println("  rr 2 D100 D101 1 D200");
    Serial.println("  wrand 1 D120 123 1 D200 0x12345678");
    Serial.println("  rblk 1 D300 2 1 M200 1");
    Serial.println("  wblk 1 D300 2 10 20 1 M200 1 0x0005");
    Serial.println("  funcheck");
    Serial.println("  endurance 1000");
    Serial.println("  reconnect");
    Serial.println("  txlimit probe");
    Serial.println("  bench");
    Serial.println("  bench row 1000");
    Serial.println("  bench block 300");
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

void handleCommand(char* line) {
    char* tokens[kMaxTokens] = {};
    const int token_count = splitTokens(line, tokens, static_cast<int>(kMaxTokens));
    if (token_count == 0) {
        printPrompt();
        return;
    }

    uppercaseInPlace(tokens[0]);

    if (strcmp(tokens[0], "HELP") == 0 || strcmp(tokens[0], "?") == 0) {
        printHelp();
    } else if (strcmp(tokens[0], "STATUS") == 0) {
        printStatus();
    } else if (strcmp(tokens[0], "CONNECT") == 0) {
        (void)connectPlc(true);
    } else if (strcmp(tokens[0], "CLOSE") == 0) {
        closePlc();
    } else if (strcmp(tokens[0], "REINIT") == 0) {
        reinitializeEthernet();
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
    } else if (strcmp(tokens[0], "FUNCHECK") == 0) {
        funcheckCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "ENDURANCE") == 0 || strcmp(tokens[0], "SOAK") == 0) {
        enduranceCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "RECONNECT") == 0 || strcmp(tokens[0], "RETRY") == 0) {
        reconnectCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "TXLIMIT") == 0 || strcmp(tokens[0], "TXBUF") == 0) {
        txlimitCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "BENCH") == 0 || strcmp(tokens[0], "PERF") == 0) {
        stopEndurance(false, false);
        stopReconnect(false);
        benchCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "DUMP") == 0) {
        printLastFrames();
    } else {
        Serial.print("unknown command: ");
        Serial.println(tokens[0]);
        printHelp();
    }

    printPrompt();
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
    while (!Serial) {
        delay(10);
    }

    Serial.println("SLMP W6300-EVB-Pico2 serial debug console");
    plc.setTimeoutMs(2000);

    (void)bringUpEthernet();
    printHelp();
    runStartupDemo();
    printPrompt();
}

void loop() {
    pollEnduranceTest();
    pollReconnectTest();
    while (Serial.available() > 0) {
        const int raw = Serial.read();
        if (raw < 0) {
            break;
        }

        const char ch = static_cast<char>(raw);
        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            serial_line[serial_line_length] = '\0';
            handleCommand(serial_line);
            serial_line_length = 0;
            serial_line[0] = '\0';
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

        serial_line[serial_line_length++] = ch;
    }
}

}  // namespace w6300_evb_pico2_serial_console
