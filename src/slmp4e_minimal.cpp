#include "slmp4e_minimal.h"

#include <string.h>

namespace slmp4e {
namespace {

constexpr uint8_t kRequestSubheader0 = 0x54;
constexpr uint8_t kRequestSubheader1 = 0x00;
constexpr uint8_t kResponseSubheader0 = 0xD4;
constexpr uint8_t kResponseSubheader1 = 0x00;

constexpr uint16_t kCommandReadTypeName = 0x0101;
constexpr uint16_t kCommandDeviceRead = 0x0401;
constexpr uint16_t kCommandDeviceWrite = 0x1401;
constexpr uint16_t kCommandDeviceReadRandom = 0x0403;
constexpr uint16_t kCommandDeviceWriteRandom = 0x1402;
constexpr uint16_t kCommandDeviceReadBlock = 0x0406;
constexpr uint16_t kCommandDeviceWriteBlock = 0x1406;
constexpr uint16_t kCommandRemotePasswordUnlock = 0x1630;
constexpr uint16_t kCommandRemotePasswordLock = 0x1631;

constexpr uint16_t kSubcommandWord = 0x0002;
constexpr uint16_t kSubcommandBit = 0x0003;

constexpr size_t kRequestHeaderSize = 19;
constexpr size_t kResponseHeaderWithoutDataSize = 15;
constexpr size_t kResponsePrefixSize = 13;

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

inline size_t packedBitBytes(size_t bit_count) {
    return (bit_count + 1U) / 2U;
}

inline size_t encodeDeviceSpec(const DeviceAddress& device, uint8_t* out, size_t capacity) {
    if (capacity < 6) {
        return 0;
    }
    writeLe32(out, device.number);
    writeLe16(out + 4, static_cast<uint16_t>(device.code));
    return 6;
}

inline bool isUnsupportedDirectDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::G:
        case DeviceCode::HG:
        case DeviceCode::LSTC:
        case DeviceCode::LSTS:
        case DeviceCode::LTC:
        case DeviceCode::LTS:
        case DeviceCode::S:
        case DeviceCode::LSTN:
        case DeviceCode::LZ:
        case DeviceCode::LTN:
        case DeviceCode::RD:
        case DeviceCode::Z:
            return true;
        default:
            return false;
    }
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
        if (isUnsupportedDirectDevice(blocks[i].device.code)) {
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
        if (isUnsupportedDirectDevice(blocks[i].device.code)) {
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

}  // namespace

Slmp4eClient::Slmp4eClient(
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
      monitoring_timer_(0x0010),
      timeout_ms_(3000),
      serial_(0),
      last_error_(Error::Ok),
      last_end_code_(0),
      last_request_length_(0),
      last_response_length_(0) {}

bool Slmp4eClient::connect(const char* host, uint16_t port) {
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

void Slmp4eClient::close() {
    transport_.close();
}

bool Slmp4eClient::connected() const {
    return transport_.connected();
}

void Slmp4eClient::setTarget(const TargetAddress& target) {
    target_ = target;
}

const TargetAddress& Slmp4eClient::target() const {
    return target_;
}

void Slmp4eClient::setMonitoringTimer(uint16_t monitoring_timer) {
    monitoring_timer_ = monitoring_timer;
}

uint16_t Slmp4eClient::monitoringTimer() const {
    return monitoring_timer_;
}

void Slmp4eClient::setTimeoutMs(uint32_t timeout_ms) {
    timeout_ms_ = timeout_ms;
}

uint32_t Slmp4eClient::timeoutMs() const {
    return timeout_ms_;
}

Error Slmp4eClient::lastError() const {
    return last_error_;
}

uint16_t Slmp4eClient::lastEndCode() const {
    return last_end_code_;
}

const uint8_t* Slmp4eClient::lastRequestFrame() const {
    return tx_buffer_;
}

size_t Slmp4eClient::lastRequestFrameLength() const {
    return last_request_length_;
}

const uint8_t* Slmp4eClient::lastResponseFrame() const {
    return rx_buffer_;
}

size_t Slmp4eClient::lastResponseFrameLength() const {
    return last_response_length_;
}

Error Slmp4eClient::readTypeName(TypeNameInfo& out) {
    out.model[0] = '\0';
    out.model_code = 0;
    out.has_model_code = false;

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(kCommandReadTypeName, 0x0000, nullptr, 0, response_data, response_length);
    if (error != Error::Ok) {
        return error;
    }

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
    return Error::Ok;
}

Error Slmp4eClient::readWords(
    const DeviceAddress& device,
    uint16_t points,
    uint16_t* values,
    size_t value_capacity
) {
    if (values == nullptr || points == 0 || value_capacity < points) {
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

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, points);

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(kCommandDeviceRead, kSubcommandWord, tx_buffer_, 8, response_data, response_length);
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != static_cast<size_t>(points) * 2U) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    for (uint16_t i = 0; i < points; ++i) {
        values[i] = readLe16(response_data + (static_cast<size_t>(i) * 2U));
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeWords(const DeviceAddress& device, const uint16_t* values, size_t count) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t payload_length = 8U + (count * 2U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, static_cast<uint16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        writeLe16(tx_buffer_ + 8 + (i * 2U), values[i]);
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWrite,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity) {
    if (values == nullptr || points == 0 || value_capacity < points) {
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

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, points);

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(kCommandDeviceRead, kSubcommandBit, tx_buffer_, 8, response_data, response_length);
    if (error != Error::Ok) {
        return error;
    }

    size_t expected_bytes = packedBitBytes(points);
    if (response_length < expected_bytes) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    size_t bit_index = 0;
    for (size_t i = 0; i < expected_bytes && bit_index < points; ++i) {
        values[bit_index++] = ((response_data[i] >> 4) & 0x1U) != 0;
        if (bit_index < points) {
            values[bit_index++] = (response_data[i] & 0x1U) != 0;
        }
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeBits(const DeviceAddress& device, const bool* values, size_t count) {
    if (values == nullptr || count == 0 || count > 0xFFFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t payload_length = 8U + packedBitBytes(count);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, static_cast<uint16_t>(count));

    for (size_t i = 0; i < packedBitBytes(count); ++i) {
        size_t index = i * 2U;
        uint8_t high = values[index] ? 0x10U : 0x00U;
        uint8_t low = 0x00U;
        if (index + 1U < count && values[index + 1U]) {
            low = 0x01U;
        }
        tx_buffer_[8 + i] = static_cast<uint8_t>(high | low);
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWrite,
        kSubcommandBit,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::readDWords(
    const DeviceAddress& device,
    uint16_t points,
    uint32_t* values,
    size_t value_capacity
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

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, static_cast<uint16_t>(points * 2U));

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(kCommandDeviceRead, kSubcommandWord, tx_buffer_, 8, response_data, response_length);
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != static_cast<size_t>(points) * 4U) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    for (uint16_t i = 0; i < points; ++i) {
        values[i] = readLe32(response_data + (static_cast<size_t>(i) * 4U));
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count) {
    if (values == nullptr || count == 0 || count > 0x7FFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isUnsupportedDirectDevice(device.code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }

    size_t payload_length = 8U + (count * 4U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    if (encodeDeviceSpec(device, tx_buffer_, tx_capacity_) == 0) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    writeLe16(tx_buffer_ + 6, static_cast<uint16_t>(count * 2U));
    for (size_t i = 0; i < count; ++i) {
        writeLe32(tx_buffer_ + 8 + (i * 4U), values[i]);
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWrite,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }
    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::readOneWord(const DeviceAddress& device, uint16_t& value) {
    return readWords(device, 1, &value, 1);
}

Error Slmp4eClient::writeOneWord(const DeviceAddress& device, uint16_t value) {
    return writeWords(device, &value, 1);
}

Error Slmp4eClient::readOneBit(const DeviceAddress& device, bool& value) {
    return readBits(device, 1, &value, 1);
}

Error Slmp4eClient::writeOneBit(const DeviceAddress& device, bool value) {
    return writeBits(device, &value, 1);
}

Error Slmp4eClient::readOneDWord(const DeviceAddress& device, uint32_t& value) {
    return readDWords(device, 1, &value, 1);
}

Error Slmp4eClient::writeOneDWord(const DeviceAddress& device, uint32_t value) {
    return writeDWords(device, &value, 1);
}

Error Slmp4eClient::readRandom(
    const DeviceAddress* word_devices,
    size_t word_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    const DeviceAddress* dword_devices,
    size_t dword_count,
    uint32_t* dword_values,
    size_t dword_value_capacity
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

    size_t payload_length = 2U + ((word_count + dword_count) * 6U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        if (encodeDeviceSpec(word_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        offset += 6U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        offset += 6U;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceReadRandom,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }

    size_t expected_length = (word_count * 2U) + (dword_count * 4U);
    if (response_length != expected_length) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    offset = 0;
    for (size_t i = 0; i < word_count; ++i) {
        word_values[i] = readLe16(response_data + offset);
        offset += 2U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        dword_values[i] = readLe32(response_data + offset);
        offset += 4U;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeRandomWords(
    const DeviceAddress* word_devices,
    const uint16_t* word_values,
    size_t word_count,
    const DeviceAddress* dword_devices,
    const uint32_t* dword_values,
    size_t dword_count
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

    size_t payload_length = 2U + (word_count * 8U) + (dword_count * 10U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_count);
    tx_buffer_[1] = static_cast<uint8_t>(dword_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_count; ++i) {
        if (encodeDeviceSpec(word_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, word_values[i]);
        offset += 8U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe32(tx_buffer_ + offset + 6U, dword_values[i]);
        offset += 10U;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWriteRandom,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count) {
    if (bit_count == 0 || bit_count > 0xFFU || bit_devices == nullptr || bit_values == nullptr) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(bit_devices, bit_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t payload_length = 1U + (bit_count * 8U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(bit_count);

    size_t offset = 1U;
    for (size_t i = 0; i < bit_count; ++i) {
        if (encodeDeviceSpec(bit_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, bit_values[i] ? 1U : 0U);
        offset += 8U;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWriteRandom,
        kSubcommandBit,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::readBlock(
    const DeviceBlockRead* word_blocks,
    size_t word_block_count,
    const DeviceBlockRead* bit_blocks,
    size_t bit_block_count,
    uint16_t* word_values,
    size_t word_value_capacity,
    uint16_t* bit_values,
    size_t bit_value_capacity
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

    size_t payload_length = 2U + ((word_block_count + bit_block_count) * 8U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(bit_block_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_block_count; ++i) {
        if (encodeDeviceSpec(word_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, word_blocks[i].points);
        offset += 8U;
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        if (encodeDeviceSpec(bit_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, bit_blocks[i].points);
        offset += 8U;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceReadBlock,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }

    size_t expected_length = (total_word_points + total_bit_points) * 2U;
    if (response_length != expected_length) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    offset = 0;
    for (size_t i = 0; i < total_word_points; ++i) {
        word_values[i] = readLe16(response_data + offset);
        offset += 2U;
    }
    for (size_t i = 0; i < total_bit_points; ++i) {
        bit_values[i] = readLe16(response_data + offset);
        offset += 2U;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::writeBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count
) {
    if ((word_block_count == 0 && bit_block_count == 0) || word_block_count > 0xFFU || bit_block_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t total_word_points = 0;
    size_t total_bit_points = 0;
    Error validate_error = summarizeBlockWriteList(word_blocks, word_block_count, total_word_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = summarizeBlockWriteList(bit_blocks, bit_block_count, total_bit_points);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t payload_length =
        2U + ((word_block_count + bit_block_count) * 8U) + ((total_word_points + total_bit_points) * 2U);
    if (tx_capacity_ < payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(bit_block_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_block_count; ++i) {
        if (encodeDeviceSpec(word_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, word_blocks[i].points);
        offset += 8U;
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        if (encodeDeviceSpec(bit_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + 6U, bit_blocks[i].points);
        offset += 8U;
    }
    for (size_t i = 0; i < word_block_count; ++i) {
        for (size_t j = 0; j < word_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, word_blocks[i].values[j]);
            offset += 2U;
        }
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        for (size_t j = 0; j < bit_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, bit_blocks[i].values[j]);
            offset += 2U;
        }
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandDeviceWriteBlock,
        kSubcommandWord,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::remotePasswordUnlock(const char* password) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandRemotePasswordUnlock,
        0x0000,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::remotePasswordLock(const char* password) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    const uint8_t* response_data = nullptr;
    size_t response_length = 0;
    Error error = request(
        kCommandRemotePasswordLock,
        0x0000,
        tx_buffer_,
        payload_length,
        response_data,
        response_length
    );
    if (error != Error::Ok) {
        return error;
    }
    if (response_length != 0) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    setError(Error::Ok);
    return last_error_;
}

Error Slmp4eClient::request(
    uint16_t command,
    uint16_t subcommand,
    const uint8_t* payload,
    size_t payload_length,
    const uint8_t*& response_data,
    size_t& response_length
) {
    response_data = nullptr;
    response_length = 0;
    last_request_length_ = 0;
    last_response_length_ = 0;

    if (!transport_.connected()) {
        setError(Error::NotConnected);
        return last_error_;
    }

    size_t frame_length = kRequestHeaderSize + payload_length;
    if (tx_buffer_ == nullptr || rx_buffer_ == nullptr || tx_capacity_ < frame_length || rx_capacity_ < kResponseHeaderWithoutDataSize) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    uint16_t serial = serial_;
    serial_ = static_cast<uint16_t>(serial_ + 1U);

    if (payload_length > 0 && payload != nullptr) {
        memmove(tx_buffer_ + kRequestHeaderSize, payload, payload_length);
    }

    tx_buffer_[0] = kRequestSubheader0;
    tx_buffer_[1] = kRequestSubheader1;
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
    last_request_length_ = frame_length;

    if (!transport_.writeAll(tx_buffer_, frame_length)) {
        setError(Error::TransportError);
        return last_error_;
    }
    if (!transport_.readExact(rx_buffer_, kResponsePrefixSize, timeout_ms_)) {
        setError(Error::TransportError);
        return last_error_;
    }
    if (rx_buffer_[0] != kResponseSubheader0 || rx_buffer_[1] != kResponseSubheader1) {
        setError(Error::ProtocolError);
        return last_error_;
    }
    if (readLe16(rx_buffer_ + 2) != serial) {
        setError(Error::ProtocolError);
        return last_error_;
    }

    uint16_t response_data_length = readLe16(rx_buffer_ + 11);
    if (response_data_length < 2U) {
        setError(Error::ProtocolError);
        return last_error_;
    }
    size_t response_frame_length = kResponsePrefixSize + response_data_length;
    if (rx_capacity_ < response_frame_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    if (!transport_.readExact(rx_buffer_ + kResponsePrefixSize, response_data_length, timeout_ms_)) {
        setError(Error::TransportError);
        return last_error_;
    }
    last_response_length_ = response_frame_length;

    uint16_t end_code = readLe16(rx_buffer_ + 13);
    if (end_code != 0) {
        setError(Error::PlcError, end_code);
        return last_error_;
    }

    response_data = rx_buffer_ + 15;
    response_length = response_data_length - 2U;
    setError(Error::Ok);
    return last_error_;
}

void Slmp4eClient::setError(Error error, uint16_t end_code) {
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
        case 0xC051:
            return "word_count_or_unit_rule_violation";
        case 0xC059:
            return "request_family_not_accepted";
        case 0xC05B:
            return "direct_g_hg_path_rejected";
        case 0xC061:
            return "request_content_or_path_rejected";
        case 0xC207:
            return "file_environment_rejected";
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

}  // namespace slmp4e
