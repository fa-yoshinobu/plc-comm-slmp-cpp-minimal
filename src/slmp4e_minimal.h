#ifndef SLMP4E_MINIMAL_H
#define SLMP4E_MINIMAL_H

#include <stddef.h>
#include <stdint.h>

namespace slmp4e {

enum class Error : uint8_t {
    Ok = 0,
    InvalidArgument,
    UnsupportedDevice,
    BufferTooSmall,
    NotConnected,
    TransportError,
    ProtocolError,
    PlcError,
    Busy,
};

enum class FrameType : uint8_t {
    Frame3E,
    Frame4E,
};

enum class DeviceCode : uint16_t {
    SM = 0x0091,
    SD = 0x00A9,
    X = 0x009C,
    Y = 0x009D,
    M = 0x0090,
    L = 0x0092,
    F = 0x0093,
    V = 0x0094,
    B = 0x00A0,
    D = 0x00A8,
    W = 0x00B4,
    TS = 0x00C1,
    TC = 0x00C0,
    TN = 0x00C2,
    LTS = 0x0051,
    LTC = 0x0050,
    LTN = 0x0052,
    STS = 0x00C7,
    STC = 0x00C6,
    STN = 0x00C8,
    LSTS = 0x0059,
    LSTC = 0x0058,
    LSTN = 0x005A,
    CS = 0x00C4,
    CC = 0x00C3,
    CN = 0x00C5,
    LCS = 0x0055,
    LCC = 0x0054,
    LCN = 0x0056,
    SB = 0x00A1,
    SW = 0x00B5,
    S = 0x0098,
    DX = 0x00A2,
    DY = 0x00A3,
    Z = 0x00CC,
    LZ = 0x0062,
    R = 0x00AF,
    ZR = 0x00B0,
    RD = 0x002C,
    G = 0x00AB,
    HG = 0x002E,
};

struct DeviceAddress {
    DeviceCode code;
    uint32_t number;
};

struct DeviceBlockRead {
    DeviceAddress device;
    uint16_t points;
};

struct DeviceBlockWrite {
    DeviceAddress device;
    const uint16_t* values;
    uint16_t points;
};

namespace dev {

struct DecNo {
    uint32_t value;
};

struct HexNo {
    uint32_t value;
};

constexpr DecNo dec(uint32_t value) {
    return {value};
}

constexpr HexNo hex(uint32_t value) {
    return {value};
}

#define SLMP4E_DEC_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(DecNo number) { \
        return {DeviceCode::name, number.value}; \
    }

#define SLMP4E_HEX_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(HexNo number) { \
        return {DeviceCode::name, number.value}; \
    }

SLMP4E_DEC_DEVICE_HELPER(SM)
SLMP4E_DEC_DEVICE_HELPER(SD)
SLMP4E_HEX_DEVICE_HELPER(X)
SLMP4E_HEX_DEVICE_HELPER(Y)
SLMP4E_DEC_DEVICE_HELPER(M)
SLMP4E_DEC_DEVICE_HELPER(L)
SLMP4E_DEC_DEVICE_HELPER(V)
SLMP4E_HEX_DEVICE_HELPER(B)
SLMP4E_DEC_DEVICE_HELPER(D)
SLMP4E_HEX_DEVICE_HELPER(W)
SLMP4E_DEC_DEVICE_HELPER(TS)
SLMP4E_DEC_DEVICE_HELPER(TC)
SLMP4E_DEC_DEVICE_HELPER(TN)
SLMP4E_DEC_DEVICE_HELPER(STS)
SLMP4E_DEC_DEVICE_HELPER(STC)
SLMP4E_DEC_DEVICE_HELPER(STN)
SLMP4E_DEC_DEVICE_HELPER(CS)
SLMP4E_DEC_DEVICE_HELPER(CC)
SLMP4E_DEC_DEVICE_HELPER(CN)
SLMP4E_DEC_DEVICE_HELPER(LCS)
SLMP4E_DEC_DEVICE_HELPER(LCC)
SLMP4E_DEC_DEVICE_HELPER(LCN)
SLMP4E_HEX_DEVICE_HELPER(SB)
SLMP4E_HEX_DEVICE_HELPER(SW)
SLMP4E_HEX_DEVICE_HELPER(DX)
SLMP4E_HEX_DEVICE_HELPER(DY)
SLMP4E_DEC_DEVICE_HELPER(R)
SLMP4E_DEC_DEVICE_HELPER(ZR)

