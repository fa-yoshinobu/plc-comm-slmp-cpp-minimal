/**
 * @file slmp_minimal.cpp
 * @brief Lightweight SLMP client implementation.
 */

#include "slmp_minimal.h"

#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
/**
 * @internal
 * @brief Mock millis() implementation for non-Arduino environments.
 */
static uint32_t millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
#endif

namespace slmp {

/**
 * @internal
 * @brief Helper to get current system time in milliseconds.
 */
static uint32_t getTimeMs() {
    return millis();
}

namespace {

constexpr uint8_t kRequestSubheader4E0 = 0x54;
constexpr uint8_t kRequestSubheader4E1 = 0x00;
constexpr uint8_t kResponseSubheader4E0 = 0xD4;
constexpr uint8_t kResponseSubheader4E1 = 0x00;

constexpr uint8_t kRequestSubheader3E0 = 0x50;
constexpr uint8_t kRequestSubheader3E1 = 0x00;
constexpr uint8_t kResponseSubheader3E0 = 0xD0;
constexpr uint8_t kResponseSubheader3E1 = 0x00;

constexpr uint16_t kCommandReadTypeName = 0x0101;
constexpr uint16_t kCommandDeviceRead = 0x0401;
constexpr uint16_t kCommandDeviceWrite = 0x1401;
constexpr uint16_t kCommandDeviceReadRandom = 0x0403;
constexpr uint16_t kCommandDeviceWriteRandom = 0x1402;
constexpr uint16_t kCommandDeviceReadBlock = 0x0406;
constexpr uint16_t kCommandDeviceWriteBlock = 0x1406;
constexpr uint16_t kCommandRemoteRun = 0x1001;
constexpr uint16_t kCommandRemoteStop = 0x1002;
constexpr uint16_t kCommandRemotePause = 0x1003;
constexpr uint16_t kCommandRemoteLatchClear = 0x1005;
constexpr uint16_t kCommandRemoteReset = 0x1006;
constexpr uint16_t kCommandSelfTest = 0x0619;
constexpr uint16_t kCommandClearError = 0x1617;
constexpr uint16_t kCommandRemotePasswordUnlock = 0x1630;
constexpr uint16_t kCommandRemotePasswordLock = 0x1631;
constexpr uint16_t kCommandMemoryRead = 0x0613;
constexpr uint16_t kCommandMemoryWrite = 0x1613;
constexpr uint16_t kCommandExtendUnitRead = 0x0601;
constexpr uint16_t kCommandExtendUnitWrite = 0x1601;
constexpr uint16_t kCommandLabelArrayRead = 0x041A;
constexpr uint16_t kCommandLabelArrayWrite = 0x141A;
constexpr uint16_t kCommandLabelRandomRead = 0x041C;
constexpr uint16_t kCommandLabelRandomWrite = 0x141B;
constexpr uint16_t kCommandMonitorRegister = 0x0801;
constexpr uint16_t kCommandMonitorExecute = 0x0802;

constexpr uint16_t kSubcommandWord = 0x0002;
constexpr uint16_t kSubcommandBit = 0x0003;

constexpr size_t kRequestHeaderSize4E = 19;
constexpr size_t kResponsePrefixSize4E = 13;
constexpr size_t kResponseHeaderWithoutDataSize4E = 15;

constexpr size_t kRequestHeaderSize3E = 15;
constexpr size_t kResponsePrefixSize3E = 9;
constexpr size_t kResponseHeaderWithoutDataSize3E = 11;

inline void writeLe16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFF);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

inline void writeLe32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFF);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

inline uint16_t readLe16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

inline uint32_t readLe32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

inline uint32_t floatToBits(float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

inline float bitsToFloat(uint32_t bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

inline size_t packedBitBytes(size_t bit_count) {
    return (bit_count + 1U) / 2U;
}

inline size_t encodeDeviceSpec(const DeviceAddress& device, CompatibilityMode mode, uint8_t* out, size_t capacity) {
    if (mode == CompatibilityMode::Legacy) {
        if (capacity < 4) return 0;
        out[0] = (uint8_t)(device.number & 0xFFU);
        out[1] = (uint8_t)((device.number >> 8U) & 0xFFU);
        out[2] = (uint8_t)((device.number >> 16U) & 0xFFU);
        out[3] = (uint8_t)device.code;
        return 4;
    } else {
        if (capacity < 6) return 0;
        writeLe32(out, device.number);
        writeLe16(out + 4, static_cast<uint16_t>(device.code));
        return 6;
    }
}

inline bool isUnsupportedDirectDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::G:   // access via readWordsModuleBuf / writeWordsModuleBuf
        case DeviceCode::HG:  // access via readWordsModuleBuf / writeWordsModuleBuf
            return true;
        default:
            return false;
    }
}

inline bool isLongTimerStateReadOnlyDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::LTS:
        case DeviceCode::LTC:
        case DeviceCode::LSTS:
        case DeviceCode::LSTC:
            return true;
        default:
            return false;
    }
}

inline bool isLongTimerCurrentBlockDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::LTN:
        case DeviceCode::LSTN:
            return true;
        default:
            return false;
    }
}

inline bool isLongCounterContactDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::LCS:
        case DeviceCode::LCC:
            return true;
        default:
            return false;
    }
}

