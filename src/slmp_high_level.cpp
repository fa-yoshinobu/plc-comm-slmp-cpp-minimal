#include "slmp_high_level.h"

#if SLMP_MINIMAL_ENABLE_HIGH_LEVEL

#include <ctype.h>
#include <string.h>

#include <string>
#include <unordered_map>

namespace slmp {
namespace highlevel {

namespace {

struct DeviceMeta {
    const char* name;
    DeviceCode code;
    uint8_t radix;
    bool bit_unit;
    bool batchable_word;
};

enum class LongReadRole : uint8_t {
    Current,
    Contact,
    Coil,
};

struct LongReadAccess {
    DeviceCode base_code;
    LongReadRole role;
};

static const DeviceMeta kDeviceMetas[] = {
    {"LSTS", DeviceCode::LSTS, 10U, true, false},
    {"LSTC", DeviceCode::LSTC, 10U, true, false},
    {"LSTN", DeviceCode::LSTN, 10U, false, true},
    {"LTS", DeviceCode::LTS, 10U, true, false},
    {"LTC", DeviceCode::LTC, 10U, true, false},
    {"LTN", DeviceCode::LTN, 10U, false, true},
    {"LCS", DeviceCode::LCS, 10U, true, false},
    {"LCC", DeviceCode::LCC, 10U, true, false},
    {"LCN", DeviceCode::LCN, 10U, false, true},
    {"STS", DeviceCode::STS, 10U, true, false},
    {"STC", DeviceCode::STC, 10U, true, false},
    {"STN", DeviceCode::STN, 10U, false, true},
    {"SM", DeviceCode::SM, 10U, true, false},
    {"SD", DeviceCode::SD, 10U, false, true},
    {"DX", DeviceCode::DX, 16U, true, false},
    {"DY", DeviceCode::DY, 16U, true, false},
    {"TS", DeviceCode::TS, 10U, true, false},
    {"TC", DeviceCode::TC, 10U, true, false},
    {"TN", DeviceCode::TN, 10U, false, true},
    {"CS", DeviceCode::CS, 10U, true, false},
    {"CC", DeviceCode::CC, 10U, true, false},
    {"CN", DeviceCode::CN, 10U, false, true},
    {"SB", DeviceCode::SB, 16U, true, false},
    {"SW", DeviceCode::SW, 16U, false, true},
    {"ZR", DeviceCode::ZR, 10U, false, true},
    {"RD", DeviceCode::RD, 10U, false, true},
    {"HG", DeviceCode::HG, 10U, false, false},
    {"LZ", DeviceCode::LZ, 10U, false, true},
    {"X", DeviceCode::X, 16U, true, false},
    {"Y", DeviceCode::Y, 16U, true, false},
    {"M", DeviceCode::M, 10U, true, false},
    {"L", DeviceCode::L, 10U, true, false},
    {"F", DeviceCode::F, 10U, true, false},
    {"V", DeviceCode::V, 10U, true, false},
    {"B", DeviceCode::B, 16U, true, false},
    {"D", DeviceCode::D, 10U, false, true},
    {"W", DeviceCode::W, 16U, false, true},
    {"Z", DeviceCode::Z, 10U, false, true},
    {"R", DeviceCode::R, 10U, false, true},
    {"G", DeviceCode::G, 10U, false, false},
};

static bool isSpaceChar(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static std::string trimAscii(const char* text) {
    if (text == nullptr) return std::string();
    const char* start = text;
    while (*start != '\0' && isSpaceChar(*start)) ++start;
    const char* end = start + strlen(start);
    while (end > start && isSpaceChar(*(end - 1))) --end;
    return std::string(start, static_cast<size_t>(end - start));
}

static std::string upperAscii(std::string text) {
    for (size_t i = 0; i < text.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        text[i] = static_cast<char>(toupper(ch));
    }
    return text;
}

static uint64_t deviceKey(const DeviceAddress& device) {
    return (static_cast<uint64_t>(static_cast<uint16_t>(device.code)) << 32) |
           static_cast<uint64_t>(device.number);
}

static bool parseUnsignedNumber(const std::string& text, uint8_t radix, uint32_t& out) {
    if (text.empty()) return false;
    uint32_t value = 0U;
    for (size_t i = 0; i < text.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        uint8_t digit = 0xFFU;
        if (ch >= '0' && ch <= '9') digit = static_cast<uint8_t>(ch - '0');
        else if (ch >= 'A' && ch <= 'F') digit = static_cast<uint8_t>(10U + (ch - 'A'));
        else if (ch >= 'a' && ch <= 'f') digit = static_cast<uint8_t>(10U + (ch - 'a'));
        if (digit == 0xFFU || digit >= radix) return false;
        value = static_cast<uint32_t>(value * radix + digit);
    }
    out = value;
    return true;
}

static const DeviceMeta* findDeviceMeta(const std::string& upper_text, std::string& number_part) {
    for (size_t i = 0; i < sizeof(kDeviceMetas) / sizeof(kDeviceMetas[0]); ++i) {
        const DeviceMeta& meta = kDeviceMetas[i];
        const size_t code_len = strlen(meta.name);
        if (upper_text.size() <= code_len) continue;
        if (upper_text.compare(0, code_len, meta.name) == 0) {
            number_part = upper_text.substr(code_len);
            return &meta;
        }
    }
    return nullptr;
}

static Error parseDeviceOnly(const char* text, DeviceAddress& out, const DeviceMeta** out_meta = nullptr) {
    const std::string trimmed = trimAscii(text);
    if (trimmed.empty()) return Error::InvalidArgument;
    const std::string upper = upperAscii(trimmed);

    std::string number_part;
    const DeviceMeta* meta = findDeviceMeta(upper, number_part);
    if (meta == nullptr) return Error::UnsupportedDevice;

    uint32_t number = 0U;
    if (!parseUnsignedNumber(number_part, meta->radix, number)) return Error::InvalidArgument;

    out.code = meta->code;
    out.number = number;
    if (out_meta != nullptr) *out_meta = meta;
    return Error::Ok;
}

static bool valueTypeUsesDword(ValueType type) {
    return type == ValueType::U32 || type == ValueType::S32 || type == ValueType::Float32;
}

static bool getLongReadAccess(DeviceCode code, LongReadAccess& out) {
    switch (code) {
        case DeviceCode::LTN:
            out = {DeviceCode::LTN, LongReadRole::Current};
            return true;
        case DeviceCode::LTS:
            out = {DeviceCode::LTN, LongReadRole::Contact};
            return true;
        case DeviceCode::LTC:
            out = {DeviceCode::LTN, LongReadRole::Coil};
            return true;
        case DeviceCode::LSTN:
            out = {DeviceCode::LSTN, LongReadRole::Current};
            return true;
        case DeviceCode::LSTS:
            out = {DeviceCode::LSTN, LongReadRole::Contact};
            return true;
        case DeviceCode::LSTC:
            out = {DeviceCode::LSTN, LongReadRole::Coil};
            return true;
        case DeviceCode::LCN:
            out = {DeviceCode::LCN, LongReadRole::Current};
            return true;
        case DeviceCode::LCS:
            out = {DeviceCode::LCN, LongReadRole::Contact};
            return true;
        case DeviceCode::LCC:
            out = {DeviceCode::LCN, LongReadRole::Coil};
            return true;
        default:
            return false;
    }
}

static ValueType defaultValueType(const DeviceMeta& meta) {
    LongReadAccess long_read{};
    if (getLongReadAccess(meta.code, long_read) && long_read.role == LongReadRole::Current) {
        return ValueType::U32;
    }
    return meta.bit_unit ? ValueType::Bit : ValueType::U16;
}

static bool validateLongReadType(DeviceCode code, ValueType type) {
    LongReadAccess long_read{};
    if (!getLongReadAccess(code, long_read)) {
        return true;
    }
    if (long_read.role == LongReadRole::Current) {
        return type == ValueType::U32 || type == ValueType::S32;
    }
    return type == ValueType::Bit;
}

static bool normalizeRequestedType(const DeviceMeta& meta, ValueType& type) {
    if (meta.bit_unit && type == ValueType::U16) {
        type = ValueType::Bit;
        return true;
    }
    if (meta.bit_unit) {
        return type == ValueType::Bit;
    }
    if (type == ValueType::Bit) {
        return false;
    }
    return true;
}

static bool parseTypeText(const std::string& text, ValueType& out) {
    const std::string upper = upperAscii(text);
    if (upper == "BIT") {
        out = ValueType::Bit;
        return true;
    }
    if (upper == "U") {
        out = ValueType::U16;
        return true;
    }
    if (upper == "S") {
        out = ValueType::S16;
        return true;
    }
    if (upper == "D") {
        out = ValueType::U32;
        return true;
    }
    if (upper == "L") {
        out = ValueType::S32;
        return true;
    }
    if (upper == "F") {
        out = ValueType::Float32;
        return true;
    }
    return false;
}

static Value decodeWordValue(uint16_t raw, ValueType type) {
    Value value;
    switch (type) {
        case ValueType::Bit:
            return Value::bitValue(raw != 0U);
        case ValueType::S16:
            return Value::s16Value(static_cast<int16_t>(raw));
        case ValueType::U16:
        default:
            return Value::u16Value(raw);
    }
}

static Value decodeDwordValue(uint32_t raw, ValueType type) {
    switch (type) {
        case ValueType::Float32: {
            float f = 0.0f;
            memcpy(&f, &raw, sizeof(f));
            return Value::float32Value(f);
        }
        case ValueType::S32:
            return Value::s32Value(static_cast<int32_t>(raw));
        case ValueType::U32:
        default:
            return Value::u32Value(raw);
    }
}

static Value decodeLongReadValue(const LongTimerResult& raw, const LongReadAccess& access, ValueType type) {
    switch (access.role) {
        case LongReadRole::Current:
            return decodeDwordValue(raw.current_value, type);
        case LongReadRole::Contact:
            return Value::bitValue(raw.contact);
        case LongReadRole::Coil:
        default:
            return Value::bitValue(raw.coil);
    }
}

static Error readLongLikePoint(
    SlmpClient& client,
    const LongReadAccess& access,
    uint32_t number,
    LongTimerResult& out
) {
    if (access.base_code == DeviceCode::LTN) {
        return client.readLongTimer(static_cast<int>(number), 1, &out, 1);
    }
    if (access.base_code == DeviceCode::LSTN) {
        return client.readLongRetentiveTimer(static_cast<int>(number), 1, &out, 1);
    }

    uint16_t words[4] = {};
    const DeviceAddress device{DeviceCode::LCN, number};
    const Error err = client.readWords(device, 4, words, 4);
    if (err != Error::Ok) return err;

    out.current_value = static_cast<uint32_t>(words[0]) | (static_cast<uint32_t>(words[1]) << 16U);
    out.status_word = words[2];
    out.contact = (words[2] & 0x0002U) != 0U;
    out.coil = (words[2] & 0x0001U) != 0U;
    return Error::Ok;
}

static uint16_t encodeWordValue(const Value& value) {
    if (value.type == ValueType::S16) return static_cast<uint16_t>(value.s16);
    return value.u16;
}

static uint32_t encodeDwordValue(const Value& value) {
    switch (value.type) {
        case ValueType::Float32: {
            uint32_t raw = 0UL;
            memcpy(&raw, &value.f32, sizeof(raw));
            return raw;
        }
        case ValueType::S32:
            return static_cast<uint32_t>(value.s32);
        case ValueType::U32:
        default:
            return value.u32;
    }
}

static bool containsDevice(const std::vector<DeviceAddress>& devices, const DeviceAddress& target) {
    const uint64_t key = deviceKey(target);
    for (size_t i = 0; i < devices.size(); ++i) {
        if (deviceKey(devices[i]) == key) return true;
    }
    return false;
}

static Error readRandomMaps(
    SlmpClient& client,
    const std::vector<DeviceAddress>& word_devices,
    const std::vector<DeviceAddress>& dword_devices,
    std::unordered_map<uint64_t, uint16_t>& word_values,
    std::unordered_map<uint64_t, uint32_t>& dword_values
) {
    size_t word_index = 0U;
    size_t dword_index = 0U;
    while (word_index < word_devices.size() || dword_index < dword_devices.size()) {
        const size_t word_chunk = (word_devices.size() - word_index > 0xFFU) ? 0xFFU : (word_devices.size() - word_index);
        const size_t dword_chunk = (dword_devices.size() - dword_index > 0xFFU) ? 0xFFU : (dword_devices.size() - dword_index);

        std::vector<uint16_t> words(word_chunk);
        std::vector<uint32_t> dwords(dword_chunk);
        const Error err = client.readRandom(
            word_chunk > 0U ? &word_devices[word_index] : nullptr,
            word_chunk,
            word_chunk > 0U ? words.data() : nullptr,
            words.size(),
            dword_chunk > 0U ? &dword_devices[dword_index] : nullptr,
            dword_chunk,
            dword_chunk > 0U ? dwords.data() : nullptr,
            dwords.size()
        );
        if (err != Error::Ok) return err;

        for (size_t i = 0; i < word_chunk; ++i)
            word_values[deviceKey(word_devices[word_index + i])] = words[i];
        for (size_t i = 0; i < dword_chunk; ++i)
            dword_values[deviceKey(dword_devices[dword_index + i])] = dwords[i];

        word_index += word_chunk;
        dword_index += dword_chunk;
    }
    return Error::Ok;
}

}  // namespace

Value Value::bitValue(bool value) {
    Value out;
    out.type = ValueType::Bit;
    out.bit = value;
    return out;
}

Value Value::u16Value(uint16_t value) {
    Value out;
    out.type = ValueType::U16;
    out.u16 = value;
    return out;
}

Value Value::s16Value(int16_t value) {
    Value out;
    out.type = ValueType::S16;
    out.s16 = value;
    return out;
}

Value Value::u32Value(uint32_t value) {
    Value out;
    out.type = ValueType::U32;
    out.u32 = value;
    return out;
}

Value Value::s32Value(int32_t value) {
    Value out;
    out.type = ValueType::S32;
    out.s32 = value;
    return out;
}

Value Value::float32Value(float value) {
    Value out;
    out.type = ValueType::Float32;
    out.f32 = value;
    return out;
}

Error parseAddressSpec(const char* address, AddressSpec& out) {
    const std::string text = trimAscii(address);
    if (text.empty()) return Error::InvalidArgument;

    const size_t colon = text.find(':');
    if (colon != std::string::npos) {
        DeviceAddress device{};
        const DeviceMeta* meta = nullptr;
        const Error err = parseDeviceOnly(text.substr(0, colon).c_str(), device, &meta);
        if (err != Error::Ok) return err;

        ValueType type = ValueType::U16;
        if (!parseTypeText(text.substr(colon + 1U), type)) return Error::InvalidArgument;
        if (!normalizeRequestedType(*meta, type)) return Error::InvalidArgument;
        if (!validateLongReadType(device.code, type)) return Error::InvalidArgument;

        out.device = device;
        out.type = type;
        out.bit_index = -1;
        out.explicit_type = true;
        return Error::Ok;
    }

    const size_t dot = text.find('.');
    if (dot != std::string::npos) {
        DeviceAddress device{};
        const DeviceMeta* meta = nullptr;
        const Error err = parseDeviceOnly(text.substr(0, dot).c_str(), device, &meta);
        if (err != Error::Ok) return err;
        if (meta->bit_unit) return Error::InvalidArgument;

        uint32_t bit_index = 0U;
        if (!parseUnsignedNumber(text.substr(dot + 1U), 16U, bit_index) || bit_index > 15U)
            return Error::InvalidArgument;

        out.device = device;
        out.type = ValueType::Bit;
        out.bit_index = static_cast<int>(bit_index);
        out.explicit_type = false;
        return Error::Ok;
    }

    DeviceAddress device{};
    const DeviceMeta* meta = nullptr;
    const Error err = parseDeviceOnly(text.c_str(), device, &meta);
    if (err != Error::Ok) return err;
    out.device = device;
    out.type = defaultValueType(*meta);
    out.bit_index = -1;
    out.explicit_type = false;
    return Error::Ok;
}

Error readTyped(SlmpClient& client, const char* device, const char* dtype, Value& out) {
    if (device == nullptr || dtype == nullptr) return Error::InvalidArgument;
    const std::string address = trimAscii(device);
    const std::string suffix = trimAscii(dtype);
    if (address.empty() || suffix.empty()) return Error::InvalidArgument;

    AddressSpec spec{};
    const std::string merged = address + ":" + suffix;
    Error err = parseAddressSpec(merged.c_str(), spec);
    if (err != Error::Ok) return err;
    if (spec.bit_index >= 0) return Error::InvalidArgument;

    LongReadAccess long_read{};
    if (getLongReadAccess(spec.device.code, long_read)) {
        LongTimerResult raw{};
        err = readLongLikePoint(client, long_read, spec.device.number, raw);
        if (err != Error::Ok) return err;
        out = decodeLongReadValue(raw, long_read, spec.type);
        return Error::Ok;
    }

    if (spec.type == ValueType::Bit) {
        bool value = false;
        err = client.readOneBit(spec.device, value);
        if (err != Error::Ok) return err;
        out = Value::bitValue(value);
        return Error::Ok;
    }

    if (valueTypeUsesDword(spec.type)) {
        uint32_t raw = 0UL;
        err = client.readOneDWord(spec.device, raw);
        if (err != Error::Ok) return err;
        out = decodeDwordValue(raw, spec.type);
        return Error::Ok;
    }

    uint16_t raw = 0U;
    err = client.readOneWord(spec.device, raw);
    if (err != Error::Ok) return err;
    out = decodeWordValue(raw, spec.type);
    return Error::Ok;
}

Error readTyped(SlmpClient& client, const char* address, Value& out) {
    AddressSpec spec{};
    Error err = parseAddressSpec(address, spec);
    if (err != Error::Ok) return err;

    if (spec.bit_index >= 0) {
        uint16_t word = 0U;
        err = client.readOneWord(spec.device, word);
        if (err != Error::Ok) return err;
        out = Value::bitValue(((word >> spec.bit_index) & 1U) != 0U);
        return Error::Ok;
    }

    LongReadAccess long_read{};
    if (getLongReadAccess(spec.device.code, long_read)) {
        LongTimerResult raw{};
        err = readLongLikePoint(client, long_read, spec.device.number, raw);
        if (err != Error::Ok) return err;
        out = decodeLongReadValue(raw, long_read, spec.type);
        return Error::Ok;
    }

    if (spec.type == ValueType::Bit) {
        bool value = false;
        err = client.readOneBit(spec.device, value);
        if (err != Error::Ok) return err;
        out = Value::bitValue(value);
        return Error::Ok;
    }

    if (valueTypeUsesDword(spec.type)) {
        uint32_t raw = 0UL;
        err = client.readOneDWord(spec.device, raw);
        if (err != Error::Ok) return err;
        out = decodeDwordValue(raw, spec.type);
        return Error::Ok;
    }

    uint16_t raw = 0U;
    err = client.readOneWord(spec.device, raw);
    if (err != Error::Ok) return err;
    out = decodeWordValue(raw, spec.type);
    return Error::Ok;
}

Error writeBitInWord(SlmpClient& client, const char* device, int bit_index, bool value) {
    if (device == nullptr || bit_index < 0 || bit_index > 15) return Error::InvalidArgument;
    DeviceAddress base{};
    const DeviceMeta* meta = nullptr;
    Error err = parseDeviceOnly(device, base, &meta);
    if (err != Error::Ok) return err;
    if (meta->bit_unit) return Error::InvalidArgument;

    uint16_t word = 0U;
    err = client.readOneWord(base, word);
    if (err != Error::Ok) return err;
    if (value) word = static_cast<uint16_t>(word | (1U << bit_index));
    else word = static_cast<uint16_t>(word & ~(1U << bit_index));
    return client.writeOneWord(base, word);
}

Error writeTyped(SlmpClient& client, const char* device, const char* dtype, const Value& value) {
    if (device == nullptr || dtype == nullptr) return Error::InvalidArgument;
    const std::string merged = trimAscii(device) + ":" + trimAscii(dtype);
    AddressSpec spec{};
    Error err = parseAddressSpec(merged.c_str(), spec);
    if (err != Error::Ok) return err;
    if (spec.bit_index >= 0 || spec.type != value.type) return Error::InvalidArgument;

    if (spec.type == ValueType::Bit) return client.writeOneBit(spec.device, value.bit);
    if (valueTypeUsesDword(spec.type)) return client.writeOneDWord(spec.device, encodeDwordValue(value));
    return client.writeOneWord(spec.device, encodeWordValue(value));
}

Error writeTyped(SlmpClient& client, const char* address, const Value& value) {
    AddressSpec spec{};
    Error err = parseAddressSpec(address, spec);
    if (err != Error::Ok) return err;
    if (spec.bit_index >= 0) {
        if (value.type != ValueType::Bit) return Error::InvalidArgument;
        return writeBitInWord(client, trimAscii(address).substr(0, trimAscii(address).find('.')).c_str(), spec.bit_index, value.bit);
    }
    if (spec.type != value.type) return Error::InvalidArgument;
    if (spec.type == ValueType::Bit) return client.writeOneBit(spec.device, value.bit);
    if (valueTypeUsesDword(spec.type)) return client.writeOneDWord(spec.device, encodeDwordValue(value));
    return client.writeOneWord(spec.device, encodeWordValue(value));
}

Error readWordsChunked(
    SlmpClient& client,
    const char* start,
    uint16_t count,
    std::vector<uint16_t>& out,
    uint16_t max_per_request,
    bool allow_split
) {
    if (start == nullptr || count == 0U || max_per_request == 0U) return Error::InvalidArgument;
    DeviceAddress device{};
    const DeviceMeta* meta = nullptr;
    Error err = parseDeviceOnly(start, device, &meta);
    if (err != Error::Ok || meta->bit_unit) return Error::InvalidArgument;
    if (count > max_per_request && !allow_split) return Error::InvalidArgument;

    out.clear();
    out.reserve(count);
    uint32_t offset = 0U;
    uint16_t remaining = count;
    while (remaining > 0U) {
        const uint16_t chunk = (remaining > max_per_request) ? max_per_request : remaining;
        std::vector<uint16_t> buffer(chunk);
        DeviceAddress current = device;
        current.number += offset;
        err = client.readWords(current, chunk, buffer.data(), buffer.size());
        if (err != Error::Ok) return err;
        out.insert(out.end(), buffer.begin(), buffer.end());
        remaining = static_cast<uint16_t>(remaining - chunk);
        offset += chunk;
    }
    return Error::Ok;
}

Error readDWordsChunked(
    SlmpClient& client,
    const char* start,
    uint16_t count,
    std::vector<uint32_t>& out,
    uint16_t max_dwords_per_request,
    bool allow_split
) {
    if (start == nullptr || count == 0U || max_dwords_per_request == 0U) return Error::InvalidArgument;
    DeviceAddress device{};
    const DeviceMeta* meta = nullptr;
    Error err = parseDeviceOnly(start, device, &meta);
    if (err != Error::Ok || meta->bit_unit) return Error::InvalidArgument;
    if (count > max_dwords_per_request && !allow_split) return Error::InvalidArgument;

    out.clear();
    out.reserve(count);
    uint32_t offset = 0U;
    uint16_t remaining = count;
    while (remaining > 0U) {
        const uint16_t chunk = (remaining > max_dwords_per_request) ? max_dwords_per_request : remaining;
        std::vector<uint32_t> buffer(chunk);
        DeviceAddress current = device;
        current.number += offset;
        err = client.readDWords(current, chunk, buffer.data(), buffer.size());
        if (err != Error::Ok) return err;
        out.insert(out.end(), buffer.begin(), buffer.end());
        remaining = static_cast<uint16_t>(remaining - chunk);
        offset += static_cast<uint32_t>(chunk) * 2U;
    }
    return Error::Ok;
}

Error compileReadPlan(const std::vector<std::string>& addresses, ReadPlan& out) {
    out.entries.clear();
    out.word_devices.clear();
    out.dword_devices.clear();
    out.entries.reserve(addresses.size());

    for (size_t i = 0; i < addresses.size(); ++i) {
        AddressSpec spec{};
        Error err = parseAddressSpec(addresses[i].c_str(), spec);
        if (err != Error::Ok) return err;

        const DeviceMeta* meta = nullptr;
        DeviceAddress verified{};
        err = parseDeviceOnly(addresses[i].find('.') != std::string::npos
                ? addresses[i].substr(0, addresses[i].find('.')).c_str()
                : addresses[i].find(':') != std::string::npos
                    ? addresses[i].substr(0, addresses[i].find(':')).c_str()
                    : addresses[i].c_str(),
            verified,
            &meta);
        if (err != Error::Ok) return err;

        BatchKind kind = BatchKind::None;
        LongReadAccess long_read{};
        if (spec.bit_index >= 0) {
            if (meta->batchable_word) {
                kind = BatchKind::BitInWord;
                if (!containsDevice(out.word_devices, spec.device)) out.word_devices.push_back(spec.device);
            }
        } else if (getLongReadAccess(spec.device.code, long_read)) {
            kind = BatchKind::LongTimer;
        } else if (!meta->bit_unit && meta->batchable_word) {
            if (valueTypeUsesDword(spec.type)) {
                kind = BatchKind::Dword;
                if (!containsDevice(out.dword_devices, spec.device)) out.dword_devices.push_back(spec.device);
            } else {
                kind = BatchKind::Word;
                if (!containsDevice(out.word_devices, spec.device)) out.word_devices.push_back(spec.device);
            }
        }

        ReadPlanEntry entry;
        entry.address = addresses[i];
        entry.spec = spec;
        entry.kind = kind;
        out.entries.push_back(entry);
    }
    return Error::Ok;
}

Error readNamed(SlmpClient& client, const std::vector<std::string>& addresses, Snapshot& out) {
    ReadPlan plan;
    Error err = compileReadPlan(addresses, plan);
    if (err != Error::Ok) return err;
    return readNamed(client, plan, out);
}

Error readNamed(SlmpClient& client, const ReadPlan& plan, Snapshot& out) {
    out.clear();
    out.reserve(plan.entries.size());

    std::unordered_map<uint64_t, uint16_t> word_values;
    std::unordered_map<uint64_t, uint32_t> dword_values;
    std::unordered_map<uint64_t, LongTimerResult> long_values;
    Error err = readRandomMaps(client, plan.word_devices, plan.dword_devices, word_values, dword_values);
    if (err != Error::Ok) return err;

    for (size_t i = 0; i < plan.entries.size(); ++i) {
        const ReadPlanEntry& entry = plan.entries[i];
        NamedValue item;
        item.address = entry.address;

        switch (entry.kind) {
            case BatchKind::Word:
                item.value = decodeWordValue(word_values[deviceKey(entry.spec.device)], entry.spec.type);
                break;
            case BatchKind::BitInWord: {
                const uint16_t word = word_values[deviceKey(entry.spec.device)];
                item.value = Value::bitValue(((word >> entry.spec.bit_index) & 1U) != 0U);
                break;
            }
            case BatchKind::Dword:
                item.value = decodeDwordValue(dword_values[deviceKey(entry.spec.device)], entry.spec.type);
                break;
            case BatchKind::LongTimer: {
                LongReadAccess long_read{};
                if (!getLongReadAccess(entry.spec.device.code, long_read)) return Error::InvalidArgument;
                const DeviceAddress base_device{long_read.base_code, entry.spec.device.number};
                const uint64_t key = deviceKey(base_device);
                auto found = long_values.find(key);
                if (found == long_values.end()) {
                    LongTimerResult raw{};
                    err = readLongLikePoint(client, long_read, entry.spec.device.number, raw);
                    if (err != Error::Ok) return err;
                    found = long_values.emplace(key, raw).first;
                }
                item.value = decodeLongReadValue(found->second, long_read, entry.spec.type);
                break;
            }
            case BatchKind::None:
            default:
                err = readTyped(client, entry.address.c_str(), item.value);
                if (err != Error::Ok) return err;
                break;
        }

        out.push_back(item);
    }

    return Error::Ok;
}

Error writeNamed(SlmpClient& client, const Snapshot& updates) {
    for (size_t i = 0; i < updates.size(); ++i) {
        const Error err = writeTyped(client, updates[i].address.c_str(), updates[i].value);
        if (err != Error::Ok) return err;
    }
    return Error::Ok;
}

Error Poller::compile(const std::vector<std::string>& addresses) {
    return compileReadPlan(addresses, plan_);
}

Error Poller::readOnce(SlmpClient& client, Snapshot& out) const {
    return readNamed(client, plan_, out);
}

}  // namespace highlevel
}  // namespace slmp

#endif