constexpr DeviceAddress FDevice(DecNo number) {
    return {DeviceCode::F, number.value};
}

#undef SLMP4E_DEC_DEVICE_HELPER
#undef SLMP4E_HEX_DEVICE_HELPER

constexpr DeviceBlockRead blockRead(DeviceAddress device, uint16_t points) {
    return {device, points};
}

constexpr DeviceBlockWrite blockWrite(DeviceAddress device, const uint16_t* values, uint16_t points) {
    return {device, values, points};
}

}  // namespace dev

struct TargetAddress {
    uint8_t network = 0x00;
    uint8_t station = 0xFF;
    uint16_t module_io = 0x03FF;
    uint8_t multidrop = 0x00;
};

struct TypeNameInfo {
    char model[17];
    uint16_t model_code;
    bool has_model_code;
};

class ITransport {
  public:
    virtual ~ITransport() = default;

    virtual bool connect(const char* host, uint16_t port) = 0;
    virtual void close() = 0;
    virtual bool connected() const = 0;
    virtual bool writeAll(const uint8_t* data, size_t length) = 0;
    virtual bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) = 0;

    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual size_t read(uint8_t* data, size_t length) = 0;
    virtual size_t available() = 0;
};

class Slmp4eClient {
  public:
    Slmp4eClient(
        ITransport& transport,
        uint8_t* tx_buffer,
        size_t tx_capacity,
        uint8_t* rx_buffer,
        size_t rx_capacity
    );

    bool connect(const char* host, uint16_t port);
    void close();
    bool connected() const;

    void setTarget(const TargetAddress& target);
    const TargetAddress& target() const;

    void setFrameType(FrameType frame_type);
    FrameType frameType() const;

    void setMonitoringTimer(uint16_t monitoring_timer);
    uint16_t monitoringTimer() const;

    void setTimeoutMs(uint32_t timeout_ms);
    uint32_t timeoutMs() const;

    Error lastError() const;
    uint16_t lastEndCode() const;
    const uint8_t* lastRequestFrame() const;
    size_t lastRequestFrameLength() const;
    const uint8_t* lastResponseFrame() const;
    size_t lastResponseFrameLength() const;

