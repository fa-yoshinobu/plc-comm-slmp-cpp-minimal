#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <EthernetCompat.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <slmp4e_arduino_transport.h>

namespace w5500_evb_pico2_serial_console {

byte kMacAddress[6] = {0x02, 0x4E, 0x53, 0x4C, 0x4D, 0x52};
const IPAddress kLocalIp(192, 168, 250, 52);
const IPAddress kDns(192, 168, 250, 1);
const IPAddress kGateway(192, 168, 250, 1);
const IPAddress kSubnet(255, 255, 255, 0);
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

constexpr uint8_t kSpiMisoPin = 16;
constexpr uint8_t kEthernetCsPin = 17;
constexpr uint8_t kSpiSckPin = 18;
constexpr uint8_t kSpiMosiPin = 19;
constexpr uint8_t kEthernetIntPin = 21;
constexpr uint32_t kEthernetSpiHz = 30000000;
constexpr int kLwipPollingMs = 3;

constexpr size_t kSerialLineCapacity = 128;
constexpr size_t kMaxTokens = 16;
constexpr size_t kMaxWordPoints = 8;
constexpr size_t kVerificationNoteCapacity = 80;

struct DeviceSpec {
    const char* name;
    slmp4e::DeviceCode code;
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
    slmp4e::DeviceAddress device = {slmp4e::DeviceCode::D, 0};
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

const DeviceSpec kDeviceSpecs[] = {
    {"SM", slmp4e::DeviceCode::SM, false},
    {"SD", slmp4e::DeviceCode::SD, false},
    {"X", slmp4e::DeviceCode::X, true},
    {"Y", slmp4e::DeviceCode::Y, true},
    {"M", slmp4e::DeviceCode::M, false},
    {"L", slmp4e::DeviceCode::L, false},
    {"F", slmp4e::DeviceCode::F, false},
    {"V", slmp4e::DeviceCode::V, false},
    {"B", slmp4e::DeviceCode::B, true},
    {"D", slmp4e::DeviceCode::D, false},
    {"W", slmp4e::DeviceCode::W, true},
    {"TS", slmp4e::DeviceCode::TS, false},
    {"TC", slmp4e::DeviceCode::TC, false},
    {"TN", slmp4e::DeviceCode::TN, false},
    {"LTS", slmp4e::DeviceCode::LTS, false},
    {"LTC", slmp4e::DeviceCode::LTC, false},
    {"LTN", slmp4e::DeviceCode::LTN, false},
    {"STS", slmp4e::DeviceCode::STS, false},
    {"STC", slmp4e::DeviceCode::STC, false},
    {"STN", slmp4e::DeviceCode::STN, false},
    {"LSTS", slmp4e::DeviceCode::LSTS, false},
    {"LSTC", slmp4e::DeviceCode::LSTC, false},
    {"LSTN", slmp4e::DeviceCode::LSTN, false},
    {"CS", slmp4e::DeviceCode::CS, false},
    {"CC", slmp4e::DeviceCode::CC, false},
    {"CN", slmp4e::DeviceCode::CN, false},
    {"LCS", slmp4e::DeviceCode::LCS, false},
    {"LCC", slmp4e::DeviceCode::LCC, false},
    {"LCN", slmp4e::DeviceCode::LCN, false},
    {"SB", slmp4e::DeviceCode::SB, true},
    {"SW", slmp4e::DeviceCode::SW, true},
    {"S", slmp4e::DeviceCode::S, false},
    {"DX", slmp4e::DeviceCode::DX, true},
    {"DY", slmp4e::DeviceCode::DY, true},
    {"Z", slmp4e::DeviceCode::Z, false},
    {"LZ", slmp4e::DeviceCode::LZ, false},
    {"R", slmp4e::DeviceCode::R, false},
    {"ZR", slmp4e::DeviceCode::ZR, false},
    {"RD", slmp4e::DeviceCode::RD, false},
    {"G", slmp4e::DeviceCode::G, false},
    {"HG", slmp4e::DeviceCode::HG, false},
};

ArduinoWiznet5500lwIP Ethernet(kEthernetCsPin, SPI, kEthernetIntPin);
EthernetClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

char serial_line[kSerialLineCapacity] = {};
size_t serial_line_length = 0;
bool ethernet_ready = false;
VerificationRecord verification = {};

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

const DeviceSpec* findDeviceSpecByCode(slmp4e::DeviceCode code) {
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

bool parseDeviceAddress(char* token, slmp4e::DeviceAddress& device) {
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

void printDeviceAddress(const slmp4e::DeviceAddress& device, uint32_t offset = 0) {
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

const char* linkStatusText(EthernetLinkStatus status) {
    switch (status) {
        case Unknown:
            return "unknown";
        case LinkON:
            return "on";
        case LinkOFF:
            return "off";
        default:
            return "invalid";
    }
}

const char* hardwareStatusText(HardwareStatus status) {
    switch (status) {
        case EthernetNoHardware:
            return "not_found";
        case EthernetHardwareFound:
            return "found";
        default:
            return "invalid";
    }
}

void printLastFrames() {
    char request_hex[256] = {};
    char response_hex[256] = {};

    if (plc.lastRequestFrameLength() == 0) {
        Serial.println("last request: <none>");
    } else {
        slmp4e::formatHexBytes(
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
        slmp4e::formatHexBytes(
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
    Serial.print(slmp4e::errorString(plc.lastError()));
    Serial.print(" end=0x");
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(" (");
    Serial.print(slmp4e::endCodeString(plc.lastEndCode()));
    Serial.println(")");
    printLastFrames();
}

void printPrompt() {
    Serial.print("> ");
}

bool bringUpEthernet() {
    SPI.setRX(kSpiMisoPin);
    SPI.setSCK(kSpiSckPin);
    SPI.setTX(kSpiMosiPin);

    Ethernet.setSPISpeed(kEthernetSpiHz);
    lwipPollingPeriod(kLwipPollingMs);

    if (!Ethernet.begin(kMacAddress, kLocalIp, kDns, kGateway, kSubnet)) {
        ethernet_ready = false;
        Serial.println("ethernet begin failed");
        Serial.print("hardware=");
        Serial.println(hardwareStatusText(Ethernet.hardwareStatus()));
        Serial.print("link=");
        Serial.println(linkStatusText(Ethernet.linkStatus()));
        return false;
    }

    ethernet_ready = true;
    delay(1000);

    Serial.print("local ip=");
    Serial.println(Ethernet.localIP());
    Serial.print("hardware=");
    Serial.println(hardwareStatusText(Ethernet.hardwareStatus()));
    Serial.print("link=");
    Serial.println(linkStatusText(Ethernet.linkStatus()));
    return true;
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
            Serial.println(slmp4e::errorString(plc.lastError()));
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
    Ethernet.end();
    ethernet_ready = false;
    (void)bringUpEthernet();
}

void printStatus() {
    Serial.print("local ip=");
    Serial.println(Ethernet.localIP());
    Serial.print("hardware=");
    Serial.println(hardwareStatusText(Ethernet.hardwareStatus()));
    Serial.print("link=");
    Serial.println(linkStatusText(Ethernet.linkStatus()));
    Serial.print("plc_connected=");
    Serial.println(plc.connected() ? "yes" : "no");
    Serial.print("timeout_ms=");
    Serial.println(plc.timeoutMs());
    Serial.print("monitoring_timer=");
    Serial.println(plc.monitoringTimer());
    Serial.print("last_error=");
    Serial.println(slmp4e::errorString(plc.lastError()));
    Serial.print("last_end_code=0x");
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(" (");
    Serial.print(slmp4e::endCodeString(plc.lastEndCode()));
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

void printHelp() {
    Serial.println("commands:");
    Serial.println("  help");
    Serial.println("  status");
    Serial.println("  connect");
    Serial.println("  close");
    Serial.println("  reinit");
    Serial.println("  type");
    Serial.println("  rw <device> [points]");
    Serial.println("  ww <device> <value...>");
    Serial.println("  rb <device>");
    Serial.println("  wb <device> <0|1>");
    Serial.println("  verifyw <device> <value...>");
    Serial.println("  verifyb <device> <0|1>");
    Serial.println("  pending");
    Serial.println("  judge <ok|ng> [note]");
    Serial.println("  timeout <ms>");
    Serial.println("  dump");
    Serial.println("examples:");
    Serial.println("  rw D100 2");
    Serial.println("  ww D120 123 456");
    Serial.println("  rb M100");
    Serial.println("  wb M100 1");
    Serial.println("  verifyw D120 123 456");
    Serial.println("  verifyb M100 1");
    Serial.println("  judge ok lamp turned on");
    Serial.println("  rb Y20");
    Serial.println("hex-address devices: X, Y, B, W, SB, SW, DX, DY");
}

void printTypeName() {
    if (!connectPlc(false)) {
        Serial.println("type failed: plc not connected");
        return;
    }

    slmp4e::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp4e::Error::Ok) {
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
    slmp4e::DeviceAddress device = {};
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
    if (plc.readWords(device, points, values, kMaxWordPoints) != slmp4e::Error::Ok) {
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

    slmp4e::DeviceAddress device = {};
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

    if (plc.writeWords(device, values, static_cast<size_t>(value_count)) != slmp4e::Error::Ok) {
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

    slmp4e::DeviceAddress device = {};
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
    if (plc.readWords(device, static_cast<uint16_t>(value_count), before, kMaxWordPoints) != slmp4e::Error::Ok) {
        printLastPlcError("verifyw before read");
        return;
    }

    if (plc.writeWords(device, values, static_cast<size_t>(value_count)) != slmp4e::Error::Ok) {
        printLastPlcError("verifyw write");
        return;
    }

    uint16_t readback[kMaxWordPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(value_count), readback, kMaxWordPoints) != slmp4e::Error::Ok) {
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
    slmp4e::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println("rb usage: rb <device>");
        return;
    }

    if (!connectPlc(false)) {
        Serial.println("rb failed: plc not connected");
        return;
    }

    bool value = false;
    if (plc.readOneBit(device, value) != slmp4e::Error::Ok) {
        printLastPlcError("readOneBit");
        return;
    }

    printDeviceAddress(device);
    Serial.print("=");
    Serial.println(value ? 1 : 0);
}

void writeBitCommand(char* device_token, char* value_token) {
    slmp4e::DeviceAddress device = {};
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

    if (plc.writeOneBit(device, value) != slmp4e::Error::Ok) {
        printLastPlcError("writeOneBit");
        return;
    }

    Serial.println("writeOneBit ok");
}

void verifyBitCommand(char* device_token, char* value_token) {
    slmp4e::DeviceAddress device = {};
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
    if (plc.readOneBit(device, before) != slmp4e::Error::Ok) {
        printLastPlcError("verifyb before read");
        return;
    }

    if (plc.writeOneBit(device, value) != slmp4e::Error::Ok) {
        printLastPlcError("verifyb write");
        return;
    }

    bool readback = false;
    if (plc.readOneBit(device, readback) != slmp4e::Error::Ok) {
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
    } else if (strcmp(tokens[0], "RW") == 0) {
        readWordsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(tokens[0], "WW") == 0) {
        writeWordsCommand(tokens, token_count);
    } else if (strcmp(tokens[0], "RB") == 0) {
        readBitCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(tokens[0], "WB") == 0) {
        writeBitCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
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

    Serial.println("SLMP4E W5500-EVB-Pico2 serial debug console");
    plc.setTimeoutMs(2000);

    (void)bringUpEthernet();
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

}  // namespace w5500_evb_pico2_serial_console