inline Error validateDirectWordReadDevice(const DeviceAddress& device, uint16_t points) {
    if (isUnsupportedDirectDevice(device.code)) {
        return Error::UnsupportedDevice;
    }
    if (isLongTimerCurrentBlockDevice(device.code) && (points == 0U || (points % 4U) != 0U)) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectBitReadDevice(const DeviceAddress& device) {
    if (isUnsupportedDirectDevice(device.code) || isLongTimerStateReadOnlyDevice(device.code)) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectDWordReadDevice(const DeviceAddress& device) {
    if (isUnsupportedDirectDevice(device.code) || isLongTimerCurrentBlockDevice(device.code)) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectDeviceList(const DeviceAddress* devices, size_t count) {
    if (count == 0) {
        return Error::Ok;
    }
    if (devices == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (isUnsupportedDirectDevice(devices[i].code)) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline Error summarizeBlockReadList(const DeviceBlockRead* blocks, size_t count, size_t& total_points) {
    total_points = 0;
    if (count == 0) {
        return Error::Ok;
    }
    if (blocks == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (blocks[i].points == 0) {
            return Error::InvalidArgument;
        }
        if (isUnsupportedDirectDevice(blocks[i].device.code) || isLongCounterContactDevice(blocks[i].device.code)) {
            return Error::UnsupportedDevice;
        }
        total_points += blocks[i].points;
    }
    return Error::Ok;
}

inline Error summarizeBlockWriteList(const DeviceBlockWrite* blocks, size_t count, size_t& total_points) {
    total_points = 0;
    if (count == 0) {
        return Error::Ok;
    }
    if (blocks == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (blocks[i].points == 0 || blocks[i].values == nullptr) {
            return Error::InvalidArgument;
        }
        if (isUnsupportedDirectDevice(blocks[i].device.code) || isLongCounterContactDevice(blocks[i].device.code)) {
            return Error::UnsupportedDevice;
        }
        total_points += blocks[i].points;
    }
    return Error::Ok;
}

inline Error encodeRemotePasswordPayload(
    const char* password,
    uint8_t* out,
    size_t capacity,
    size_t& payload_length
) {
    payload_length = 0;
    if (password == nullptr) {
        return Error::InvalidArgument;
    }
    if (out == nullptr || capacity < 8U) {
        return Error::BufferTooSmall;
    }

    size_t password_length = 0;
    while (password[password_length] != '\0') {
        unsigned char ch = static_cast<unsigned char>(password[password_length]);
        if (ch > 0x7FU) {
            return Error::InvalidArgument;
        }
        ++password_length;
        if (password_length > 32U) {
            return Error::InvalidArgument;
        }
    }

    if (password_length < 6U || capacity < (2U + password_length)) {
        return password_length < 6U ? Error::InvalidArgument : Error::BufferTooSmall;
    }

    writeLe16(out, static_cast<uint16_t>(password_length));
    memcpy(out + 2, password, password_length);
    payload_length = 2U + password_length;
    return Error::Ok;
}

inline Error encodeRemoteModePayload(uint16_t mode, uint8_t* out, size_t capacity, size_t& payload_length) {
    payload_length = 0;
    if (out == nullptr || capacity < 2U) {
        return Error::BufferTooSmall;
    }
    writeLe16(out, mode);
    payload_length = 2U;
    return Error::Ok;
}

inline Error encodeRemoteRunPayload(
    bool force,
    uint16_t clear_mode,
    uint8_t* out,
    size_t capacity,
    size_t& payload_length
) {
    payload_length = 0;
    if (clear_mode > 2U) {
        return Error::InvalidArgument;
    }
    if (out == nullptr || capacity < 4U) {
        return Error::BufferTooSmall;
    }
    const uint16_t mode = force ? 0x0003U : 0x0001U;
    writeLe16(out, mode);
    writeLe16(out + 2, clear_mode);
    payload_length = 4U;
    return Error::Ok;
}

inline Error encodeSelfTestPayload(
    const uint8_t* data,
    size_t data_length,
    uint8_t* out,
    size_t capacity,
    size_t& payload_length
) {
    payload_length = 0;
    if (data == nullptr || data_length == 0U || data_length > 960U) {
        return Error::InvalidArgument;
    }
    if (out == nullptr || capacity < (2U + data_length)) {
        return Error::BufferTooSmall;
    }
    writeLe16(out, static_cast<uint16_t>(data_length));
    memcpy(out + 2, data, data_length);
    payload_length = 2U + data_length;
    return Error::Ok;
}

inline bool isMixedWriteRetryEndCode(uint16_t end_code) {
    switch (end_code) {
        case 0xC056:
        case 0xC05B:
        case 0xC061:
            return true;
        default:
            return false;
    }
}

inline char asciiUpper(char ch) {
    return (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
}

/**
 * @internal
 * @brief Encode a link direct device spec (Jx\device).
 * Format (always 11 bytes, always QL 3-byte number): 0x00 0x00 + dev_no(3LE) + dev_code(1) + 0x00 0x00 + j_net(1) + 0x00 + 0xF9
 */
inline size_t encodeLinkDirectDeviceSpec(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint8_t* out, size_t capacity) {
    if (capacity < 11) return 0;
    out[0] = 0x00; out[1] = 0x00;
    out[2] = (uint8_t)(dev_no & 0xFFU);
    out[3] = (uint8_t)((dev_no >> 8U) & 0xFFU);
    out[4] = (uint8_t)((dev_no >> 16U) & 0xFFU);
    out[5] = (uint8_t)((uint16_t)code & 0xFFU);
    out[6] = 0x00; out[7] = 0x00;
    out[8] = j_net;
    out[9] = 0x00;
    out[10] = 0xF9;
    return 11;
}

/**
 * @internal
 * @brief Encode a module buffer device spec (Ux\G or Ux\HG).
 * Uses the "capture-aligned" G/HG layout verified by GOT pcap.
 * Format: ext_spec_mod(1) + dev_mod_idx(1) + device_spec(4/6) + dev_mod_flags(1) + 0x00(1) + slot(2LE) + DM(1)
 * DM = 0xF8 for G, 0xFA for HG.
 */
inline size_t encodeModuleBufDeviceSpec(uint16_t slot, bool use_hg, uint32_t dev_no, CompatibilityMode mode, uint8_t* out, size_t capacity) {
    size_t device_spec_size = (mode == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t total = 2U + device_spec_size + 2U + 2U + 1U;  // 11 (Legacy) or 13 (iQR)
    if (capacity < total) return 0;
    DeviceAddress dev{use_hg ? DeviceCode::HG : DeviceCode::G, dev_no};
    size_t offset = 0;
    out[offset++] = 0x00;  // ext_spec_mod
    out[offset++] = 0x00;  // dev_mod_idx
    size_t written = encodeDeviceSpec(dev, mode, out + offset, capacity - offset);
    if (written == 0) return 0;
    offset += written;
    out[offset++] = 0x00;  // dev_mod_flags
    out[offset++] = 0x00;  // reserved
    writeLe16(out + offset, slot);
    offset += 2U;
    out[offset++] = use_hg ? 0xFAU : 0xF8U;  // DM
    return offset;
}

/**
 * @internal
 * @brief Append a UTF-16-LE label name (char_count LE16 + raw UTF-16-LE bytes) into a buffer.
 * Returns bytes written, or 0 on insufficient capacity.
 */
inline size_t appendLabelName(const LabelName& name, uint8_t* out, size_t capacity) {
    size_t needed = 2U + static_cast<size_t>(name.length) * 2U;
    if (capacity < needed) return 0;
    writeLe16(out, name.length);
    for (uint16_t i = 0; i < name.length; ++i) {
        writeLe16(out + 2U + static_cast<size_t>(i) * 2U, name.chars[i]);
    }
    return needed;
}

/**
 * @internal
 * @brief Encode an ExtDeviceSpec into a buffer using the appropriate encoder.
 * Returns bytes written, or 0 on failure.
 */
inline size_t encodeExtDeviceSpec(const ExtDeviceSpec& spec, CompatibilityMode mode, uint8_t* out, size_t capacity) {
    if (spec.kind == ExtDeviceSpec::Kind::ModuleBuf) {
        return encodeModuleBufDeviceSpec(spec.mod.slot, spec.mod.use_hg, spec.mod.dev_no, mode, out, capacity);
    } else {
        return encodeLinkDirectDeviceSpec(spec.link.j_net, spec.link.code, spec.link.dev_no, out, capacity);
    }
}

}  // namespace

SlmpClient::SlmpClient(
    ITransport& transport,
    uint8_t* tx_buffer,
    size_t tx_capacity,
    uint8_t* rx_buffer,
    size_t rx_capacity
)
    : transport_(transport),
      tx_buffer_(tx_buffer),
      tx_capacity_(tx_capacity),
      rx_buffer_(rx_buffer),
      rx_capacity_(rx_capacity),
      target_(),
      frame_type_(FrameType::Frame4E),
      compatibility_mode_(CompatibilityMode::iQR),
      monitoring_timer_(0x0010),
      timeout_ms_(3000),
      serial_(0),
      last_error_(Error::Ok),
      last_end_code_(0),
      last_request_length_(0),
      last_response_length_(0),
      state_(State::Idle),
      bytes_transferred_(0),
      last_activity_ms_(0),
      async_ctx_{} {}

/** @brief Returns true if an asynchronous operation is currently active. */
bool SlmpClient::isBusy() const {
    return state_ != State::Idle;
}

bool SlmpClient::connect(const char* host, uint16_t port) {
    if (host == nullptr || port == 0) {
        setError(Error::InvalidArgument);
        return false;
    }
    if (!transport_.connect(host, port)) {
        setError(Error::TransportError);
        return false;
    }
    setError(Error::Ok);
    return true;
}

void SlmpClient::close() {
    transport_.close();
}

bool SlmpClient::connected() const {
    return transport_.connected();
}

void SlmpClient::setTarget(const TargetAddress& target) {
    target_ = target;
}

const TargetAddress& SlmpClient::target() const {
    return target_;
}

void SlmpClient::setFrameType(FrameType frame_type) {
    frame_type_ = frame_type;
}

FrameType SlmpClient::frameType() const {
    return frame_type_;
}

void SlmpClient::setCompatibilityMode(CompatibilityMode mode) {
    compatibility_mode_ = mode;
}

CompatibilityMode SlmpClient::compatibilityMode() const {
    return compatibility_mode_;
}

void SlmpClient::setMonitoringTimer(uint16_t monitoring_timer) {
    monitoring_timer_ = monitoring_timer;
}

uint16_t SlmpClient::monitoringTimer() const {
    return monitoring_timer_;
}

void SlmpClient::setTimeoutMs(uint32_t timeout_ms) {
    timeout_ms_ = timeout_ms;
}

uint32_t SlmpClient::timeoutMs() const {
    return timeout_ms_;
}

Error SlmpClient::lastError() const {
    return last_error_;
}

uint16_t SlmpClient::lastEndCode() const {
    return last_end_code_;
}

const uint8_t* SlmpClient::lastRequestFrame() const {
    return tx_buffer_;
}

size_t SlmpClient::lastRequestFrameLength() const {
    return last_request_length_;
}

const uint8_t* SlmpClient::lastResponseFrame() const {
    return rx_buffer_;
}

size_t SlmpClient::lastResponseFrameLength() const {
    return last_response_length_;
}

Error SlmpClient::startAsync(AsyncContext::Type type, size_t payload_length, uint32_t now_ms) {
    if (isBusy()) {
        setError(Error::Busy);
        return last_error_;
    }
    if (!transport_.connected()) {
        setError(Error::NotConnected);
        return last_error_;
    }

    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    uint16_t command = 0;
    uint16_t subcommand = 0;

    const uint16_t subcommand_word = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0000 : kSubcommandWord;
    const uint16_t subcommand_bit = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0001 : kSubcommandBit;

    switch (type) {
        case AsyncContext::Type::ReadTypeName:
            command = kCommandReadTypeName;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadWords:
        case AsyncContext::Type::ReadDWords:
        case AsyncContext::Type::ReadFloat32s:
        case AsyncContext::Type::ReadBits:
            command = kCommandDeviceRead;
            subcommand = (type == AsyncContext::Type::ReadBits) ? subcommand_bit : subcommand_word;
            break;
        case AsyncContext::Type::WriteWords:
        case AsyncContext::Type::WriteDWords:
        case AsyncContext::Type::WriteFloat32s:
        case AsyncContext::Type::WriteBits:
            command = kCommandDeviceWrite;
            subcommand = (type == AsyncContext::Type::WriteBits) ? subcommand_bit : subcommand_word;
            break;
        case AsyncContext::Type::ReadRandom:
            command = kCommandDeviceReadRandom;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::WriteRandomWords:
            command = kCommandDeviceWriteRandom;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::WriteRandomBits:
            command = kCommandDeviceWriteRandom;
            subcommand = subcommand_bit;
            break;
        case AsyncContext::Type::ReadBlock:
            command = kCommandDeviceReadBlock;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::WriteBlock:
            command = kCommandDeviceWriteBlock;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::RemoteRun:
            command = kCommandRemoteRun;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::RemoteStop:
            command = kCommandRemoteStop;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::RemotePause:
            command = kCommandRemotePause;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::RemoteLatchClear:
            command = kCommandRemoteLatchClear;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::RemoteReset:
            command = kCommandRemoteReset;
            subcommand = async_ctx_.data.remoteReset.subcommand;
            break;
        case AsyncContext::Type::SelfTest:
            command = kCommandSelfTest;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ClearError:
            command = kCommandClearError;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::PasswordUnlock:
            command = kCommandRemotePasswordUnlock;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::PasswordLock:
            command = kCommandRemotePasswordLock;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadLongTimer:
            command = kCommandDeviceRead;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::ReadModuleBufWords:
            command = kCommandDeviceRead;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0080U : 0x0082U;
            break;
        case AsyncContext::Type::WriteModuleBufWords:
            command = kCommandDeviceWrite;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0080U : 0x0082U;
            break;
        case AsyncContext::Type::ReadModuleBufBits:
            command = kCommandDeviceRead;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0081U : 0x0083U;
            break;
        case AsyncContext::Type::WriteModuleBufBits:
            command = kCommandDeviceWrite;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0081U : 0x0083U;
            break;
        case AsyncContext::Type::ReadLinkDirectWords:
            command = kCommandDeviceRead;
            subcommand = 0x0080U;
            break;
        case AsyncContext::Type::WriteWordsLinkDirect:
            command = kCommandDeviceWrite;
            subcommand = 0x0080U;
            break;
        case AsyncContext::Type::ReadLinkDirectBits:
            command = kCommandDeviceRead;
            subcommand = 0x0081U;
            break;
        case AsyncContext::Type::WriteBitsLinkDirect:
            command = kCommandDeviceWrite;
            subcommand = 0x0081U;
            break;
        case AsyncContext::Type::ReadMemoryWords:
            command = kCommandMemoryRead;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::WriteMemoryWords:
            command = kCommandMemoryWrite;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadExtendUnitBytes:
            command = kCommandExtendUnitRead;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::WriteExtendUnitBytes:
            command = kCommandExtendUnitWrite;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadArrayLabels:
            command = kCommandLabelArrayRead;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::WriteArrayLabels:
            command = kCommandLabelArrayWrite;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadRandomLabels:
            command = kCommandLabelRandomRead;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::WriteRandomLabels:
            command = kCommandLabelRandomWrite;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadRandomExt:
        case AsyncContext::Type::WriteRandomWordsExt:
            command = (type == AsyncContext::Type::ReadRandomExt) ? kCommandDeviceReadRandom : kCommandDeviceWriteRandom;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0080U : 0x0082U;
            break;
        case AsyncContext::Type::WriteRandomBitsExt:
            command = kCommandDeviceWriteRandom;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0081U : 0x0083U;
            break;
        case AsyncContext::Type::RegisterMonitorDevices:
            command = kCommandMonitorRegister;
            subcommand = subcommand_word;
            break;
        case AsyncContext::Type::RegisterMonitorDevicesExt:
            command = kCommandMonitorRegister;
            subcommand = (compatibility_mode_ == CompatibilityMode::Legacy) ? 0x0080U : 0x0082U;
            break;
        case AsyncContext::Type::RunMonitorCycle:
            command = kCommandMonitorExecute;
            subcommand = 0x0000;
            break;
        default:
            setError(Error::InvalidArgument);
            return last_error_;
    }

    uint16_t serial = serial_;
    serial_ = static_cast<uint16_t>(serial_ + 1U);

    if (payload_length > 0) {
        memmove(tx_buffer_ + request_header_size, tx_buffer_, payload_length);
    }

    if (frame_type_ == FrameType::Frame4E) {
        tx_buffer_[0] = kRequestSubheader4E0;
        tx_buffer_[1] = kRequestSubheader4E1;
        writeLe16(tx_buffer_ + 2, serial);
        tx_buffer_[4] = 0x00;
        tx_buffer_[5] = 0x00;
        tx_buffer_[6] = target_.network;
        tx_buffer_[7] = target_.station;
        writeLe16(tx_buffer_ + 8, target_.module_io);
        tx_buffer_[10] = target_.multidrop;
        writeLe16(tx_buffer_ + 11, static_cast<uint16_t>(6U + payload_length));
        writeLe16(tx_buffer_ + 13, monitoring_timer_);
        writeLe16(tx_buffer_ + 15, command);
        writeLe16(tx_buffer_ + 17, subcommand);
    } else {
        tx_buffer_[0] = kRequestSubheader3E0;
        tx_buffer_[1] = kRequestSubheader3E1;
        tx_buffer_[2] = target_.network;
        tx_buffer_[3] = target_.station;
        writeLe16(tx_buffer_ + 4, target_.module_io);
        tx_buffer_[6] = target_.multidrop;
        writeLe16(tx_buffer_ + 7, static_cast<uint16_t>(6U + payload_length));
        writeLe16(tx_buffer_ + 9, monitoring_timer_);
        writeLe16(tx_buffer_ + 11, command);
        writeLe16(tx_buffer_ + 13, subcommand);
    }

    last_request_length_ = request_header_size + payload_length;
    state_ = State::Sending;
    bytes_transferred_ = 0;
    last_activity_ms_ = now_ms;
    async_ctx_.type = type;

    setError(Error::Ok);
    return last_error_;
}

void SlmpClient::update(uint32_t now_ms) {
    if (state_ == State::Idle) return;

    if (now_ms - last_activity_ms_ >= timeout_ms_) {
        setError(Error::TransportError);
        state_ = State::Idle;
        return;
    }

    size_t response_prefix_size = (frame_type_ == FrameType::Frame4E) ? kResponsePrefixSize4E : kResponsePrefixSize3E;

    if (state_ == State::Sending) {
        size_t written = transport_.write(tx_buffer_ + bytes_transferred_, last_request_length_ - bytes_transferred_);
        if (written > 0) {
            bytes_transferred_ += written;
            last_activity_ms_ = now_ms;
            if (bytes_transferred_ == last_request_length_) {
                if (async_ctx_.type == AsyncContext::Type::RemoteReset &&
                    !async_ctx_.data.remoteReset.expect_response) {
                    state_ = State::Idle;
                    setError(Error::Ok);
                    return;
                }
                state_ = State::ReceivingPrefix;
                bytes_transferred_ = 0;
            }
        }
    } else if (state_ == State::ReceivingPrefix) {
        size_t read = transport_.read(rx_buffer_ + bytes_transferred_, response_prefix_size - bytes_transferred_);
        if (read > 0) {
            bytes_transferred_ += read;
            last_activity_ms_ = now_ms;
            if (bytes_transferred_ == response_prefix_size) {
                uint16_t response_data_length = 0;
                if (frame_type_ == FrameType::Frame4E) {
                    if (rx_buffer_[0] != kResponseSubheader4E0 || rx_buffer_[1] != kResponseSubheader4E1) {
                        setError(Error::ProtocolError);
                        state_ = State::Idle;
                        return;
                    }
                    uint16_t serial = (frame_type_ == FrameType::Frame4E) ? readLe16(tx_buffer_ + 2) : 0;
                    if (readLe16(rx_buffer_ + 2) != serial) {
                        setError(Error::ProtocolError);
                        state_ = State::Idle;
                        return;
                    }
                    response_data_length = readLe16(rx_buffer_ + 11);
                } else {
                    if (rx_buffer_[0] != kResponseSubheader3E0 || rx_buffer_[1] != kResponseSubheader3E1) {
                        setError(Error::ProtocolError);
                        state_ = State::Idle;
                        return;
                    }
                    response_data_length = readLe16(rx_buffer_ + 7);
                }

                if (response_data_length < 2U) {
                    setError(Error::ProtocolError);
                    state_ = State::Idle;
                    return;
                }
                last_response_length_ = response_prefix_size + response_data_length;
                if (rx_capacity_ < last_response_length_) {
                    setError(Error::BufferTooSmall);
                    state_ = State::Idle;
                    return;
                }
                state_ = State::ReceivingBody;
                bytes_transferred_ = 0;
            }
        }
    } else if (state_ == State::ReceivingBody) {
        size_t to_read = last_response_length_ - (response_prefix_size + bytes_transferred_);
        size_t read = transport_.read(rx_buffer_ + response_prefix_size + bytes_transferred_, to_read);
        if (read > 0) {
            bytes_transferred_ += read;
            last_activity_ms_ = now_ms;
            if (response_prefix_size + bytes_transferred_ == last_response_length_) {
                state_ = State::Idle;
                completeAsync();
            }
        }
    }
}

void SlmpClient::completeAsync() {
    size_t response_prefix_size = (frame_type_ == FrameType::Frame4E) ? kResponsePrefixSize4E : kResponsePrefixSize3E;
    uint16_t end_code = readLe16(rx_buffer_ + response_prefix_size);
    if (async_ctx_.type == AsyncContext::Type::WriteBlock) {
        const AsyncContext::WriteBlockStage stage = async_ctx_.data.writeBlock.stage;
        const bool has_mixed_blocks = async_ctx_.data.writeBlock.has_mixed_blocks;
        const BlockWriteOptions options = async_ctx_.data.writeBlock.options;
        const DeviceBlockWrite* word_blocks = async_ctx_.data.writeBlock.word_blocks;
        const size_t word_block_count = async_ctx_.data.writeBlock.word_block_count;
        const DeviceBlockWrite* bit_blocks = async_ctx_.data.writeBlock.bit_blocks;
        const size_t bit_block_count = async_ctx_.data.writeBlock.bit_block_count;

        if (end_code != 0) {
            if (stage == AsyncContext::WriteBlockStage::Direct && has_mixed_blocks && options.retry_mixed_on_error &&
                isMixedWriteRetryEndCode(end_code)) {
                beginWriteBlockRequest(
                    word_blocks,
                    word_block_count,
                    nullptr,
                    0,
                    word_blocks,
                    word_block_count,
                    bit_blocks,
                    bit_block_count,
                    options,
                    AsyncContext::WriteBlockStage::SplitWord,
                    true,
                    last_activity_ms_
                );
                return;
            }
            setError(Error::PlcError, end_code);
            return;
        }

        if (stage == AsyncContext::WriteBlockStage::SplitWord && bit_block_count > 0U) {
            beginWriteBlockRequest(
                nullptr,
                0,
                bit_blocks,
                bit_block_count,
                word_blocks,
                word_block_count,
                bit_blocks,
                bit_block_count,
                options,
                AsyncContext::WriteBlockStage::SplitBit,
                true,
                last_activity_ms_
            );
            return;
        }
    } else if (end_code != 0) {
        setError(Error::PlcError, end_code);
        return;
    }

    const uint8_t* response_data = rx_buffer_ + response_prefix_size + 2U;
    size_t response_length = last_response_length_ - (response_prefix_size + 2U);

    switch (async_ctx_.type) {
        case AsyncContext::Type::ReadTypeName: {
            TypeNameInfo& out = *async_ctx_.data.readTypeName.out;
            size_t model_length = response_length < 16 ? response_length : 16;
            memcpy(out.model, response_data, model_length);
            out.model[model_length] = '\0';
            while (model_length > 0 && (out.model[model_length - 1] == ' ' || out.model[model_length - 1] == '\0')) {
                out.model[model_length - 1] = '\0';
                --model_length;
            }
            if (response_length >= 18) {
                out.model_code = readLe16(response_data + 16);
                out.has_model_code = true;
            }
            break;
        }
        case AsyncContext::Type::ReadWords: {
            uint16_t* values = static_cast<uint16_t*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            if (response_length != static_cast<size_t>(points) * 2U) {
                setError(Error::ProtocolError);
                return;
            }
            for (uint16_t i = 0; i < points; ++i) {
                values[i] = readLe16(response_data + (static_cast<size_t>(i) * 2U));
            }
            break;
        }
        case AsyncContext::Type::ReadDWords: {
            uint32_t* values = static_cast<uint32_t*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            if (response_length != static_cast<size_t>(points) * 4U) {
                setError(Error::ProtocolError);
                return;
            }
            for (uint16_t i = 0; i < points; ++i) {
                values[i] = readLe32(response_data + (static_cast<size_t>(i) * 4U));
            }
            break;
        }
        case AsyncContext::Type::ReadFloat32s: {
            float* values = static_cast<float*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            if (response_length != static_cast<size_t>(points) * 4U) {
                setError(Error::ProtocolError);
                return;
            }
            for (uint16_t i = 0; i < points; ++i) {
                values[i] = bitsToFloat(readLe32(response_data + (static_cast<size_t>(i) * 4U)));
            }
            break;
        }
        case AsyncContext::Type::ReadBits: {
            bool* values = static_cast<bool*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            size_t expected_bytes = packedBitBytes(points);
            if (response_length < expected_bytes) {
                setError(Error::ProtocolError);
                return;
            }
            size_t bit_index = 0;
            for (size_t i = 0; i < expected_bytes && bit_index < points; ++i) {
                values[bit_index++] = ((response_data[i] >> 4) & 0x1U) != 0;
                if (bit_index < points) {
                    values[bit_index++] = (response_data[i] & 0x1U) != 0;
                }
            }
            break;
        }
        case AsyncContext::Type::ReadRandom: {
            uint16_t* word_values = async_ctx_.data.readRandom.word_values;
            uint16_t word_count = async_ctx_.data.readRandom.word_count;
            uint32_t* dword_values = async_ctx_.data.readRandom.dword_values;
            uint16_t dword_count = async_ctx_.data.readRandom.dword_count;
            size_t expected_length = (word_count * 2U) + (dword_count * 4U);
            if (response_length != expected_length) {
                setError(Error::ProtocolError);
                return;
            }
            size_t offset = 0;
            for (size_t i = 0; i < word_count; ++i) {
                word_values[i] = readLe16(response_data + offset);
                offset += 2U;
            }
            for (size_t i = 0; i < dword_count; ++i) {
                dword_values[i] = readLe32(response_data + offset);
                offset += 4U;
            }
            break;
        }
        case AsyncContext::Type::ReadBlock: {
            uint16_t* word_values = async_ctx_.data.readBlock.word_values;
            size_t total_word_points = async_ctx_.data.readBlock.total_word_points;
            uint16_t* bit_values = async_ctx_.data.readBlock.bit_values;
            size_t total_bit_points = async_ctx_.data.readBlock.total_bit_points;
            size_t expected_length = (total_word_points + total_bit_points) * 2U;
            if (response_length != expected_length) {
                setError(Error::ProtocolError);
                return;
            }
            size_t offset = 0;
            for (size_t i = 0; i < total_word_points; ++i) {
                word_values[i] = readLe16(response_data + offset);
                offset += 2U;
            }
            for (size_t i = 0; i < total_bit_points; ++i) {
                bit_values[i] = readLe16(response_data + offset);
                offset += 2U;
            }
            break;
        }
        case AsyncContext::Type::SelfTest: {
            if (response_length < 2U) {
                setError(Error::ProtocolError);
                return;
            }
            const size_t loopback_size = readLe16(response_data);
            if (response_length != (2U + loopback_size)) {
                setError(Error::ProtocolError);
                return;
            }
            if (async_ctx_.data.selfTest.out == nullptr || async_ctx_.data.selfTest.out_length == nullptr ||
                async_ctx_.data.selfTest.out_capacity < loopback_size) {
                setError(Error::InvalidArgument);
                return;
            }
            memcpy(async_ctx_.data.selfTest.out, response_data + 2U, loopback_size);
            *async_ctx_.data.selfTest.out_length = loopback_size;
            break;
        }
        case AsyncContext::Type::ReadLongTimer: {
            LongTimerResult* out = async_ctx_.data.readLongTimer.out;
            uint16_t points = async_ctx_.data.readLongTimer.points;
            size_t expected = static_cast<size_t>(points) * 8U;  // 4 words * 2 bytes each
            if (response_length < expected) {
                setError(Error::ProtocolError);
                return;
            }
            for (uint16_t i = 0; i < points; ++i) {
                size_t base = static_cast<size_t>(i) * 8U;
                uint32_t lo = readLe16(response_data + base);
                uint32_t hi = readLe16(response_data + base + 2U);
                uint16_t status = readLe16(response_data + base + 4U);
                out[i].current_value = (hi << 16U) | lo;
                out[i].status_word = status;
                out[i].contact = (status & 0x0002U) != 0;
                out[i].coil    = (status & 0x0001U) != 0;
            }
            break;
        }
        case AsyncContext::Type::ReadModuleBufWords:
        case AsyncContext::Type::ReadLinkDirectWords:
        case AsyncContext::Type::ReadMemoryWords: {
            uint16_t* values = static_cast<uint16_t*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            if (response_length != static_cast<size_t>(points) * 2U) {
                setError(Error::ProtocolError);
                return;
            }
            for (uint16_t i = 0; i < points; ++i) {
                values[i] = readLe16(response_data + static_cast<size_t>(i) * 2U);
            }
            break;
        }
        case AsyncContext::Type::ReadModuleBufBits:
        case AsyncContext::Type::ReadLinkDirectBits: {
            bool* values = static_cast<bool*>(async_ctx_.data.common.values);
            uint16_t points = async_ctx_.data.common.points;
            size_t expected_bytes = packedBitBytes(points);
            if (response_length < expected_bytes) {
                setError(Error::ProtocolError);
                return;
            }
            size_t bit_index = 0;
            for (size_t i = 0; i < expected_bytes && bit_index < points; ++i) {
                values[bit_index++] = ((response_data[i] >> 4) & 0x1U) != 0;
                if (bit_index < points) {
                    values[bit_index++] = (response_data[i] & 0x1U) != 0;
                }
            }
            break;
        }
        case AsyncContext::Type::ReadExtendUnitBytes: {
            uint8_t* out = static_cast<uint8_t*>(async_ctx_.data.common.values);
            uint16_t byte_length = async_ctx_.data.common.points;
            if (response_length < byte_length) {
                setError(Error::ProtocolError);
                return;
            }
            memcpy(out, response_data, byte_length);
            break;
        }
        case AsyncContext::Type::ReadArrayLabels: {
            LabelArrayReadResult* out = async_ctx_.data.readArrayLabels.out;
            size_t capacity = async_ctx_.data.readArrayLabels.capacity;
            size_t* out_count = async_ctx_.data.readArrayLabels.out_count;
            if (response_length < 2U) {
                setError(Error::ProtocolError);
                return;
            }
            uint16_t count = readLe16(response_data);
            if (count > capacity) {
                setError(Error::BufferTooSmall);
                return;
            }
            size_t offset = 2U;
            for (uint16_t i = 0; i < count; ++i) {
                if (offset + 4U > response_length) {
                    setError(Error::ProtocolError);
                    return;
                }
                out[i].dt_id = response_data[offset];
                out[i].unit_specification = response_data[offset + 1U];
                out[i].array_data_length = readLe16(response_data + offset + 2U);
                offset += 4U;
                size_t data_bytes = (out[i].unit_specification == 0)
                    ? static_cast<size_t>(out[i].array_data_length) * 2U
                    : static_cast<size_t>(out[i].array_data_length);
                if (offset + data_bytes > response_length) {
                    setError(Error::ProtocolError);
                    return;
                }
                out[i].data = response_data + offset;
                out[i].data_bytes = data_bytes;
                offset += data_bytes;
            }
            async_ctx_.data.readArrayLabels.count = count;
            if (out_count != nullptr) *out_count = count;
            break;
        }
        case AsyncContext::Type::ReadRandomLabels: {
            LabelRandomReadResult* out = async_ctx_.data.readRandomLabels.out;
            size_t capacity = async_ctx_.data.readRandomLabels.capacity;
            size_t* out_count = async_ctx_.data.readRandomLabels.out_count;
            if (response_length < 2U) {
                setError(Error::ProtocolError);
                return;
            }
            uint16_t count = readLe16(response_data);
            if (count > capacity) {
                setError(Error::BufferTooSmall);
                return;
            }
            size_t offset = 2U;
            for (uint16_t i = 0; i < count; ++i) {
                if (offset + 4U > response_length) {
                    setError(Error::ProtocolError);
                    return;
                }
                out[i].dt_id = response_data[offset];
                out[i].spare = response_data[offset + 1U];
                out[i].result_length = readLe16(response_data + offset + 2U);
                offset += 4U;
                if (offset + out[i].result_length > response_length) {
                    setError(Error::ProtocolError);
                    return;
                }
                out[i].data = response_data + offset;
                offset += out[i].result_length;
            }
            async_ctx_.data.readRandomLabels.count = count;
            if (out_count != nullptr) *out_count = count;
            break;
        }
        case AsyncContext::Type::ReadRandomExt:
        case AsyncContext::Type::RunMonitorCycle: {
            uint16_t* word_values = async_ctx_.data.readRandom.word_values;
            uint16_t word_count = async_ctx_.data.readRandom.word_count;
            uint32_t* dword_values = async_ctx_.data.readRandom.dword_values;
            uint16_t dword_count = async_ctx_.data.readRandom.dword_count;
            size_t expected_length = (static_cast<size_t>(word_count) * 2U) + (static_cast<size_t>(dword_count) * 4U);
            if (response_length != expected_length) {
                setError(Error::ProtocolError);
                return;
            }
            size_t offset = 0;
            for (uint16_t i = 0; i < word_count; ++i) {
                word_values[i] = readLe16(response_data + offset);
                offset += 2U;
            }
            for (uint16_t i = 0; i < dword_count; ++i) {
                dword_values[i] = readLe32(response_data + offset);
                offset += 4U;
            }
            break;
        }
        default:
            if (response_length != 0) {
                setError(Error::ProtocolError);
                return;
            }
            break;
    }
    setError(Error::Ok);
}

Error SlmpClient::beginReadTypeName(TypeNameInfo& out, uint32_t now_ms) {
    out.model[0] = '\0';
    out.model_code = 0;
    out.has_model_code = false;
    async_ctx_.data.readTypeName.out = &out;
    return startAsync(AsyncContext::Type::ReadTypeName, 0, now_ms);
}

Error SlmpClient::readTypeName(TypeNameInfo& out) {
    Error err = beginReadTypeName(out, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::readCpuOperationState(CpuOperationState& out) {
    uint16_t status_word = 0U;
    Error err = readWords(dev::SD(dev::dec(203)), 1, &status_word, 1);
    if (err != Error::Ok) return err;

    const uint8_t raw_code = static_cast<uint8_t>(status_word & 0x000FU);
    CpuOperationStatus status = CpuOperationStatus::Unknown;
    switch (raw_code) {
        case 0x00U:
            status = CpuOperationStatus::Run;
            break;
        case 0x02U:
            status = CpuOperationStatus::Stop;
            break;
        case 0x03U:
            status = CpuOperationStatus::Pause;
            break;
        default:
            status = CpuOperationStatus::Unknown;
            break;
    }

    out.status = status;
    out.raw_status_word = status_word;
    out.raw_code = raw_code;
    return Error::Ok;
}

Error SlmpClient::beginReadWords(
    const DeviceAddress& device,
    uint16_t points,
    uint16_t* values,
    size_t value_capacity,
    uint32_t now_ms
) {
    if (values == nullptr || points == 0 || value_capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectWordReadDevice(device, points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    if (tx_capacity_ < 8) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadWords, written + 2U, now_ms);
}

Error SlmpClient::readWords(
    const DeviceAddress& device,
    uint16_t points,
    uint16_t* values,
    size_t value_capacity
) {
    Error err = beginReadWords(device, points, values, value_capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteWords(
    const DeviceAddress& device,
    const uint16_t* values,
    size_t count,
    uint32_t now_ms
) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = spec_size + 2U + (count * 2U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + written + 2U + (i * 2U), values[i]);
    }

    return startAsync(AsyncContext::Type::WriteWords, payload_length, now_ms);
}

Error SlmpClient::writeWords(const DeviceAddress& device, const uint16_t* values, size_t count) {
    Error err = beginWriteWords(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginReadBits(
    const DeviceAddress& device,
    uint16_t points,
    bool* values,
    size_t value_capacity,
    uint32_t now_ms
) {
    if (values == nullptr || points == 0 || value_capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectBitReadDevice(device);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    if (tx_capacity_ < 8) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadBits, written + 2U, now_ms);
}

Error SlmpClient::readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity) {
    Error err = beginReadBits(device, points, values, value_capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteBits(
    const DeviceAddress& device,
    const bool* values,
    size_t count,
    uint32_t now_ms
) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = spec_size + 2U + packedBitBytes(count);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(count));

    for (size_t i = 0; i < packedBitBytes(count); ++i) {
        size_t index = i * 2U;
        uint8_t high = values[index] ? 0x10U : 0x00U;
        uint8_t low = 0x00U;
        if (index + 1U < count && values[index + 1U]) {
            low = 0x01U;
        }
        tx_buffer_[written + 2U + i] = static_cast<uint8_t>(high | low);
    }

    return startAsync(AsyncContext::Type::WriteBits, payload_length, now_ms);
}

Error SlmpClient::writeBits(const DeviceAddress& device, const bool* values, size_t count) {
    Error err = beginWriteBits(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginReadDWords(
    const DeviceAddress& device,
    uint16_t points,
    uint32_t* values,
    size_t value_capacity,
    uint32_t now_ms
) {
    if (values == nullptr || points == 0 || value_capacity < points || points > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectDWordReadDevice(device);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    if (tx_capacity_ < 8) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(points * 2U));

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadDWords, written + 2U, now_ms);
}

Error SlmpClient::readDWords(
    const DeviceAddress& device,
    uint16_t points,
    uint32_t* values,
    size_t value_capacity
) {
    Error err = beginReadDWords(device, points, values, value_capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteDWords(
    const DeviceAddress& device,
    const uint32_t* values,
    size_t count,
    uint32_t now_ms
) {
    if (values == nullptr || count == 0 || count > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = spec_size + 2U + (count * 4U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(count * 2U));
    for (size_t i = 0; i < count; ++i) {
        writeLe32(tx_buffer_ + written + 2U + (i * 4U), values[i]);
    }

    return startAsync(AsyncContext::Type::WriteDWords, payload_length, now_ms);
}

Error SlmpClient::writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count) {
    Error err = beginWriteDWords(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginReadFloat32s(
    const DeviceAddress& device,
    uint16_t points,
    float* values,
    size_t value_capacity,
    uint32_t now_ms
) {
    if (values == nullptr || points == 0 || value_capacity < points || points > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }
    if (tx_capacity_ < 8) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(points * 2U));

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadFloat32s, written + 2U, now_ms);
}

Error SlmpClient::readFloat32s(
    const DeviceAddress& device,
    uint16_t points,
    float* values,
    size_t value_capacity
) {
    Error err = beginReadFloat32s(device, points, values, value_capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteFloat32s(
    const DeviceAddress& device,
    const float* values,
    size_t count,
    uint32_t now_ms
) {
    if (values == nullptr || count == 0 || count > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = spec_size + 2U + (count * 4U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    size_t written = encodeDeviceSpec(device, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, static_cast<uint16_t>(count * 2U));
    for (size_t i = 0; i < count; ++i) {
        writeLe32(tx_buffer_ + written + 2U + (i * 4U), floatToBits(values[i]));
    }

    return startAsync(AsyncContext::Type::WriteFloat32s, payload_length, now_ms);
}

Error SlmpClient::writeFloat32s(const DeviceAddress& device, const float* values, size_t count) {
    Error err = beginWriteFloat32s(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

// -----------------------------------------------------------------------
// Long timer / long retentive timer helpers (iQ-R)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadLongTimer(int head_no, int points, LongTimerResult* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points <= 0 || head_no < 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 8U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    uint16_t word_points = static_cast<uint16_t>(points) * 4U;
    DeviceAddress dev{DeviceCode::LTN, static_cast<uint32_t>(head_no)};
    size_t written = encodeDeviceSpec(dev, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, word_points);
    async_ctx_.data.readLongTimer.out = out;
    async_ctx_.data.readLongTimer.points = static_cast<uint16_t>(points);
    return startAsync(AsyncContext::Type::ReadLongTimer, written + 2U, now_ms);
}

Error SlmpClient::readLongTimer(int head_no, int points, LongTimerResult* out, size_t capacity) {
    Error err = beginReadLongTimer(head_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginReadLongRetentiveTimer(int head_no, int points, LongTimerResult* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points <= 0 || head_no < 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 8U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    uint16_t word_points = static_cast<uint16_t>(points) * 4U;
    DeviceAddress dev{DeviceCode::LSTN, static_cast<uint32_t>(head_no)};
    size_t written = encodeDeviceSpec(dev, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, word_points);
    async_ctx_.data.readLongTimer.out = out;
    async_ctx_.data.readLongTimer.points = static_cast<uint16_t>(points);
    return startAsync(AsyncContext::Type::ReadLongTimer, written + 2U, now_ms);
}

Error SlmpClient::readLongRetentiveTimer(int head_no, int points, LongTimerResult* out, size_t capacity) {
    Error err = beginReadLongRetentiveTimer(head_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::readLtcStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || points <= 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    Error err = readLongTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].coil;
    return last_error_;
}

Error SlmpClient::readLtsStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || points <= 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    Error err = readLongTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].contact;
    return last_error_;
}

Error SlmpClient::readLstcStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || points <= 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    Error err = readLongRetentiveTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].coil;
    return last_error_;
}

Error SlmpClient::readLstsStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || points <= 0 || capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    Error err = readLongRetentiveTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].contact;
    return last_error_;
}

// -----------------------------------------------------------------------
// Module Buffer (Intelligent Module Ux\G / Ux\HG)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points == 0 || capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t written = encodeModuleBufDeviceSpec(slot, use_hg, dev_no, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadModuleBufWords, written + 2U, now_ms);
}

Error SlmpClient::readWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity) {
    Error err = beginReadWordsModuleBuf(slot, use_hg, dev_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t* values, size_t count, uint32_t now_ms) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t spec_size = encodeModuleBufDeviceSpec(slot, use_hg, dev_no, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (spec_size == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    size_t payload_length = spec_size + 2U + count * 2U;
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + spec_size, static_cast<uint16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + spec_size + 2U + i * 2U, values[i]);
    }
    return startAsync(AsyncContext::Type::WriteModuleBufWords, payload_length, now_ms);
}

Error SlmpClient::writeWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t* values, size_t count) {
    Error err = beginWriteWordsModuleBuf(slot, use_hg, dev_no, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginReadBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points == 0 || capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t written = encodeModuleBufDeviceSpec(slot, use_hg, dev_no, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadModuleBufBits, written + 2U, now_ms);
}

Error SlmpClient::readBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool* out, size_t capacity) {
    Error err = beginReadBitsModuleBuf(slot, use_hg, dev_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool* values, size_t count, uint32_t now_ms) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t spec_size = encodeModuleBufDeviceSpec(slot, use_hg, dev_no, compatibility_mode_, tx_buffer_, tx_capacity_);
    if (spec_size == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    size_t payload_length = spec_size + 2U + packedBitBytes(count);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + spec_size, static_cast<uint16_t>(count));
    for (size_t i = 0; i < packedBitBytes(count); ++i) {
        size_t index = i * 2U;
        uint8_t high = values[index] ? 0x10U : 0x00U;
        uint8_t low = 0x00U;
        if (index + 1U < count && values[index + 1U]) low = 0x01U;
        tx_buffer_[spec_size + 2U + i] = static_cast<uint8_t>(high | low);
    }
    return startAsync(AsyncContext::Type::WriteModuleBufBits, payload_length, now_ms);
}

Error SlmpClient::writeBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool* values, size_t count) {
    Error err = beginWriteBitsModuleBuf(slot, use_hg, dev_no, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

// -----------------------------------------------------------------------
// Link Direct Device (CC-Link IE Jx\device)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points == 0 || capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t written = encodeLinkDirectDeviceSpec(j_net, code, dev_no, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadLinkDirectWords, written + 2U, now_ms);
}

Error SlmpClient::readWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity) {
    Error err = beginReadWordsLinkDirect(j_net, code, dev_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t* values, size_t count, uint32_t now_ms) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t spec_size = encodeLinkDirectDeviceSpec(j_net, code, dev_no, tx_buffer_, tx_capacity_);
    if (spec_size == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    size_t payload_length = spec_size + 2U + count * 2U;
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + spec_size, static_cast<uint16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + spec_size + 2U + i * 2U, values[i]);
    }
    return startAsync(AsyncContext::Type::WriteWordsLinkDirect, payload_length, now_ms);
}

Error SlmpClient::writeWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t* values, size_t count) {
    Error err = beginWriteWordsLinkDirect(j_net, code, dev_no, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginReadBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || points == 0 || capacity < points) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t written = encodeLinkDirectDeviceSpec(j_net, code, dev_no, tx_buffer_, tx_capacity_);
    if (written == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + written, points);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadLinkDirectBits, written + 2U, now_ms);
}

Error SlmpClient::readBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool* out, size_t capacity) {
    Error err = beginReadBitsLinkDirect(j_net, code, dev_no, points, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool* values, size_t count, uint32_t now_ms) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t spec_size = encodeLinkDirectDeviceSpec(j_net, code, dev_no, tx_buffer_, tx_capacity_);
    if (spec_size == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    size_t payload_length = spec_size + 2U + packedBitBytes(count);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + spec_size, static_cast<uint16_t>(count));
    for (size_t i = 0; i < packedBitBytes(count); ++i) {
        size_t index = i * 2U;
        uint8_t high = values[index] ? 0x10U : 0x00U;
        uint8_t low = 0x00U;
        if (index + 1U < count && values[index + 1U]) low = 0x01U;
        tx_buffer_[spec_size + 2U + i] = static_cast<uint8_t>(high | low);
    }
    return startAsync(AsyncContext::Type::WriteBitsLinkDirect, payload_length, now_ms);
}

Error SlmpClient::writeBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool* values, size_t count) {
    Error err = beginWriteBitsLinkDirect(j_net, code, dev_no, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

// -----------------------------------------------------------------------
// Memory Read / Write (0x0613 / 0x1613)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || word_length == 0 || capacity < word_length) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 6U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe32(tx_buffer_, head_address);
    writeLe16(tx_buffer_ + 4U, word_length);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = word_length;
    return startAsync(AsyncContext::Type::ReadMemoryWords, 6U, now_ms);
}

Error SlmpClient::readMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity) {
    Error err = beginReadMemoryWords(head_address, word_length, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteMemoryWords(uint32_t head_address, const uint16_t* values, size_t count, uint32_t now_ms) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t payload_length = 6U + count * 2U;
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe32(tx_buffer_, head_address);
    writeLe16(tx_buffer_ + 4U, static_cast<uint16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + 6U + i * 2U, values[i]);
    }
    return startAsync(AsyncContext::Type::WriteMemoryWords, payload_length, now_ms);
}

Error SlmpClient::writeMemoryWords(uint32_t head_address, const uint16_t* values, size_t count) {
    Error err = beginWriteMemoryWords(head_address, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

// -----------------------------------------------------------------------
// Extend Unit Read / Write (0x0601 / 0x1601)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t* out, size_t capacity, uint32_t now_ms) {
    if (out == nullptr || byte_length == 0 || capacity < byte_length) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 8U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe32(tx_buffer_, head_address);
    writeLe16(tx_buffer_ + 4U, byte_length);
    writeLe16(tx_buffer_ + 6U, module_no);
    async_ctx_.data.common.values = out;
    async_ctx_.data.common.points = byte_length;
    return startAsync(AsyncContext::Type::ReadExtendUnitBytes, 8U, now_ms);
}

Error SlmpClient::readExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t* out, size_t capacity) {
    Error err = beginReadExtendUnitBytes(head_address, byte_length, module_no, out, capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::readExtendUnitWords(uint32_t head_address, uint16_t word_length, uint16_t module_no, uint16_t* out, size_t capacity) {
    if (out == nullptr || word_length == 0 || capacity < word_length) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    uint16_t byte_length = static_cast<uint16_t>(word_length * 2U);
    uint8_t* byte_buf = reinterpret_cast<uint8_t*>(out);
    Error err = readExtendUnitBytes(head_address, byte_length, module_no, byte_buf, byte_length);
    if (err != Error::Ok) return err;
    // Decode big-endian bytes in-place (words were written little-endian into the same buffer)
    for (uint16_t i = 0; i < word_length; ++i) {
        out[i] = readLe16(byte_buf + static_cast<size_t>(i) * 2U);
    }
    return last_error_;
}

Error SlmpClient::beginWriteExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t* data, size_t byte_length, uint32_t now_ms) {
    if (data == nullptr || byte_length == 0 || byte_length > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t payload_length = 8U + byte_length;
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe32(tx_buffer_, head_address);
    writeLe16(tx_buffer_ + 4U, static_cast<uint16_t>(byte_length));
    writeLe16(tx_buffer_ + 6U, module_no);
    memcpy(tx_buffer_ + 8U, data, byte_length);
    return startAsync(AsyncContext::Type::WriteExtendUnitBytes, payload_length, now_ms);
}

Error SlmpClient::writeExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t* data, size_t byte_length) {
    Error err = beginWriteExtendUnitBytes(head_address, module_no, data, byte_length, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::writeExtendUnitWords(uint32_t head_address, uint16_t module_no, const uint16_t* values, size_t count) {
    if (values == nullptr || count == 0 || count > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t payload_length = 8U + count * 2U;
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe32(tx_buffer_, head_address);
    writeLe16(tx_buffer_ + 4U, static_cast<uint16_t>(count * 2U));
    writeLe16(tx_buffer_ + 6U, module_no);
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + 8U + i * 2U, values[i]);
    }
    Error err = startAsync(AsyncContext::Type::WriteExtendUnitBytes, payload_length, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

// -----------------------------------------------------------------------
// CPU Buffer convenience wrappers (extend unit module 0x03E0)
// -----------------------------------------------------------------------

Error SlmpClient::readCpuBufferBytes(uint32_t head_address, uint16_t byte_length, uint8_t* out, size_t capacity) {
    return readExtendUnitBytes(head_address, byte_length, 0x03E0, out, capacity);
}

Error SlmpClient::readCpuBufferWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity) {
    return readExtendUnitWords(head_address, word_length, 0x03E0, out, capacity);
}

Error SlmpClient::writeCpuBufferBytes(uint32_t head_address, const uint8_t* data, size_t byte_length) {
    return writeExtendUnitBytes(head_address, 0x03E0, data, byte_length);
}

Error SlmpClient::writeCpuBufferWords(uint32_t head_address, const uint16_t* values, size_t count) {
    return writeExtendUnitWords(head_address, 0x03E0, values, count);
}

Error SlmpClient::readExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t& value) {
    return readExtendUnitWords(head_address, 1, module_no, &value, 1);
}

Error SlmpClient::readExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t& value) {
    uint16_t buf[2] = {0, 0};
    Error err = readExtendUnitWords(head_address, 2, module_no, buf, 2);
    if (err == Error::Ok) value = (static_cast<uint32_t>(buf[1]) << 16) | buf[0];
    return err;
}

Error SlmpClient::writeExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t value) {
    return writeExtendUnitWords(head_address, module_no, &value, 1);
}

Error SlmpClient::writeExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t value) {
    uint16_t buf[2] = { static_cast<uint16_t>(value & 0xFFFFU), static_cast<uint16_t>(value >> 16) };
    return writeExtendUnitWords(head_address, module_no, buf, 2);
}

Error SlmpClient::readCpuBufferWord(uint32_t head_address, uint16_t& value) {
    return readCpuBufferWords(head_address, 1, &value, 1);
}

Error SlmpClient::readCpuBufferDWord(uint32_t head_address, uint32_t& value) {
    uint16_t buf[2] = {0, 0};
    Error err = readCpuBufferWords(head_address, 2, buf, 2);
    if (err == Error::Ok) value = (static_cast<uint32_t>(buf[1]) << 16) | buf[0];
    return err;
}

Error SlmpClient::writeCpuBufferWord(uint32_t head_address, uint16_t value) {
    return writeCpuBufferWords(head_address, &value, 1);
}

Error SlmpClient::writeCpuBufferDWord(uint32_t head_address, uint32_t value) {
    uint16_t buf[2] = { static_cast<uint16_t>(value & 0xFFFFU), static_cast<uint16_t>(value >> 16) };
    return writeCpuBufferWords(head_address, buf, 2);
}

// -----------------------------------------------------------------------
// Label Read / Write
// -----------------------------------------------------------------------

Error SlmpClient::beginReadArrayLabels(
    const LabelArrayReadPoint* points, size_t point_count,
    LabelArrayReadResult* out, size_t out_capacity, size_t* out_count,
    const LabelName* abbrevs, size_t abbrev_count,
    uint32_t now_ms
) {
    if (points == nullptr || point_count == 0 || out == nullptr || out_capacity < point_count) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    // Encode payload into tx_buffer
    size_t offset = 0;
    if (tx_capacity_ < 4U) { setError(Error::BufferTooSmall); return last_error_; }
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(point_count)); offset += 2U;
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(abbrev_count)); offset += 2U;
    for (size_t i = 0; i < abbrev_count; ++i) {
        size_t w = appendLabelName(abbrevs[i], tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
    }
    for (size_t i = 0; i < point_count; ++i) {
        size_t w = appendLabelName(points[i].label, tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
        if (offset + 4U > tx_capacity_) { setError(Error::BufferTooSmall); return last_error_; }
        tx_buffer_[offset++] = points[i].unit_specification;
        tx_buffer_[offset++] = 0x00;
        writeLe16(tx_buffer_ + offset, points[i].array_data_length); offset += 2U;
    }
    async_ctx_.data.readArrayLabels.out = out;
    async_ctx_.data.readArrayLabels.capacity = out_capacity;
    async_ctx_.data.readArrayLabels.count = 0;
    async_ctx_.data.readArrayLabels.out_count = out_count;
    return startAsync(AsyncContext::Type::ReadArrayLabels, offset, now_ms);
}

Error SlmpClient::readArrayLabels(
    const LabelArrayReadPoint* points, size_t point_count,
    LabelArrayReadResult* out, size_t out_capacity, size_t* out_count,
    const LabelName* abbrevs, size_t abbrev_count
) {
    Error err = beginReadArrayLabels(points, point_count, out, out_capacity, out_count, abbrevs, abbrev_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteArrayLabels(
    const LabelArrayWritePoint* points, size_t point_count,
    const LabelName* abbrevs, size_t abbrev_count,
    uint32_t now_ms
) {
    if (points == nullptr || point_count == 0) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t offset = 0;
    if (tx_capacity_ < 4U) { setError(Error::BufferTooSmall); return last_error_; }
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(point_count)); offset += 2U;
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(abbrev_count)); offset += 2U;
    for (size_t i = 0; i < abbrev_count; ++i) {
        size_t w = appendLabelName(abbrevs[i], tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
    }
    for (size_t i = 0; i < point_count; ++i) {
        size_t w = appendLabelName(points[i].label, tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
        if (offset + 4U + points[i].data_bytes > tx_capacity_) { setError(Error::BufferTooSmall); return last_error_; }
        tx_buffer_[offset++] = points[i].unit_specification;
        tx_buffer_[offset++] = 0x00;
        writeLe16(tx_buffer_ + offset, points[i].array_data_length); offset += 2U;
        memcpy(tx_buffer_ + offset, points[i].data, points[i].data_bytes);
        offset += points[i].data_bytes;
    }
    return startAsync(AsyncContext::Type::WriteArrayLabels, offset, now_ms);
}

Error SlmpClient::writeArrayLabels(
    const LabelArrayWritePoint* points, size_t point_count,
    const LabelName* abbrevs, size_t abbrev_count
) {
    Error err = beginWriteArrayLabels(points, point_count, abbrevs, abbrev_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginReadRandomLabels(
    const LabelName* labels, size_t label_count,
    LabelRandomReadResult* out, size_t out_capacity, size_t* out_count,
    const LabelName* abbrevs, size_t abbrev_count,
    uint32_t now_ms
) {
    if (labels == nullptr || label_count == 0 || out == nullptr || out_capacity < label_count) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t offset = 0;
    if (tx_capacity_ < 4U) { setError(Error::BufferTooSmall); return last_error_; }
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(label_count)); offset += 2U;
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(abbrev_count)); offset += 2U;
    for (size_t i = 0; i < abbrev_count; ++i) {
        size_t w = appendLabelName(abbrevs[i], tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
    }
    for (size_t i = 0; i < label_count; ++i) {
        size_t w = appendLabelName(labels[i], tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
    }
    async_ctx_.data.readRandomLabels.out = out;
    async_ctx_.data.readRandomLabels.capacity = out_capacity;
    async_ctx_.data.readRandomLabels.count = 0;
    async_ctx_.data.readRandomLabels.out_count = out_count;
    return startAsync(AsyncContext::Type::ReadRandomLabels, offset, now_ms);
}

Error SlmpClient::readRandomLabels(
    const LabelName* labels, size_t label_count,
    LabelRandomReadResult* out, size_t out_capacity, size_t* out_count,
    const LabelName* abbrevs, size_t abbrev_count
) {
    Error err = beginReadRandomLabels(labels, label_count, out, out_capacity, out_count, abbrevs, abbrev_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteRandomLabels(
    const LabelRandomWritePoint* points, size_t point_count,
    const LabelName* abbrevs, size_t abbrev_count,
    uint32_t now_ms
) {
    if (points == nullptr || point_count == 0) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t offset = 0;
    if (tx_capacity_ < 4U) { setError(Error::BufferTooSmall); return last_error_; }
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(point_count)); offset += 2U;
    writeLe16(tx_buffer_ + offset, static_cast<uint16_t>(abbrev_count)); offset += 2U;
    for (size_t i = 0; i < abbrev_count; ++i) {
        size_t w = appendLabelName(abbrevs[i], tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
    }
    for (size_t i = 0; i < point_count; ++i) {
        size_t w = appendLabelName(points[i].label, tx_buffer_ + offset, tx_capacity_ - offset);
        if (w == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += w;
        if (offset + 2U + points[i].data_bytes > tx_capacity_) { setError(Error::BufferTooSmall); return last_error_; }
        writeLe16(tx_buffer_ + offset, points[i].data_bytes); offset += 2U;
        memcpy(tx_buffer_ + offset, points[i].data, points[i].data_bytes);
        offset += points[i].data_bytes;
    }
    return startAsync(AsyncContext::Type::WriteRandomLabels, offset, now_ms);
}

Error SlmpClient::writeRandomLabels(
    const LabelRandomWritePoint* points, size_t point_count,
    const LabelName* abbrevs, size_t abbrev_count
) {
    Error err = beginWriteRandomLabels(points, point_count, abbrevs, abbrev_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::readOneWord(const DeviceAddress& device, uint16_t& value) {
    return readWords(device, 1, &value, 1);
}

Error SlmpClient::writeOneWord(const DeviceAddress& device, uint16_t value) {
    return writeWords(device, &value, 1);
}

Error SlmpClient::readOneBit(const DeviceAddress& device, bool& value) {
    return readBits(device, 1, &value, 1);
}

Error SlmpClient::writeOneBit(const DeviceAddress& device, bool value) {
    return writeBits(device, &value, 1);
}

Error SlmpClient::readOneDWord(const DeviceAddress& device, uint32_t& value) {
    return readDWords(device, 1, &value, 1);
}

Error SlmpClient::writeOneDWord(const DeviceAddress& device, uint32_t value) {
    return writeDWords(device, &value, 1);
}

Error SlmpClient::readOneFloat32(const DeviceAddress& device, float& value) {
    return readFloat32s(device, 1, &value, 1);
}

Error SlmpClient::writeOneFloat32(const DeviceAddress& device, float value) {
    return writeFloat32s(device, &value, 1);
}

Error SlmpClient::beginReadRandom(
    const DeviceAddress* word_devices,
    size_t word_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    const DeviceAddress* dword_devices,
    size_t dword_count,
    uint32_t* dword_values,
    size_t dword_value_capacity,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if ((word_count > 0 && (word_values == nullptr || word_value_capacity < word_count)) ||
        (dword_count > 0 && (dword_values == nullptr || dword_value_capacity < dword_count))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateDirectDeviceList(dword_devices, dword_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        if (isLongCounterContactDevice(word_devices[i].code)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (isLongCounterContactDevice(dword_devices[i].code)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = 2U + ((word_count + dword_count) * spec_size);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        if (encodeDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        offset += spec_size;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        offset += spec_size;
    }

    async_ctx_.data.readRandom.word_values = word_values;
    async_ctx_.data.readRandom.word_count = static_cast<uint16_t>(word_count);
    async_ctx_.data.readRandom.dword_values = dword_values;
    async_ctx_.data.readRandom.dword_count = static_cast<uint16_t>(dword_count);

    return startAsync(AsyncContext::Type::ReadRandom, payload_length, now_ms);
}

Error SlmpClient::readRandom(
    const DeviceAddress* word_devices,
    size_t word_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    const DeviceAddress* dword_devices,
    size_t dword_count,
    uint32_t* dword_values,
    size_t dword_value_capacity
) {
    Error err = beginReadRandom(
        word_devices,
        word_count,
        word_values,
        word_value_capacity,
        dword_devices,
        dword_count,
        dword_values,
        dword_value_capacity,
        getTimeMs()
    );
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteRandomWords(
    const DeviceAddress* word_devices,
    const uint16_t* word_values,
    size_t word_count,
    const DeviceAddress* dword_devices,
    const uint32_t* dword_values,
    size_t dword_count,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateDirectDeviceList(dword_devices, dword_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = 2U + (word_count * (spec_size + 2U)) + (dword_count * (spec_size + 4U));
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        if (encodeDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + spec_size, word_values[i]);
        offset += spec_size + 2U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe32(tx_buffer_ + offset + spec_size, dword_values[i]);
        offset += spec_size + 4U;
    }

    return startAsync(AsyncContext::Type::WriteRandomWords, payload_length, now_ms);
}

Error SlmpClient::writeRandomWords(
    const DeviceAddress* word_devices,
    const uint16_t* word_values,
    size_t word_count,
    const DeviceAddress* dword_devices,
    const uint32_t* dword_values,
    size_t dword_count
) {
    Error err = beginWriteRandomWords(
        word_devices,
        word_values,
        word_count,
        dword_devices,
        dword_values,
        dword_count,
        getTimeMs()
    );
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteRandomBits(
    const DeviceAddress* bit_devices,
    const bool* bit_values,
    size_t bit_count,
    uint32_t now_ms
) {
    if (bit_count == 0 || bit_count > 0xFFU || bit_devices == nullptr || bit_values == nullptr) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(bit_devices, bit_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t val_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 1U : 2U;
    size_t payload_length = 1U + (bit_count * (spec_size + val_size));
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(bit_count);

    size_t offset = 1U;
    for (size_t i = 0; i < bit_count; ++i) {
        if (encodeDeviceSpec(bit_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        if (compatibility_mode_ == CompatibilityMode::Legacy) {
            tx_buffer_[offset + spec_size] = bit_values[i] ? 1U : 0U;
            offset += spec_size + 1U;
        } else {
            writeLe16(tx_buffer_ + offset + spec_size, bit_values[i] ? 1U : 0U);
            offset += spec_size + 2U;
        }
    }

    return startAsync(AsyncContext::Type::WriteRandomBits, payload_length, now_ms);
}

Error SlmpClient::writeRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count) {
    Error err = beginWriteRandomBits(bit_devices, bit_values, bit_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginReadBlock(
    const DeviceBlockRead* word_blocks,
    size_t word_block_count,
    const DeviceBlockRead* bit_blocks,
    size_t bit_block_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    uint16_t* bit_values,
    size_t bit_value_capacity,
    uint32_t now_ms
) {
    if ((word_block_count == 0 && bit_block_count == 0) || word_block_count > 0xFFU || bit_block_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t total_word_points = 0;
    size_t total_bit_points = 0;
    Error validate_error = summarizeBlockReadList(word_blocks, word_block_count, total_word_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = summarizeBlockReadList(bit_blocks, bit_block_count, total_bit_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    if ((total_word_points > 0 && (word_values == nullptr || word_value_capacity < total_word_points)) ||
        (total_bit_points > 0 && (bit_values == nullptr || bit_value_capacity < total_bit_points))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = 2U + ((word_block_count + bit_block_count) * (spec_size + 2U));
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(bit_block_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_block_count; ++i) {
        size_t written = encodeDeviceSpec(word_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, word_blocks[i].points);
        offset += written + 2U;
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        size_t written = encodeDeviceSpec(bit_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, bit_blocks[i].points);
        offset += written + 2U;
    }

    async_ctx_.data.readBlock.word_values = word_values;
    async_ctx_.data.readBlock.total_word_points = total_word_points;
    async_ctx_.data.readBlock.bit_values = bit_values;
    async_ctx_.data.readBlock.total_bit_points = total_bit_points;

    return startAsync(AsyncContext::Type::ReadBlock, payload_length, now_ms);
}

Error SlmpClient::readBlock(
    const DeviceBlockRead* word_blocks,
    size_t word_block_count,
    const DeviceBlockRead* bit_blocks,
    size_t bit_block_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    uint16_t* bit_values,
    size_t bit_value_capacity
) {
    Error err = beginReadBlock(
        word_blocks,
        word_block_count,
        bit_blocks,
        bit_block_count,
        word_values,
        word_value_capacity,
        bit_values,
        bit_value_capacity,
        getTimeMs()
    );
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginWriteBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count,
    const BlockWriteOptions& options,
    uint32_t now_ms
) {
    const bool has_mixed_blocks = (word_block_count > 0U && bit_block_count > 0U);

    if (options.split_mixed_blocks && has_mixed_blocks) {
        return beginWriteBlockRequest(
            word_blocks,
            word_block_count,
            nullptr,
            0,
            word_blocks,
            word_block_count,
            bit_blocks,
            bit_block_count,
            options,
            AsyncContext::WriteBlockStage::SplitWord,
            true,
            now_ms
        );
    }

    return beginWriteBlockRequest(
        word_blocks,
        word_block_count,
        bit_blocks,
        bit_block_count,
        word_blocks,
        word_block_count,
        bit_blocks,
        bit_block_count,
        options,
        AsyncContext::WriteBlockStage::Direct,
        has_mixed_blocks,
        now_ms
    );
}

Error SlmpClient::beginWriteBlockRequest(
    const DeviceBlockWrite* request_word_blocks,
    size_t request_word_block_count,
    const DeviceBlockWrite* request_bit_blocks,
    size_t request_bit_block_count,
    const DeviceBlockWrite* all_word_blocks,
    size_t all_word_block_count,
    const DeviceBlockWrite* all_bit_blocks,
    size_t all_bit_block_count,
    const BlockWriteOptions& options,
    AsyncContext::WriteBlockStage stage,
    bool has_mixed_blocks,
    uint32_t now_ms
) {
    if ((request_word_block_count == 0 && request_bit_block_count == 0) || request_word_block_count > 0xFFU ||
        request_bit_block_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t total_word_points = 0;
    size_t total_bit_points = 0;
    Error validate_error = summarizeBlockWriteList(request_word_blocks, request_word_block_count, total_word_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = summarizeBlockWriteList(request_bit_blocks, request_bit_block_count, total_bit_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length =
        2U + ((request_word_block_count + request_bit_block_count) * (spec_size + 2U)) +
        ((total_word_points + total_bit_points) * 2U);
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(request_word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(request_bit_block_count);

    size_t offset = 2U;
    for (size_t i = 0; i < request_word_block_count; ++i) {
        size_t written =
            encodeDeviceSpec(request_word_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, request_word_blocks[i].points);
        offset += written + 2U;
    }
    for (size_t i = 0; i < request_bit_block_count; ++i) {
        size_t written =
            encodeDeviceSpec(request_bit_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, request_bit_blocks[i].points);
        offset += written + 2U;
    }
    for (size_t i = 0; i < request_word_block_count; ++i) {
        for (size_t j = 0; j < request_word_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, request_word_blocks[i].values[j]);
            offset += 2U;
        }
    }
    for (size_t i = 0; i < request_bit_block_count; ++i) {
        for (size_t j = 0; j < request_bit_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, request_bit_blocks[i].values[j]);
            offset += 2U;
        }
    }

    Error err = startAsync(AsyncContext::Type::WriteBlock, payload_length, now_ms);
    if (err != Error::Ok) {
        return err;
    }

    async_ctx_.data.writeBlock.word_blocks = all_word_blocks;
    async_ctx_.data.writeBlock.word_block_count = all_word_block_count;
    async_ctx_.data.writeBlock.bit_blocks = all_bit_blocks;
    async_ctx_.data.writeBlock.bit_block_count = all_bit_block_count;
    async_ctx_.data.writeBlock.options = options;
    async_ctx_.data.writeBlock.stage = stage;
    async_ctx_.data.writeBlock.has_mixed_blocks = has_mixed_blocks;
    return last_error_;
}

Error SlmpClient::beginWriteBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count,
    uint32_t now_ms
) {
    return beginWriteBlock(
        word_blocks,
        word_block_count,
        bit_blocks,
        bit_block_count,
        BlockWriteOptions{},
        now_ms
    );
}

Error SlmpClient::writeBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count
) {
    return writeBlock(
        word_blocks,
        word_block_count,
        bit_blocks,
        bit_block_count,
        BlockWriteOptions{}
    );
}

Error SlmpClient::writeBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count,
    const BlockWriteOptions& options
) {
    const bool has_mixed_blocks = (word_block_count > 0U && bit_block_count > 0U);

    if (options.split_mixed_blocks && has_mixed_blocks) {
        Error err = writeBlock(word_blocks, word_block_count, nullptr, 0, BlockWriteOptions{});
        if (err != Error::Ok) {
            return err;
        }
        return writeBlock(nullptr, 0, bit_blocks, bit_block_count, BlockWriteOptions{});
    }

    Error err = beginWriteBlock(word_blocks, word_block_count, bit_blocks, bit_block_count, options, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteRun(bool force, uint16_t clear_mode, uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemoteRunPayload(force, clear_mode, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemoteRun, payload_length, now_ms);
}

Error SlmpClient::remoteRun(bool force, uint16_t clear_mode) {
    Error err = beginRemoteRun(force, clear_mode, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteStop(uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemoteModePayload(0x0001U, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemoteStop, payload_length, now_ms);
}

Error SlmpClient::remoteStop() {
    Error err = beginRemoteStop(getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemotePause(bool force, uint32_t now_ms) {
    size_t payload_length = 0;
    const uint16_t mode = force ? 0x0003U : 0x0001U;
    Error encode_error = encodeRemoteModePayload(mode, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemotePause, payload_length, now_ms);
}

Error SlmpClient::remotePause(bool force) {
    Error err = beginRemotePause(force, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteLatchClear(uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemoteModePayload(0x0001U, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemoteLatchClear, payload_length, now_ms);
}

Error SlmpClient::remoteLatchClear() {
    Error err = beginRemoteLatchClear(getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteReset(uint16_t subcommand, bool expect_response, uint32_t now_ms) {
    if (subcommand != 0x0000U && subcommand != 0x0001U) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    async_ctx_.data.remoteReset.subcommand = subcommand;
    async_ctx_.data.remoteReset.expect_response = expect_response;
    return startAsync(AsyncContext::Type::RemoteReset, 0, now_ms);
}

Error SlmpClient::remoteReset(uint16_t subcommand, bool expect_response) {
    Error err = beginRemoteReset(subcommand, expect_response, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginSelfTestLoopback(
    const uint8_t* data,
    size_t data_length,
    uint8_t* out,
    size_t out_capacity,
    size_t* out_length,
    uint32_t now_ms
) {
    size_t payload_length = 0;
    Error encode_error = encodeSelfTestPayload(data, data_length, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }
    if (out == nullptr || out_length == nullptr) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    *out_length = 0U;
    async_ctx_.data.selfTest.out = out;
    async_ctx_.data.selfTest.out_capacity = out_capacity;
    async_ctx_.data.selfTest.out_length = out_length;
    return startAsync(AsyncContext::Type::SelfTest, payload_length, now_ms);
}

Error SlmpClient::selfTestLoopback(
    const uint8_t* data,
    size_t data_length,
    uint8_t* out,
    size_t out_capacity,
    size_t& out_length
) {
    Error err = beginSelfTestLoopback(data, data_length, out, out_capacity, &out_length, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginClearError(uint32_t now_ms) {
    return startAsync(AsyncContext::Type::ClearError, 0, now_ms);
}

Error SlmpClient::clearError() {
    Error err = beginClearError(getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemotePasswordUnlock(const char* password, uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::PasswordUnlock, payload_length, now_ms);
}

Error SlmpClient::remotePasswordUnlock(const char* password) {
    Error err = beginRemotePasswordUnlock(password, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemotePasswordLock(const char* password, uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::PasswordLock, payload_length, now_ms);
}

Error SlmpClient::remotePasswordLock(const char* password) {
    Error err = beginRemotePasswordLock(password, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

void SlmpClient::setError(Error error, uint16_t end_code) {
    last_error_ = error;
    last_end_code_ = end_code;
}

const char* errorString(Error error) {
    switch (error) {
        case Error::Ok:
            return "ok";
        case Error::InvalidArgument:
            return "invalid_argument";
        case Error::UnsupportedDevice:
            return "unsupported_device";
        case Error::BufferTooSmall:
            return "buffer_too_small";
        case Error::NotConnected:
            return "not_connected";
        case Error::TransportError:
            return "transport_error";
        case Error::ProtocolError:
            return "protocol_error";
        case Error::PlcError:
            return "plc_error";
        default:
            return "unknown_error";
    }
}

const char* endCodeString(uint16_t end_code) {
    switch (end_code) {
        case 0x0000:
            return "success";
        case 0x4013:
            return "state_rejected";
        case 0x4030:
            return "device_or_path_rejected";
        case 0x4031:
            return "range_or_allocation_mismatch";
        case 0x4043:
            return "invalid_extend_unit_argument";
        case 0x4080:
            return "target_or_module_mismatch";
        case 0x40C0:
            return "label_condition_failure";
        case 0x413E:
            return "file_state_or_environment_rejected";
        case 0x414A:
            return "target_or_write_path_rejected";
        case 0xC051:
            return "word_count_or_unit_rule_violation";
        case 0xC056:
            return "request_format_or_combination_rejected";
        case 0xC059:
            return "request_family_not_accepted";
        case 0xC05B:
            return "direct_g_hg_path_rejected";
        case 0xC061:
            return "request_content_or_path_rejected";
        case 0xC201:
            return "password_lock_or_authentication_required";
        case 0xC207:
            return "file_environment_rejected";
        case 0xC810:
            return "invalid_password";
        default:
            return "unknown_plc_end_code";
    }
}

size_t formatHexBytes(const uint8_t* data, size_t length, char* out, size_t out_capacity) {
    static const char kHexDigits[] = "0123456789ABCDEF";

    size_t required = length == 0 ? 0 : (length * 3U) - 1U;
    size_t written = 0;

    if (out != nullptr && out_capacity > 0) {
        out[0] = '\0';
    }
    if (data == nullptr && length != 0) {
        return 0;
    }

    for (size_t i = 0; i < length; ++i) {
        char chars[3];
        chars[0] = kHexDigits[(data[i] >> 4) & 0x0FU];
        chars[1] = kHexDigits[data[i] & 0x0FU];
        chars[2] = ' ';

        for (size_t j = 0; j < 2U; ++j) {
            if (out != nullptr && written + 1U < out_capacity) {
                out[written] = chars[j];
            }
            ++written;
        }

        if (i + 1U < length) {
            if (out != nullptr && written + 1U < out_capacity) {
                out[written] = chars[2];
            }
            ++written;
        }
    }

    if (out != nullptr && out_capacity > 0) {
        size_t terminator_index = written < (out_capacity - 1U) ? written : (out_capacity - 1U);
        out[terminator_index] = '\0';
    }
    return required;
}

// ---------------------------------------------------------------------------
// Extended random read / write
// ---------------------------------------------------------------------------

Error SlmpClient::beginReadRandomExt(
    const ExtDeviceSpec* word_devices, size_t word_count,
    uint16_t* word_values, size_t word_value_capacity,
    const ExtDeviceSpec* dword_devices, size_t dword_count,
    uint32_t* dword_values, size_t dword_value_capacity,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument); return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr || word_value_capacity < word_count)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr || dword_value_capacity < dword_count))) {
        setError(Error::InvalidArgument); return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);
    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        size_t written = encodeExtDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += written;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        size_t written = encodeExtDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += written;
    }

    async_ctx_.data.readRandom.word_values = word_values;
    async_ctx_.data.readRandom.word_count = static_cast<uint16_t>(word_count);
    async_ctx_.data.readRandom.dword_values = dword_values;
    async_ctx_.data.readRandom.dword_count = static_cast<uint16_t>(dword_count);
    return startAsync(AsyncContext::Type::ReadRandomExt, offset, now_ms);
}

Error SlmpClient::readRandomExt(
    const ExtDeviceSpec* word_devices, size_t word_count,
    uint16_t* word_values, size_t word_value_capacity,
    const ExtDeviceSpec* dword_devices, size_t dword_count,
    uint32_t* dword_values, size_t dword_value_capacity
) {
    Error err = beginReadRandomExt(word_devices, word_count, word_values, word_value_capacity,
                                   dword_devices, dword_count, dword_values, dword_value_capacity,
                                   getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteRandomWordsExt(
    const ExtDeviceSpec* word_devices, const uint16_t* word_values, size_t word_count,
    const ExtDeviceSpec* dword_devices, const uint32_t* dword_values, size_t dword_count,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument); return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr))) {
        setError(Error::InvalidArgument); return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);
    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        size_t written = encodeExtDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        writeLe16(tx_buffer_ + offset + written, word_values[i]);
        offset += written + 2U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        size_t written = encodeExtDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        writeLe32(tx_buffer_ + offset + written, dword_values[i]);
        offset += written + 4U;
    }
    return startAsync(AsyncContext::Type::WriteRandomWordsExt, offset, now_ms);
}

Error SlmpClient::writeRandomWordsExt(
    const ExtDeviceSpec* word_devices, const uint16_t* word_values, size_t word_count,
    const ExtDeviceSpec* dword_devices, const uint32_t* dword_values, size_t dword_count
) {
    Error err = beginWriteRandomWordsExt(word_devices, word_values, word_count,
                                         dword_devices, dword_values, dword_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginWriteRandomBitsExt(
    const ExtDeviceSpec* bit_devices, const bool* bit_values, size_t bit_count,
    uint32_t now_ms
) {
    if (bit_count == 0 || bit_count > 0xFFU || bit_devices == nullptr || bit_values == nullptr) {
        setError(Error::InvalidArgument); return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(bit_count);
    size_t offset = 1U;
    size_t val_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 1U : 2U;
    for (size_t i = 0; i < bit_count; ++i) {
        size_t written = encodeExtDeviceSpec(bit_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        if (val_size == 1U) {
            tx_buffer_[offset + written] = bit_values[i] ? 1U : 0U;
        } else {
            writeLe16(tx_buffer_ + offset + written, bit_values[i] ? 1U : 0U);
        }
        offset += written + val_size;
    }
    return startAsync(AsyncContext::Type::WriteRandomBitsExt, offset, now_ms);
}

Error SlmpClient::writeRandomBitsExt(const ExtDeviceSpec* bit_devices, const bool* bit_values, size_t bit_count) {
    Error err = beginWriteRandomBitsExt(bit_devices, bit_values, bit_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

// ---------------------------------------------------------------------------
// Monitor register / execute
// ---------------------------------------------------------------------------

Error SlmpClient::beginRegisterMonitorDevices(
    const DeviceAddress* word_devices, size_t word_count,
    const DeviceAddress* dword_devices, size_t dword_count,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument); return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count);
    if (validate_error != Error::Ok) { setError(validate_error); return last_error_; }
    validate_error = validateDirectDeviceList(dword_devices, dword_count);
    if (validate_error != Error::Ok) { setError(validate_error); return last_error_; }
    for (size_t i = 0; i < word_count; ++i) {
        if (isLongCounterContactDevice(word_devices[i].code)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (isLongCounterContactDevice(dword_devices[i].code)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length = 2U + ((word_count + dword_count) * spec_size);
    if (tx_capacity_ < payload_length) { setError(Error::BufferTooSmall); return last_error_; }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);
    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        if (encodeDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall); return last_error_;
        }
        offset += spec_size;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall); return last_error_;
        }
        offset += spec_size;
    }
    return startAsync(AsyncContext::Type::RegisterMonitorDevices, payload_length, now_ms);
}

Error SlmpClient::registerMonitorDevices(
    const DeviceAddress* word_devices, size_t word_count,
    const DeviceAddress* dword_devices, size_t dword_count
) {
    Error err = beginRegisterMonitorDevices(word_devices, word_count, dword_devices, dword_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginRegisterMonitorDevicesExt(
    const ExtDeviceSpec* word_devices, size_t word_count,
    const ExtDeviceSpec* dword_devices, size_t dword_count,
    uint32_t now_ms
) {
    if ((word_count == 0 && dword_count == 0) || word_count > 0xFFU || dword_count > 0xFFU) {
        setError(Error::InvalidArgument); return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);
    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        size_t written = encodeExtDeviceSpec(word_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += written;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        size_t written = encodeExtDeviceSpec(dword_devices[i], compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) { setError(Error::BufferTooSmall); return last_error_; }
        offset += written;
    }
    return startAsync(AsyncContext::Type::RegisterMonitorDevicesExt, offset, now_ms);
}

Error SlmpClient::registerMonitorDevicesExt(
    const ExtDeviceSpec* word_devices, size_t word_count,
    const ExtDeviceSpec* dword_devices, size_t dword_count
) {
    Error err = beginRegisterMonitorDevicesExt(word_devices, word_count, dword_devices, dword_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

Error SlmpClient::beginRunMonitorCycle(
    uint16_t* word_values, uint16_t word_count,
    uint32_t* dword_values, uint16_t dword_count,
    uint32_t now_ms
) {
    if ((word_count > 0 && word_values == nullptr) || (dword_count > 0 && dword_values == nullptr)) {
        setError(Error::InvalidArgument); return last_error_;
    }
    async_ctx_.data.readRandom.word_values = word_values;
    async_ctx_.data.readRandom.word_count = word_count;
    async_ctx_.data.readRandom.dword_values = dword_values;
    async_ctx_.data.readRandom.dword_count = dword_count;
    return startAsync(AsyncContext::Type::RunMonitorCycle, 0, now_ms);
}

Error SlmpClient::runMonitorCycle(
    uint16_t* word_values, uint16_t word_count,
    uint32_t* dword_values, uint16_t dword_count
) {
    Error err = beginRunMonitorCycle(word_values, word_count, dword_values, dword_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) { update(getTimeMs()); }
    return last_error_;
}

}  // namespace slmp