    Error readTypeName(TypeNameInfo& out);
    Error readWords(const DeviceAddress& device, uint16_t points, uint16_t* values, size_t value_capacity);
    Error writeWords(const DeviceAddress& device, const uint16_t* values, size_t count);
    Error readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity);
    Error writeBits(const DeviceAddress& device, const bool* values, size_t count);
    Error readDWords(const DeviceAddress& device, uint16_t points, uint32_t* values, size_t value_capacity);
    Error writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count);
    Error readOneWord(const DeviceAddress& device, uint16_t& value);
    Error writeOneWord(const DeviceAddress& device, uint16_t value);
    Error readOneBit(const DeviceAddress& device, bool& value);
    Error writeOneBit(const DeviceAddress& device, bool value);
    Error readOneDWord(const DeviceAddress& device, uint32_t& value);
    Error writeOneDWord(const DeviceAddress& device, uint32_t value);
    Error readRandom(
        const DeviceAddress* word_devices,
        size_t word_count,
        uint16_t* word_values,
        size_t word_value_capacity,
        const DeviceAddress* dword_devices,
        size_t dword_count,
        uint32_t* dword_values,
        size_t dword_value_capacity
    );
    Error writeRandomWords(
        const DeviceAddress* word_devices,
        const uint16_t* word_values,
        size_t word_count,
        const DeviceAddress* dword_devices,
        const uint32_t* dword_values,
        size_t dword_count
    );
    Error writeRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count);
    Error readBlock(
        const DeviceBlockRead* word_blocks,
        size_t word_block_count,
        const DeviceBlockRead* bit_blocks,
        size_t bit_block_count,
        uint16_t* word_values,
        size_t word_value_capacity,
        uint16_t* bit_values,
        size_t bit_value_capacity
    );
    Error writeBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count
    );
    Error remotePasswordUnlock(const char* password);
    Error remotePasswordLock(const char* password);

    // Async API
    void update(uint32_t now_ms);
    bool isBusy() const;

    Error beginReadTypeName(TypeNameInfo& out, uint32_t now_ms);
    Error beginReadWords(const DeviceAddress& device, uint16_t points, uint16_t* values, size_t value_capacity, uint32_t now_ms);
    Error beginWriteWords(const DeviceAddress& device, const uint16_t* values, size_t count, uint32_t now_ms);
    Error beginReadBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity, uint32_t now_ms);
    Error beginWriteBits(const DeviceAddress& device, const bool* values, size_t count, uint32_t now_ms);
    Error beginReadDWords(const DeviceAddress& device, uint16_t points, uint32_t* values, size_t value_capacity, uint32_t now_ms);
    Error beginWriteDWords(const DeviceAddress& device, const uint32_t* values, size_t count, uint32_t now_ms);
    Error beginReadRandom(
        const DeviceAddress* word_devices,
        size_t word_count,
        uint16_t* word_values,
        size_t word_value_capacity,
        const DeviceAddress* dword_devices,
        size_t dword_count,
        uint32_t* dword_values,
        size_t dword_value_capacity,
        uint32_t now_ms
    );
    Error beginWriteRandomWords(
        const DeviceAddress* word_devices,
        const uint16_t* word_values,
        size_t word_count,
        const DeviceAddress* dword_devices,
        const uint32_t* dword_values,
        size_t dword_count,
        uint32_t now_ms
    );
    Error beginWriteRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count, uint32_t now_ms);
    Error beginReadBlock(
        const DeviceBlockRead* word_blocks,
        size_t word_block_count,
        const DeviceBlockRead* bit_blocks,
        size_t bit_block_count,
        uint16_t* word_values,
        size_t word_value_capacity,
        uint16_t* bit_values,
        size_t bit_value_capacity,
        uint32_t now_ms
    );
    Error beginWriteBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count,
        uint32_t now_ms
    );
    Error beginRemotePasswordUnlock(const char* password, uint32_t now_ms);
    Error beginRemotePasswordLock(const char* password, uint32_t now_ms);

  private:
    enum class State : uint8_t {
        Idle,
        Sending,
        ReceivingPrefix,
        ReceivingBody,
    };

    struct AsyncContext {
        enum class Type : uint8_t {
            None,
            ReadTypeName,
            ReadWords,
            WriteWords,
            ReadBits,
            WriteBits,
            ReadDWords,
            WriteDWords,
            ReadRandom,
            WriteRandomWords,
            WriteRandomBits,
            ReadBlock,
            WriteBlock,
            PasswordUnlock,
            PasswordLock,
        };

        Type type = Type::None;
        union {
            struct { void* values; uint16_t points; } common;
            struct { TypeNameInfo* out; } readTypeName;
            struct { uint16_t* word_values; uint16_t word_count; uint32_t* dword_values; uint16_t dword_count; } readRandom;
            struct { uint16_t* word_values; size_t total_word_points; uint16_t* bit_values; size_t total_bit_points; } readBlock;
        } data;
    };

    Error startAsync(AsyncContext::Type type, size_t payload_length, uint32_t now_ms);
    void completeAsync();

    Error request(
        uint16_t command,
        uint16_t subcommand,
        const uint8_t* payload,
        size_t payload_length,
        const uint8_t*& response_data,
        size_t& response_length
    );

    void setError(Error error, uint16_t end_code = 0);

    ITransport& transport_;
    uint8_t* tx_buffer_;
    size_t tx_capacity_;
    uint8_t* rx_buffer_;
    size_t rx_capacity_;
    TargetAddress target_;
    FrameType frame_type_;
    uint16_t monitoring_timer_;
    uint32_t timeout_ms_;
    uint16_t serial_;
    Error last_error_;
    uint16_t last_end_code_;
    size_t last_request_length_;
    size_t last_response_length_;

    State state_;
    size_t bytes_transferred_;
    uint32_t last_activity_ms_;
    AsyncContext async_ctx_;
};

const char* errorString(Error error);
const char* endCodeString(uint16_t end_code);
size_t formatHexBytes(const uint8_t* data, size_t length, char* out, size_t out_capacity);

}  // namespace slmp4e

#endif
