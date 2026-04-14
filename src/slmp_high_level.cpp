#include "slmp_high_level.h"

#if SLMP_MINIMAL_ENABLE_HIGH_LEVEL

#include <algorithm>
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

static const DeviceMeta* findDeviceMetaByCode(DeviceCode code) {
    for (size_t i = 0; i < sizeof(kDeviceMetas) / sizeof(kDeviceMetas[0]); ++i) {
        if (kDeviceMetas[i].code == code) return &kDeviceMetas[i];
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

enum class WriteRoute : uint8_t {
    Word,
    Dword,
    Bit,
    RandomDword,
    RandomBit,
};

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
    if (meta.code == DeviceCode::LZ) {
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

static WriteRoute resolveWriteRoute(const AddressSpec& spec) {
    if (spec.type == ValueType::Bit) {
        switch (spec.device.code) {
            case DeviceCode::LTS:
            case DeviceCode::LTC:
            case DeviceCode::LSTS:
            case DeviceCode::LSTC:
                return WriteRoute::RandomBit;
            default:
                return WriteRoute::Bit;
        }
    }

    if (valueTypeUsesDword(spec.type)) {
        switch (spec.device.code) {
            case DeviceCode::LTN:
            case DeviceCode::LSTN:
            case DeviceCode::LZ:
                return WriteRoute::RandomDword;
            default:
                return WriteRoute::Dword;
        }
    }

    return WriteRoute::Word;
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

static const char* valueTypeSuffix(ValueType type) {
    switch (type) {
        case ValueType::Bit:
            return "BIT";
        case ValueType::U16:
            return "U";
        case ValueType::S16:
            return "S";
        case ValueType::U32:
            return "D";
        case ValueType::S32:
            return "L";
        case ValueType::Float32:
            return "F";
        default:
            return nullptr;
    }
}

static void appendUnsignedText(std::string& out, uint32_t value, uint8_t radix) {
    char scratch[16] = {};
    size_t length = 0U;
    do {
        const uint32_t digit = value % radix;
        scratch[length++] = "0123456789ABCDEF"[digit];
        value /= radix;
    } while (value != 0U);
    while (length > 0U) {
        out.push_back(scratch[--length]);
    }
}

static Error copyTextToBuffer(const std::string& text, char* out, size_t out_size) {
    if (out == nullptr || out_size == 0U) return Error::InvalidArgument;
    if (text.size() + 1U > out_size) return Error::BufferTooSmall;
    memcpy(out, text.c_str(), text.size() + 1U);
    return Error::Ok;
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

template <typename T, size_t N>
constexpr size_t countOf(const T (&)[N]) { return N; }

struct DeviceRangeItemDevice {
    const char* name;
    bool is_bit_device;
};

struct DeviceRangeRowSpec {
    const char* item;
    DeviceRangeCategory category;
    const DeviceRangeItemDevice* devices;
    size_t device_count;
    DeviceRangeNotation notation;
};

enum class DeviceRangeValueKind : uint8_t {
    Unsupported,
    Undefined,
    Fixed,
    WordRegister,
    DWordRegister,
    WordRegisterClipped,
    DWordRegisterClipped,
};

struct DeviceRangeValueSpecInternal {
    DeviceRangeValueKind kind;
    uint16_t reg;
    uint32_t fixed_value;
    uint32_t clip_value;
    const char* source;
    const char* notes;
};

struct DeviceRangeRuleSpec {
    const char* item;
    DeviceRangeValueSpecInternal value;
};

struct DeviceRangeProfileSpec {
    DeviceRangeFamily family;
    uint16_t register_start;
    uint16_t register_count;
    const DeviceRangeRuleSpec* rules;
    size_t rule_count;
};

constexpr DeviceRangeValueSpecInternal rangeFixed(uint32_t value, const char* source) {
    return {DeviceRangeValueKind::Fixed, 0U, value, 0U, source, nullptr};
}

constexpr DeviceRangeValueSpecInternal rangeWord(uint16_t reg, const char* source, const char* notes = nullptr) {
    return {DeviceRangeValueKind::WordRegister, reg, 0U, 0U, source, notes};
}

constexpr DeviceRangeValueSpecInternal rangeDWord(uint16_t reg, const char* source, const char* notes = nullptr) {
    return {DeviceRangeValueKind::DWordRegister, reg, 0U, 0U, source, notes};
}

constexpr DeviceRangeValueSpecInternal rangeWordClipped(uint16_t reg, uint32_t clip, const char* source, const char* notes = nullptr) {
    return {DeviceRangeValueKind::WordRegisterClipped, reg, 0U, clip, source, notes};
}

constexpr DeviceRangeValueSpecInternal rangeDWordClipped(uint16_t reg, uint32_t clip, const char* source, const char* notes = nullptr) {
    return {DeviceRangeValueKind::DWordRegisterClipped, reg, 0U, clip, source, notes};
}

constexpr DeviceRangeValueSpecInternal rangeUnsupported(const char* notes) {
    return {DeviceRangeValueKind::Unsupported, 0U, 0U, 0U, "Unsupported", notes};
}

constexpr DeviceRangeValueSpecInternal rangeUndefined(const char* notes) {
    return {DeviceRangeValueKind::Undefined, 0U, 0U, 0U, "Undefined", notes};
}

const DeviceRangeItemDevice kXDevice[] = {{"X", true}};
const DeviceRangeItemDevice kYDevice[] = {{"Y", true}};
const DeviceRangeItemDevice kMDevice[] = {{"M", true}};
const DeviceRangeItemDevice kBDevice[] = {{"B", true}};
const DeviceRangeItemDevice kSBDevice[] = {{"SB", true}};
const DeviceRangeItemDevice kFDevice[] = {{"F", true}};
const DeviceRangeItemDevice kVDevice[] = {{"V", true}};
const DeviceRangeItemDevice kLDevice[] = {{"L", true}};
const DeviceRangeItemDevice kSDevice[] = {{"S", true}};
const DeviceRangeItemDevice kDDevice[] = {{"D", false}};
const DeviceRangeItemDevice kWDevice[] = {{"W", false}};
const DeviceRangeItemDevice kSWDevice[] = {{"SW", false}};
const DeviceRangeItemDevice kRDevice[] = {{"R", false}};
const DeviceRangeItemDevice kTDevices[] = {{"TS", true}, {"TC", true}, {"TN", false}};
const DeviceRangeItemDevice kSTDevices[] = {{"STS", true}, {"STC", true}, {"STN", false}};
const DeviceRangeItemDevice kCDevices[] = {{"CS", true}, {"CC", true}, {"CN", false}};
const DeviceRangeItemDevice kLTDevices[] = {{"LTS", true}, {"LTC", true}, {"LTN", false}};
const DeviceRangeItemDevice kLSTDevices[] = {{"LSTS", true}, {"LSTC", true}, {"LSTN", false}};
const DeviceRangeItemDevice kLCDevices[] = {{"LCS", true}, {"LCC", true}, {"LCN", false}};
const DeviceRangeItemDevice kZDevice[] = {{"Z", false}};
const DeviceRangeItemDevice kLZDevice[] = {{"LZ", false}};
const DeviceRangeItemDevice kZRDevice[] = {{"ZR", false}};
const DeviceRangeItemDevice kRDDevice[] = {{"RD", false}};
const DeviceRangeItemDevice kSMDevice[] = {{"SM", true}};
const DeviceRangeItemDevice kSDDevice[] = {{"SD", false}};

const DeviceRangeRowSpec kDeviceRangeRows[] = {
    {"X", DeviceRangeCategory::Bit, kXDevice, countOf(kXDevice), DeviceRangeNotation::Base16},
    {"Y", DeviceRangeCategory::Bit, kYDevice, countOf(kYDevice), DeviceRangeNotation::Base16},
    {"M", DeviceRangeCategory::Bit, kMDevice, countOf(kMDevice), DeviceRangeNotation::Base10},
    {"B", DeviceRangeCategory::Bit, kBDevice, countOf(kBDevice), DeviceRangeNotation::Base16},
    {"SB", DeviceRangeCategory::Bit, kSBDevice, countOf(kSBDevice), DeviceRangeNotation::Base16},
    {"F", DeviceRangeCategory::Bit, kFDevice, countOf(kFDevice), DeviceRangeNotation::Base10},
    {"V", DeviceRangeCategory::Bit, kVDevice, countOf(kVDevice), DeviceRangeNotation::Base10},
    {"L", DeviceRangeCategory::Bit, kLDevice, countOf(kLDevice), DeviceRangeNotation::Base10},
    {"S", DeviceRangeCategory::Bit, kSDevice, countOf(kSDevice), DeviceRangeNotation::Base10},
    {"D", DeviceRangeCategory::Word, kDDevice, countOf(kDDevice), DeviceRangeNotation::Base10},
    {"W", DeviceRangeCategory::Word, kWDevice, countOf(kWDevice), DeviceRangeNotation::Base16},
    {"SW", DeviceRangeCategory::Word, kSWDevice, countOf(kSWDevice), DeviceRangeNotation::Base16},
    {"R", DeviceRangeCategory::Word, kRDevice, countOf(kRDevice), DeviceRangeNotation::Base10},
    {"T", DeviceRangeCategory::TimerCounter, kTDevices, countOf(kTDevices), DeviceRangeNotation::Base10},
    {"ST", DeviceRangeCategory::TimerCounter, kSTDevices, countOf(kSTDevices), DeviceRangeNotation::Base10},
    {"C", DeviceRangeCategory::TimerCounter, kCDevices, countOf(kCDevices), DeviceRangeNotation::Base10},
    {"LT", DeviceRangeCategory::TimerCounter, kLTDevices, countOf(kLTDevices), DeviceRangeNotation::Base10},
    {"LST", DeviceRangeCategory::TimerCounter, kLSTDevices, countOf(kLSTDevices), DeviceRangeNotation::Base10},
    {"LC", DeviceRangeCategory::TimerCounter, kLCDevices, countOf(kLCDevices), DeviceRangeNotation::Base10},
    {"Z", DeviceRangeCategory::Index, kZDevice, countOf(kZDevice), DeviceRangeNotation::Base10},
    {"LZ", DeviceRangeCategory::Index, kLZDevice, countOf(kLZDevice), DeviceRangeNotation::Base10},
    {"ZR", DeviceRangeCategory::FileRefresh, kZRDevice, countOf(kZRDevice), DeviceRangeNotation::Base10},
    {"RD", DeviceRangeCategory::FileRefresh, kRDDevice, countOf(kRDDevice), DeviceRangeNotation::Base10},
    {"SM", DeviceRangeCategory::Bit, kSMDevice, countOf(kSMDevice), DeviceRangeNotation::Base10},
    {"SD", DeviceRangeCategory::Word, kSDDevice, countOf(kSDDevice), DeviceRangeNotation::Base10},
};

const DeviceRangeRuleSpec kIqRRangeRules[] = {
    {"X", rangeDWord(260U, "SD260-SD261 (32-bit)")},
    {"Y", rangeDWord(262U, "SD262-SD263 (32-bit)")},
    {"M", rangeDWord(264U, "SD264-SD265 (32-bit)")},
    {"B", rangeDWord(266U, "SD266-SD267 (32-bit)")},
    {"SB", rangeDWord(268U, "SD268-SD269 (32-bit)")},
    {"F", rangeDWord(270U, "SD270-SD271 (32-bit)")},
    {"V", rangeDWord(272U, "SD272-SD273 (32-bit)")},
    {"L", rangeDWord(274U, "SD274-SD275 (32-bit)")},
    {"S", rangeDWord(276U, "SD276-SD277 (32-bit)")},
    {"D", rangeDWord(280U, "SD280-SD281 (32-bit)")},
    {"W", rangeDWord(282U, "SD282-SD283 (32-bit)")},
    {"SW", rangeDWord(284U, "SD284-SD285 (32-bit)")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"ST", rangeDWord(290U, "SD290-SD291 (32-bit)")},
    {"C", rangeDWord(292U, "SD292-SD293 (32-bit)")},
    {"LT", rangeDWord(294U, "SD294-SD295 (32-bit)")},
    {"LST", rangeDWord(296U, "SD296-SD297 (32-bit)")},
    {"LC", rangeDWord(298U, "SD298-SD299 (32-bit)")},
    {"Z", rangeWord(300U, "SD300")},
    {"LZ", rangeWord(302U, "SD302")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"SM", rangeFixed(4096U, "Fixed family limit")},
    {"SD", rangeFixed(4096U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kMxFRangeRules[] = {
    {"X", rangeDWord(260U, "SD260-SD261 (32-bit)")},
    {"Y", rangeDWord(262U, "SD262-SD263 (32-bit)")},
    {"M", rangeDWord(264U, "SD264-SD265 (32-bit)")},
    {"B", rangeDWord(266U, "SD266-SD267 (32-bit)")},
    {"SB", rangeDWord(268U, "SD268-SD269 (32-bit)")},
    {"F", rangeDWord(270U, "SD270-SD271 (32-bit)")},
    {"V", rangeDWord(272U, "SD272-SD273 (32-bit)")},
    {"L", rangeDWord(274U, "SD274-SD275 (32-bit)")},
    {"S", rangeUnsupported("Not supported on MX-F.")},
    {"D", rangeDWord(280U, "SD280-SD281 (32-bit)")},
    {"W", rangeDWord(282U, "SD282-SD283 (32-bit)")},
    {"SW", rangeDWord(284U, "SD284-SD285 (32-bit)")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"ST", rangeDWord(290U, "SD290-SD291 (32-bit)")},
    {"C", rangeDWord(292U, "SD292-SD293 (32-bit)")},
    {"LT", rangeDWord(294U, "SD294-SD295 (32-bit)")},
    {"LST", rangeDWord(296U, "SD296-SD297 (32-bit)")},
    {"LC", rangeDWord(298U, "SD298-SD299 (32-bit)")},
    {"Z", rangeWord(300U, "SD300")},
    {"LZ", rangeWord(302U, "SD302")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"SM", rangeFixed(10000U, "Fixed family limit")},
    {"SD", rangeFixed(10000U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kMxRRangeRules[] = {
    {"X", rangeDWord(260U, "SD260-SD261 (32-bit)")},
    {"Y", rangeDWord(262U, "SD262-SD263 (32-bit)")},
    {"M", rangeDWord(264U, "SD264-SD265 (32-bit)")},
    {"B", rangeDWord(266U, "SD266-SD267 (32-bit)")},
    {"SB", rangeDWord(268U, "SD268-SD269 (32-bit)")},
    {"F", rangeDWord(270U, "SD270-SD271 (32-bit)")},
    {"V", rangeDWord(272U, "SD272-SD273 (32-bit)")},
    {"L", rangeDWord(274U, "SD274-SD275 (32-bit)")},
    {"S", rangeUnsupported("Not supported on MX-R.")},
    {"D", rangeDWord(280U, "SD280-SD281 (32-bit)")},
    {"W", rangeDWord(282U, "SD282-SD283 (32-bit)")},
    {"SW", rangeDWord(284U, "SD284-SD285 (32-bit)")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"ST", rangeDWord(290U, "SD290-SD291 (32-bit)")},
    {"C", rangeDWord(292U, "SD292-SD293 (32-bit)")},
    {"LT", rangeDWord(294U, "SD294-SD295 (32-bit)")},
    {"LST", rangeDWord(296U, "SD296-SD297 (32-bit)")},
    {"LC", rangeDWord(298U, "SD298-SD299 (32-bit)")},
    {"Z", rangeWord(300U, "SD300")},
    {"LZ", rangeWord(302U, "SD302")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"SM", rangeFixed(4496U, "Fixed family limit")},
    {"SD", rangeFixed(4496U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kIqFRangeRules[] = {
    {"X", rangeDWord(260U, "SD260-SD261 (32-bit)", "Manual addressing for iQ-F X devices is octal.")},
    {"Y", rangeDWord(262U, "SD262-SD263 (32-bit)", "Manual addressing for iQ-F Y devices is octal.")},
    {"M", rangeDWord(264U, "SD264-SD265 (32-bit)")},
    {"B", rangeDWord(266U, "SD266-SD267 (32-bit)")},
    {"SB", rangeDWord(268U, "SD268-SD269 (32-bit)")},
    {"F", rangeDWord(270U, "SD270-SD271 (32-bit)")},
    {"V", rangeUnsupported("Not supported on iQ-F.")},
    {"L", rangeDWord(274U, "SD274-SD275 (32-bit)")},
    {"S", rangeUnsupported("Not supported on iQ-F.")},
    {"D", rangeDWord(280U, "SD280-SD281 (32-bit)")},
    {"W", rangeDWord(282U, "SD282-SD283 (32-bit)")},
    {"SW", rangeDWord(284U, "SD284-SD285 (32-bit)")},
    {"R", rangeDWord(304U, "SD304-SD305 (32-bit)")},
    {"T", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"ST", rangeDWord(290U, "SD290-SD291 (32-bit)")},
    {"C", rangeDWord(292U, "SD292-SD293 (32-bit)")},
    {"LT", rangeUnsupported("Not supported on iQ-F.")},
    {"LST", rangeUnsupported("Not supported on iQ-F.")},
    {"LC", rangeDWord(298U, "SD298-SD299 (32-bit)")},
    {"Z", rangeWord(300U, "SD300")},
    {"LZ", rangeWord(302U, "SD302")},
    {"ZR", rangeUnsupported("Not supported on iQ-F.")},
    {"RD", rangeUnsupported("Not supported on iQ-F.")},
    {"SM", rangeFixed(10000U, "Fixed family limit")},
    {"SD", rangeFixed(12000U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kQCpuRangeRules[] = {
    {"X", rangeWord(290U, "SD290")},
    {"Y", rangeWord(291U, "SD291")},
    {"M", rangeWordClipped(292U, 32768U, "SD292", "Upper bound is clipped to 32768.")},
    {"B", rangeWordClipped(294U, 32768U, "SD294", "Upper bound is clipped to 32768.")},
    {"SB", rangeWord(296U, "SD296")},
    {"F", rangeWord(295U, "SD295")},
    {"V", rangeWord(297U, "SD297")},
    {"L", rangeWord(293U, "SD293")},
    {"S", rangeWord(298U, "SD298")},
    {"D", rangeWordClipped(302U, 32768U, "SD302", "Upper bound is clipped to 32768 and excludes extended area.")},
    {"W", rangeWordClipped(303U, 32768U, "SD303", "Upper bound is clipped to 32768 and excludes extended area.")},
    {"SW", rangeWord(304U, "SD304")},
    {"R", rangeFixed(32768U, "Fixed family limit")},
    {"T", rangeWord(299U, "SD299")},
    {"ST", rangeWord(300U, "SD300")},
    {"C", rangeWord(301U, "SD301")},
    {"LT", rangeUnsupported("Not supported on QCPU.")},
    {"LST", rangeUnsupported("Not supported on QCPU.")},
    {"LC", rangeUnsupported("Not supported on QCPU.")},
    {"Z", rangeFixed(10U, "Fixed family limit")},
    {"LZ", rangeUnsupported("Not supported on QCPU.")},
    {"ZR", rangeUndefined("No finite upper-bound register is defined for QCPU ZR.")},
    {"RD", rangeUnsupported("Not supported on QCPU.")},
    {"SM", rangeFixed(1024U, "Fixed family limit")},
    {"SD", rangeFixed(1024U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kLCpuRangeRules[] = {
    {"X", rangeWord(290U, "SD290")},
    {"Y", rangeWord(291U, "SD291")},
    {"M", rangeDWord(286U, "SD286-SD287 (32-bit)")},
    {"B", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"SB", rangeWord(296U, "SD296")},
    {"F", rangeWord(295U, "SD295")},
    {"V", rangeWord(297U, "SD297")},
    {"L", rangeWord(293U, "SD293")},
    {"S", rangeWord(298U, "SD298")},
    {"D", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"W", rangeDWord(310U, "SD310-SD311 (32-bit)")},
    {"SW", rangeWord(304U, "SD304")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeWord(299U, "SD299")},
    {"ST", rangeWord(300U, "SD300")},
    {"C", rangeWord(301U, "SD301")},
    {"LT", rangeUnsupported("Not supported on LCPU.")},
    {"LST", rangeUnsupported("Not supported on LCPU.")},
    {"LC", rangeUnsupported("Not supported on LCPU.")},
    {"Z", rangeWord(305U, "SD305", "Requires ZZ = FFFFh for the reported upper bound.")},
    {"LZ", rangeUnsupported("Not supported on LCPU.")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeUnsupported("Not supported on LCPU.")},
    {"SM", rangeFixed(2048U, "Fixed family limit")},
    {"SD", rangeFixed(2048U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kQnURangeRules[] = {
    {"X", rangeWord(290U, "SD290")},
    {"Y", rangeWord(291U, "SD291")},
    {"M", rangeDWord(286U, "SD286-SD287 (32-bit)")},
    {"B", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"SB", rangeWord(296U, "SD296")},
    {"F", rangeWord(295U, "SD295")},
    {"V", rangeWord(297U, "SD297")},
    {"L", rangeWord(293U, "SD293")},
    {"S", rangeWord(298U, "SD298")},
    {"D", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"W", rangeDWord(310U, "SD310-SD311 (32-bit)")},
    {"SW", rangeWord(304U, "SD304")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeWord(299U, "SD299")},
    {"ST", rangeWord(300U, "SD300")},
    {"C", rangeWord(301U, "SD301")},
    {"LT", rangeUnsupported("Not supported on QnU.")},
    {"LST", rangeUnsupported("Not supported on QnU.")},
    {"LC", rangeUnsupported("Not supported on QnU.")},
    {"Z", rangeWord(305U, "SD305", "Requires ZZ = FFFFh for the reported upper bound.")},
    {"LZ", rangeUnsupported("Not supported on QnU.")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeUnsupported("Not supported on QnU.")},
    {"SM", rangeFixed(2048U, "Fixed family limit")},
    {"SD", rangeFixed(2048U, "Fixed family limit")},
};

const DeviceRangeRuleSpec kQnUDVRangeRules[] = {
    {"X", rangeWord(290U, "SD290")},
    {"Y", rangeWord(291U, "SD291")},
    {"M", rangeDWord(286U, "SD286-SD287 (32-bit)")},
    {"B", rangeDWord(288U, "SD288-SD289 (32-bit)")},
    {"SB", rangeWord(296U, "SD296")},
    {"F", rangeWord(295U, "SD295")},
    {"V", rangeWord(297U, "SD297")},
    {"L", rangeWord(293U, "SD293")},
    {"S", rangeWord(298U, "SD298")},
    {"D", rangeDWord(308U, "SD308-SD309 (32-bit)")},
    {"W", rangeDWord(310U, "SD310-SD311 (32-bit)")},
    {"SW", rangeWord(304U, "SD304")},
    {"R", rangeDWordClipped(306U, 32768U, "SD306-SD307 (32-bit)", "Upper bound is clipped to 32768.")},
    {"T", rangeWord(299U, "SD299")},
    {"ST", rangeWord(300U, "SD300")},
    {"C", rangeWord(301U, "SD301")},
    {"LT", rangeUnsupported("Not supported on QnUDV.")},
    {"LST", rangeUnsupported("Not supported on QnUDV.")},
    {"LC", rangeUnsupported("Not supported on QnUDV.")},
    {"Z", rangeWord(305U, "SD305", "Requires ZZ = FFFFh for the reported upper bound.")},
    {"LZ", rangeUnsupported("Not supported on QnUDV.")},
    {"ZR", rangeDWord(306U, "SD306-SD307 (32-bit)")},
    {"RD", rangeUnsupported("Not supported on QnUDV.")},
    {"SM", rangeFixed(2048U, "Fixed family limit")},
    {"SD", rangeFixed(2048U, "Fixed family limit")},
};

const DeviceRangeProfileSpec kDeviceRangeProfiles[] = {
    {DeviceRangeFamily::IqR, 260U, 50U, kIqRRangeRules, countOf(kIqRRangeRules)},
    {DeviceRangeFamily::MxF, 260U, 50U, kMxFRangeRules, countOf(kMxFRangeRules)},
    {DeviceRangeFamily::MxR, 260U, 50U, kMxRRangeRules, countOf(kMxRRangeRules)},
    {DeviceRangeFamily::IqF, 260U, 46U, kIqFRangeRules, countOf(kIqFRangeRules)},
    {DeviceRangeFamily::QCpu, 290U, 15U, kQCpuRangeRules, countOf(kQCpuRangeRules)},
    {DeviceRangeFamily::LCpu, 286U, 26U, kLCpuRangeRules, countOf(kLCpuRangeRules)},
    {DeviceRangeFamily::QnU, 286U, 26U, kQnURangeRules, countOf(kQnURangeRules)},
    {DeviceRangeFamily::QnUDV, 286U, 26U, kQnUDVRangeRules, countOf(kQnUDVRangeRules)},
};

static const char* deviceRangeFamilyLabelImpl(DeviceRangeFamily family) {
    switch (family) {
        case DeviceRangeFamily::IqR: return "IQ-R";
        case DeviceRangeFamily::MxF: return "MX-F";
        case DeviceRangeFamily::MxR: return "MX-R";
        case DeviceRangeFamily::IqF: return "IQ-F";
        case DeviceRangeFamily::QCpu: return "QCPU";
        case DeviceRangeFamily::LCpu: return "LCPU";
        case DeviceRangeFamily::QnU: return "QnU";
        case DeviceRangeFamily::QnUDV: return "QnUDV";
        default: return "";
    }
}

static const DeviceRangeProfileSpec* findDeviceRangeProfile(DeviceRangeFamily family) {
    for (size_t i = 0; i < countOf(kDeviceRangeProfiles); ++i) {
        if (kDeviceRangeProfiles[i].family == family) return &kDeviceRangeProfiles[i];
    }
    return nullptr;
}

static const DeviceRangeRuleSpec* findDeviceRangeRule(const DeviceRangeProfileSpec& profile, const char* item) {
    for (size_t i = 0; i < profile.rule_count; ++i) {
        if (strcmp(profile.rules[i].item, item) == 0) return &profile.rules[i];
    }
    return nullptr;
}

static DeviceRangeNotation resolveDeviceRangeNotation(DeviceRangeFamily family, const char* device, DeviceRangeNotation fallback) {
    if (family == DeviceRangeFamily::IqF &&
        (strcmp(device, "X") == 0 || strcmp(device, "Y") == 0))
        return DeviceRangeNotation::Base8;
    return fallback;
}

static bool readDeviceRangeWord(const std::vector<uint16_t>& registers, uint16_t register_start, uint16_t reg, uint32_t& out) {
    if (reg < register_start) return false;
    const size_t index = static_cast<size_t>(reg - register_start);
    if (index >= registers.size()) return false;
    out = registers[index];
    return true;
}

static bool readDeviceRangeDWord(const std::vector<uint16_t>& registers, uint16_t register_start, uint16_t reg, uint32_t& out) {
    uint32_t low = 0U;
    uint32_t high = 0U;
    if (!readDeviceRangeWord(registers, register_start, reg, low)) return false;
    if (!readDeviceRangeWord(registers, register_start, static_cast<uint16_t>(reg + 1U), high)) return false;
    out = low | (high << 16U);
    return true;
}

static Error evaluateDeviceRangePointCount(
    const DeviceRangeValueSpecInternal& spec,
    const std::vector<uint16_t>& registers,
    uint16_t register_start,
    bool& has_point_count,
    uint32_t& point_count
) {
    has_point_count = false;
    point_count = 0U;
    switch (spec.kind) {
        case DeviceRangeValueKind::Unsupported:
        case DeviceRangeValueKind::Undefined:
            return Error::Ok;
        case DeviceRangeValueKind::Fixed:
            has_point_count = true;
            point_count = spec.fixed_value;
            return Error::Ok;
        case DeviceRangeValueKind::WordRegister:
            if (!readDeviceRangeWord(registers, register_start, spec.reg, point_count)) return Error::ProtocolError;
            has_point_count = true;
            return Error::Ok;
        case DeviceRangeValueKind::DWordRegister:
            if (!readDeviceRangeDWord(registers, register_start, spec.reg, point_count)) return Error::ProtocolError;
            has_point_count = true;
            return Error::Ok;
        case DeviceRangeValueKind::WordRegisterClipped:
            if (!readDeviceRangeWord(registers, register_start, spec.reg, point_count)) return Error::ProtocolError;
            point_count = std::min(point_count, spec.clip_value);
            has_point_count = true;
            return Error::Ok;
        case DeviceRangeValueKind::DWordRegisterClipped:
            if (!readDeviceRangeDWord(registers, register_start, spec.reg, point_count)) return Error::ProtocolError;
            point_count = std::min(point_count, spec.clip_value);
            has_point_count = true;
            return Error::Ok;
        default:
            return Error::InvalidArgument;
    }
}

static void appendUnsignedTextPadded(std::string& out, uint32_t value, uint8_t radix, size_t width) {
    std::string formatted;
    appendUnsignedText(formatted, value, radix);
    if (formatted.size() < width) out.append(width - formatted.size(), '0');
    out += formatted;
}

static std::string formatDeviceRangeAddress(const char* device, DeviceRangeNotation notation, bool has_upper_bound, uint32_t upper_bound) {
    if (!has_upper_bound) return std::string();
    std::string text(device);
    if (notation == DeviceRangeNotation::Base10) {
        text.push_back('0');
        text.push_back('-');
        text += device;
        appendUnsignedText(text, upper_bound, 10U);
        return text;
    }

    const uint8_t radix = (notation == DeviceRangeNotation::Base8) ? 8U : 16U;
    std::string upper_text;
    appendUnsignedText(upper_text, upper_bound, radix);
    const size_t width = std::max<size_t>(3U, upper_text.size());

    appendUnsignedTextPadded(text, 0U, radix, width);
    text.push_back('-');
    text += device;
    appendUnsignedTextPadded(text, upper_bound, radix, width);
    return text;
}

}  // namespace

const char* deviceRangeFamilyLabel(DeviceRangeFamily family) {
    return deviceRangeFamilyLabelImpl(family);
}

Error readDeviceRangeCatalogForFamily(SlmpClient& client, DeviceRangeFamily family, DeviceRangeCatalog& out) {
    const DeviceRangeProfileSpec* profile = findDeviceRangeProfile(family);
    if (profile == nullptr) return Error::InvalidArgument;

    std::vector<uint16_t> registers(profile->register_count);
    const Error read_error = client.readWords(dev::SD(dev::dec(profile->register_start)), profile->register_count, registers.data(), registers.size());
    if (read_error != Error::Ok) return read_error;

    out.model = deviceRangeFamilyLabelImpl(family);
    out.model_code = 0U;
    out.has_model_code = false;
    out.family = family;
    out.entries.clear();
    out.entries.reserve(40U);

    for (size_t row_index = 0; row_index < countOf(kDeviceRangeRows); ++row_index) {
        const DeviceRangeRowSpec& row = kDeviceRangeRows[row_index];
        const DeviceRangeRuleSpec* rule = findDeviceRangeRule(*profile, row.item);
        if (rule == nullptr) return Error::InvalidArgument;

        bool has_point_count = false;
        uint32_t point_count = 0U;
        const Error eval_error = evaluateDeviceRangePointCount(rule->value, registers, profile->register_start, has_point_count, point_count);
        if (eval_error != Error::Ok) return eval_error;

        const bool supported = rule->value.kind != DeviceRangeValueKind::Unsupported;
        const bool has_upper_bound = has_point_count && point_count > 0U;
        const uint32_t upper_bound = has_upper_bound ? (point_count - 1U) : 0U;

        for (size_t device_index = 0; device_index < row.device_count; ++device_index) {
            const DeviceRangeItemDevice& device = row.devices[device_index];
            DeviceRangeEntry entry;
            entry.device = device.name;
            entry.category = row.category;
            entry.is_bit_device = device.is_bit_device;
            entry.supported = supported;
            entry.lower_bound = 0U;
            entry.upper_bound = upper_bound;
            entry.has_upper_bound = has_upper_bound;
            entry.point_count = point_count;
            entry.has_point_count = has_point_count;
            entry.notation = resolveDeviceRangeNotation(family, device.name, row.notation);
            entry.address_range = formatDeviceRangeAddress(device.name, entry.notation, has_upper_bound, upper_bound);
            entry.source = rule->value.source != nullptr ? rule->value.source : "";
            entry.notes = rule->value.notes != nullptr ? rule->value.notes : "";
            out.entries.push_back(entry);
        }
    }

    return Error::Ok;
}

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

Error formatAddressSpec(const AddressSpec& spec, char* out, size_t out_size) {
    const DeviceMeta* meta = findDeviceMetaByCode(spec.device.code);
    if (meta == nullptr) return Error::UnsupportedDevice;
    if (spec.bit_index < -1 || spec.bit_index > 15) return Error::InvalidArgument;

    if (spec.bit_index >= 0) {
        if (meta->bit_unit || spec.type != ValueType::Bit) return Error::InvalidArgument;
    } else {
        ValueType normalized_type = spec.type;
        if (!normalizeRequestedType(*meta, normalized_type)) return Error::InvalidArgument;
        if (normalized_type != spec.type) return Error::InvalidArgument;
        if (!validateLongReadType(spec.device.code, spec.type)) return Error::InvalidArgument;
    }

    std::string formatted(meta->name);
    appendUnsignedText(formatted, spec.device.number, meta->radix);

    if (spec.bit_index >= 0) {
        formatted.push_back('.');
        appendUnsignedText(formatted, static_cast<uint32_t>(spec.bit_index), 16U);
        return copyTextToBuffer(formatted, out, out_size);
    }

    const ValueType default_type = defaultValueType(*meta);
    if (spec.explicit_type || spec.type != default_type) {
        const char* suffix = valueTypeSuffix(spec.type);
        if (suffix == nullptr) return Error::InvalidArgument;
        formatted.push_back(':');
        formatted += suffix;
    }

    return copyTextToBuffer(formatted, out, out_size);
}

Error normalizeAddress(const char* address, char* out, size_t out_size) {
    AddressSpec spec{};
    const Error err = parseAddressSpec(address, spec);
    if (err != Error::Ok) return err;
    return formatAddressSpec(spec, out, out_size);
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

    switch (resolveWriteRoute(spec)) {
        case WriteRoute::RandomBit: {
            const DeviceAddress devices[] = {spec.device};
            const bool values[] = {value.bit};
            return client.writeRandomBits(devices, values, 1);
        }
        case WriteRoute::Bit:
            return client.writeOneBit(spec.device, value.bit);
        case WriteRoute::RandomDword: {
            const DeviceAddress devices[] = {spec.device};
            const uint32_t values[] = {encodeDwordValue(value)};
            return client.writeRandomWords(nullptr, nullptr, 0, devices, values, 1);
        }
        case WriteRoute::Dword:
            return client.writeOneDWord(spec.device, encodeDwordValue(value));
        default:
            return client.writeOneWord(spec.device, encodeWordValue(value));
    }
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
    switch (resolveWriteRoute(spec)) {
        case WriteRoute::RandomBit: {
            const DeviceAddress devices[] = {spec.device};
            const bool values[] = {value.bit};
            return client.writeRandomBits(devices, values, 1);
        }
        case WriteRoute::Bit:
            return client.writeOneBit(spec.device, value.bit);
        case WriteRoute::RandomDword: {
            const DeviceAddress devices[] = {spec.device};
            const uint32_t values[] = {encodeDwordValue(value)};
            return client.writeRandomWords(nullptr, nullptr, 0, devices, values, 1);
        }
        case WriteRoute::Dword:
            return client.writeOneDWord(spec.device, encodeDwordValue(value));
        default:
            return client.writeOneWord(spec.device, encodeWordValue(value));
    }
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
