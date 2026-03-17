#include "slmp4e_minimal.h"

#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
static uint32_t millis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
#endif

namespace slmp4e {

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
constexpr uint16_t kCommandRemotePasswordUnlock = 0x1630;
constexpr uint16_t kCommandRemotePasswordLock = 0x1631;

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

inline size_t packedBitBytes(size_t bit_count) {
    return (bit_count + 1U) / 2U;
}

inline size_t encodeDeviceSpec(const DeviceAddress& device, uint8_t* out, size_t capacity) {
    if (capacity < 6) return 0;
    writeLe32(out, device.number);
    writeLe16(out + 4, static_cast<uint16_t>(device.code));
    return 6;
}

inline bool isUnsupportedDirectDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::G:
        case DeviceCode::HG:
        case DeviceCode::LCC:
        case DeviceCode::LCN:
        case DeviceCode::LCS:
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
      frame_type_(FrameType::Frame4E),
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

bool Slmp4eClient::isBusy() const {
    return state_ != State::Idle;
}

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

void Slmp4eClient::setFrameType(FrameType frame_type) {
    frame_type_ = frame_type;
}

FrameType Slmp4eClient::frameType() const {
    return frame_type_;
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

Error Slmp4eClient::startAsync(AsyncContext::Type type, size_t payload_length, uint32_t now_ms) {
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
    switch (type) {
        case AsyncContext::Type::ReadTypeName:
            command = kCommandReadTypeName;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::ReadWords:
        case AsyncContext::Type::ReadDWords:
        case AsyncContext::Type::ReadBits:
            command = kCommandDeviceRead;
            subcommand = (type == AsyncContext::Type::ReadBits) ? kSubcommandBit : kSubcommandWord;
            break;
        case AsyncContext::Type::WriteWords:
        case AsyncContext::Type::WriteDWords:
        case AsyncContext::Type::WriteBits:
            command = kCommandDeviceWrite;
            subcommand = (type == AsyncContext::Type::WriteBits) ? kSubcommandBit : kSubcommandWord;
            break;
        case AsyncContext::Type::ReadRandom:
            command = kCommandDeviceReadRandom;
            subcommand = kSubcommandWord;
            break;
        case AsyncContext::Type::WriteRandomWords:
            command = kCommandDeviceWriteRandom;
            subcommand = kSubcommandWord;
            break;
        case AsyncContext::Type::WriteRandomBits:
            command = kCommandDeviceWriteRandom;
            subcommand = kSubcommandBit;
            break;
        case AsyncContext::Type::ReadBlock:
            command = kCommandDeviceReadBlock;
            subcommand = kSubcommandWord;
            break;
        case AsyncContext::Type::WriteBlock:
            command = kCommandDeviceWriteBlock;
            subcommand = kSubcommandWord;
            break;
        case AsyncContext::Type::PasswordUnlock:
            command = kCommandRemotePasswordUnlock;
            subcommand = 0x0000;
            break;
        case AsyncContext::Type::PasswordLock:
            command = kCommandRemotePasswordLock;
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

void Slmp4eClient::update(uint32_t now_ms) {
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

void Slmp4eClient::completeAsync() {
    size_t response_prefix_size = (frame_type_ == FrameType::Frame4E) ? kResponsePrefixSize4E : kResponsePrefixSize3E;
    uint16_t end_code = readLe16(rx_buffer_ + response_prefix_size);
    if (end_code != 0) {
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
        default:
            if (response_length != 0) {
                setError(Error::ProtocolError);
                return;
            }
            break;
    }
    setError(Error::Ok);
}

Error Slmp4eClient::beginReadTypeName(TypeNameInfo& out, uint32_t now_ms) {
    out.model[0] = '\0';
    out.model_code = 0;
    out.has_model_code = false;
    async_ctx_.data.readTypeName.out = &out;
    return startAsync(AsyncContext::Type::ReadTypeName, 0, now_ms);
}

Error Slmp4eClient::readTypeName(TypeNameInfo& out) {
    Error err = beginReadTypeName(out, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginReadWords(
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

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadWords, 8, now_ms);
}

Error Slmp4eClient::readWords(
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

Error Slmp4eClient::beginWriteWords(
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

    return startAsync(AsyncContext::Type::WriteWords, payload_length, now_ms);
}

Error Slmp4eClient::writeWords(const DeviceAddress& device, const uint16_t* values, size_t count) {
    Error err = beginWriteWords(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginReadBits(
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

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadBits, 8, now_ms);
}

Error Slmp4eClient::readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity) {
    Error err = beginReadBits(device, points, values, value_capacity, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginWriteBits(
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

    return startAsync(AsyncContext::Type::WriteBits, payload_length, now_ms);
}

Error Slmp4eClient::writeBits(const DeviceAddress& device, const bool* values, size_t count) {
    Error err = beginWriteBits(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginReadDWords(
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

    async_ctx_.data.common.values = values;
    async_ctx_.data.common.points = points;
    return startAsync(AsyncContext::Type::ReadDWords, 8, now_ms);
}

Error Slmp4eClient::readDWords(
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

Error Slmp4eClient::beginWriteDWords(
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

    return startAsync(AsyncContext::Type::WriteDWords, payload_length, now_ms);
}

Error Slmp4eClient::writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count) {
    Error err = beginWriteDWords(device, values, count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
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

Error Slmp4eClient::beginReadRandom(
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

    size_t spec_size = 6U;
    size_t payload_length = 2U + ((word_count + dword_count) * spec_size);
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
        offset += spec_size;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
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

Error Slmp4eClient::beginWriteRandomWords(
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

    size_t spec_size = 6U;
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
        if (encodeDeviceSpec(word_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + spec_size, word_values[i]);
        offset += spec_size + 2U;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (encodeDeviceSpec(dword_devices[i], tx_buffer_ + offset, tx_capacity_ - offset) == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe32(tx_buffer_ + offset + spec_size, dword_values[i]);
        offset += spec_size + 4U;
    }

    return startAsync(AsyncContext::Type::WriteRandomWords, payload_length, now_ms);
}

Error Slmp4eClient::writeRandomWords(
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

Error Slmp4eClient::beginWriteRandomBits(
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

    size_t spec_size = 6U;
    size_t payload_length = 1U + (bit_count * (spec_size + 2U));
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
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
        writeLe16(tx_buffer_ + offset + spec_size, bit_values[i] ? 1U : 0U);
        offset += spec_size + 2U;
    }

    return startAsync(AsyncContext::Type::WriteRandomBits, payload_length, now_ms);
}

Error Slmp4eClient::writeRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count) {
    Error err = beginWriteRandomBits(bit_devices, bit_values, bit_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginReadBlock(
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

    size_t spec_size = 6U;
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
        size_t written = encodeDeviceSpec(word_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, word_blocks[i].points);
        offset += written + 2U;
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        size_t written = encodeDeviceSpec(bit_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset);
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

Error Slmp4eClient::beginWriteBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count,
    uint32_t now_ms
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

    size_t spec_size = 6U;
    size_t payload_length =
        2U + ((word_block_count + bit_block_count) * (spec_size + 2U)) + ((total_word_points + total_bit_points) * 2U);
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(bit_block_count);

    size_t offset = 2U;
    for (size_t i = 0; i < word_block_count; ++i) {
        size_t written = encodeDeviceSpec(word_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, word_blocks[i].points);
        offset += written + 2U;
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        size_t written = encodeDeviceSpec(bit_blocks[i].device, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, bit_blocks[i].points);
        offset += written + 2U;
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

    return startAsync(AsyncContext::Type::WriteBlock, payload_length, now_ms);
}

Error Slmp4eClient::writeBlock(
    const DeviceBlockWrite* word_blocks,
    size_t word_block_count,
    const DeviceBlockWrite* bit_blocks,
    size_t bit_block_count
) {
    Error err = beginWriteBlock(word_blocks, word_block_count, bit_blocks, bit_block_count, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginRemotePasswordUnlock(const char* password, uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::PasswordUnlock, payload_length, now_ms);
}

Error Slmp4eClient::remotePasswordUnlock(const char* password) {
    Error err = beginRemotePasswordUnlock(password, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error Slmp4eClient::beginRemotePasswordLock(const char* password, uint32_t now_ms) {
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(password, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::PasswordLock, payload_length, now_ms);
}

Error Slmp4eClient::remotePasswordLock(const char* password) {
    Error err = beginRemotePasswordLock(password, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
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
