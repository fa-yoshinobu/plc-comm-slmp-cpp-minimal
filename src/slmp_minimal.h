/**
 * @file slmp_minimal.h
 * @brief Ultra-lightweight SLMP (MC Protocol) Client for Embedded C++.
 * 
 * Provides a minimal, buffer-efficient implementation of the SLMP protocol
 * suitable for resource-constrained embedded systems like Arduino, ESP32, and RP2040.
 * 
 * @author FA-YOSHINOBU
 * @copyright MIT License
 */

#ifndef SLMP_MINIMAL_H
#define SLMP_MINIMAL_H

#include <stddef.h>
#include <stdint.h>

/**
 * @namespace slmp
 * @brief Root namespace for all SLMP communication components.
 */
namespace slmp {

/**
 * @defgroup SLMP_Core Core API
 * @brief Essential SLMP client and types.
 * @{
 */

/**
 * @enum Error
 * @brief Error codes returned by SLMP operations.
 */
enum class Error : uint8_t {
    Ok = 0,                 ///< Operation successful.
    InvalidArgument,        ///< One or more arguments are invalid (e.g. nullptr or range error).
    UnsupportedDevice,      ///< The requested device code is not supported by this library or target PLC.
    BufferTooSmall,         ///< Provided buffer capacity is insufficient for request or response.
    NotConnected,           ///< Transport layer is not connected.
    TransportError,         ///< Error occurred in the transport layer (e.g., TCP timeout, UDP loss).
    ProtocolError,          ///< Malformed packet or protocol violation detected.
    PlcError,               ///< PLC returned a non-zero end code (use @ref SlmpClient::lastEndCode to get details).
    Busy,                   ///< An asynchronous operation is already in progress.
};

/**
 * @enum FrameType
 * @brief SLMP frame formats supported by the library.
 */
enum class FrameType : uint8_t {
    Frame3E,                ///< Standard 3E frame (most common for Q/L/iQ-R).
    Frame4E,                ///< 4E frame (extended functionality, used for iQ-R specific features).
};

/**
 * @enum CompatibilityMode
 * @brief Subcommand selection for device access.
 * 
 * Determines whether to use modern iQ-R extended subcommands or legacy Q/L subcommands.
 */
enum class CompatibilityMode : uint8_t {
    iQR,     ///< Use subcommands 0x0002/0x0003 (iQ-R series extensions). Supports larger address ranges.
    Legacy,  ///< Use subcommands 0x0000/0x0001 (Legacy Q/L series).
};

/**
 * @enum ProfileClass
 * @brief Classified PLC series profiles for automatic configuration.
 */
enum class ProfileClass : uint8_t {
    Unknown,                ///< Model not recognized.
    LegacyQL,               ///< Standard Q or L series.
    ModernIQR,              ///< Modern iQ-R series.
};

/** @} */ // end of SLMP_Core

/**
 * @defgroup SLMP_Devices Device Definition
 * @brief Device codes and addressing helpers.
 * @{
 */

/**
 * @enum DeviceCode
 * @brief SLMP binary device codes (Subcommand dependent).
 */
enum class DeviceCode : uint16_t {
    SM = 0x0091,            ///< Special Relay
    SD = 0x00A9,            ///< Special Register
    X = 0x009C,             ///< Input
    Y = 0x009D,             ///< Output
    M = 0x0090,             ///< Internal Relay
    L = 0x0092,             ///< Latch Relay
    F = 0x0093,             ///< Annunciator
    V = 0x0094,             ///< Edge Relay
    B = 0x00A0,             ///< Link Relay
    D = 0x00A8,             ///< Data Register
    W = 0x00B4,             ///< Link Register
    TS = 0x00C1,            ///< Timer Contact
    TC = 0x00C0,            ///< Timer Coil
    TN = 0x00C2,            ///< Timer Current Value
    LTS = 0x0051,           ///< Long Timer Contact
    LTC = 0x0050,           ///< Long Timer Coil
    LTN = 0x0052,           ///< Long Timer Current Value
    STS = 0x00C7,           ///< Retentive Timer Contact
    STC = 0x00C6,           ///< Retentive Timer Coil
    STN = 0x00C8,           ///< Retentive Timer Current Value
    LSTS = 0x0059,          ///< Long Retentive Timer Contact
    LSTC = 0x0058,          ///< Long Retentive Timer Coil
    LSTN = 0x005A,          ///< Long Retentive Timer Current Value
    CS = 0x00C4,            ///< Counter Contact
    CC = 0x00C3,            ///< Counter Coil
    CN = 0x00C5,            ///< Counter Current Value
    LCS = 0x0055,           ///< Long Counter Contact
    LCC = 0x0054,           ///< Long Counter Coil
    LCN = 0x0056,           ///< Long Counter Current Value
    SB = 0x00A1,            ///< Link Special Relay
    SW = 0x00B5,            ///< Link Special Register
    S = 0x0098,             ///< Step Relay
    DX = 0x00A2,            ///< Direct Input
    DY = 0x00A3,            ///< Direct Output
    Z = 0x00CC,             ///< Index Register
    LZ = 0x0062,            ///< Long Index Register
    R = 0x00AF,             ///< File Register
    ZR = 0x00B0,            ///< File Register (Continuous)
    RD = 0x002C,            ///< Refresh Data Register
    G = 0x00AB,             ///< Buffer Memory
    HG = 0x002E,            ///< Long Buffer Memory
};

/**
 * @struct DeviceAddress
 * @brief Represents a specific device and its numeric address.
 */
struct DeviceAddress {
    DeviceCode code;        ///< Device type code (e.g. D, M, X).
    uint32_t number;        ///< Numeric address (index). Use @ref dev::dec or @ref dev::hex.
};

/**
 * @struct DeviceBlockRead
 * @brief Description for a contiguous block of devices to read.
 */
struct DeviceBlockRead {
    DeviceAddress device;   ///< Starting device address.
    uint16_t points;        ///< Number of points to read.
};

/**
 * @struct DeviceBlockWrite
 * @brief Description for a contiguous block of devices to write.
 */
struct DeviceBlockWrite {
    DeviceAddress device;   ///< Starting device address.
    const uint16_t* values; ///< Pointer to word values to write.
    uint16_t points;        ///< Number of points to write.
};

/**
 * @struct BlockWriteOptions
 * @brief Configuration for block write operations.
 */
struct BlockWriteOptions {
    bool split_mixed_blocks;    ///< Split bit and word blocks into separate requests.
    bool retry_mixed_on_error;  ///< Retry as separate requests if mixed write fails.
};

/**
 * @namespace slmp::dev
 * @brief Fluent API for defining device addresses.
 * 
 * Provides a readable way to define PLC devices:
 * @code
 * auto d100 = slmp::dev::D(slmp::dev::dec(100));
 * auto x0 = slmp::dev::X(slmp::dev::hex(0x00));
 * @endcode
 */
namespace dev {

struct DecNo { uint32_t value; };
struct HexNo { uint32_t value; };

/** @brief Define decimal address. */
constexpr DecNo dec(uint32_t value) { return {value}; }
/** @brief Define hexadecimal address. */
constexpr HexNo hex(uint32_t value) { return {value}; }

#define SLMP_DEC_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(DecNo number) { \
        return {DeviceCode::name, number.value}; \
    }

#define SLMP_HEX_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(HexNo number) { \
        return {DeviceCode::name, number.value}; \
    }

SLMP_DEC_DEVICE_HELPER(SM)
SLMP_DEC_DEVICE_HELPER(SD)
SLMP_HEX_DEVICE_HELPER(X)
SLMP_HEX_DEVICE_HELPER(Y)
SLMP_DEC_DEVICE_HELPER(M)
SLMP_DEC_DEVICE_HELPER(L)
SLMP_DEC_DEVICE_HELPER(V)
SLMP_HEX_DEVICE_HELPER(B)
SLMP_DEC_DEVICE_HELPER(D)
SLMP_HEX_DEVICE_HELPER(W)
SLMP_DEC_DEVICE_HELPER(TS)
SLMP_DEC_DEVICE_HELPER(TC)
SLMP_DEC_DEVICE_HELPER(TN)
SLMP_DEC_DEVICE_HELPER(STS)
SLMP_DEC_DEVICE_HELPER(STC)
SLMP_DEC_DEVICE_HELPER(STN)
SLMP_DEC_DEVICE_HELPER(CS)
SLMP_DEC_DEVICE_HELPER(CC)
SLMP_DEC_DEVICE_HELPER(CN)
SLMP_DEC_DEVICE_HELPER(LCS)
SLMP_DEC_DEVICE_HELPER(LCC)
SLMP_DEC_DEVICE_HELPER(LCN)
SLMP_HEX_DEVICE_HELPER(SB)
SLMP_HEX_DEVICE_HELPER(SW)
SLMP_HEX_DEVICE_HELPER(DX)
SLMP_HEX_DEVICE_HELPER(DY)
SLMP_DEC_DEVICE_HELPER(R)
SLMP_DEC_DEVICE_HELPER(ZR)

/** @brief Annunciator (F) helper. */
constexpr DeviceAddress FDevice(DecNo number) {
    return {DeviceCode::F, number.value};
}

#undef SLMP_DEC_DEVICE_HELPER
#undef SLMP_HEX_DEVICE_HELPER

/** @brief Create a block read descriptor. */
constexpr DeviceBlockRead blockRead(DeviceAddress device, uint16_t points) {
    return {device, points};
}

/** @brief Create a block write descriptor. */
constexpr DeviceBlockWrite blockWrite(DeviceAddress device, const uint16_t* values, uint16_t points) {
    return {device, values, points};
}

}  // namespace dev

/** @} */ // end of SLMP_Devices

/**
 * @defgroup SLMP_Transport Transport Interface
 * @brief Abstraction for communication layers.
 * @{
 */

/**
 * @struct TargetAddress
 * @brief SLMP target station routing information.
 */
struct TargetAddress {
    uint8_t network = 0x00;     ///< Network number (0=Local).
    uint8_t station = 0xFF;     ///< Station number (255=Control CPU).
    uint16_t module_io = 0x03FF; ///< Module I/O number (0x03FF=Own Station).
    uint8_t multidrop = 0x00;   ///< Multidrop station number.
};

/**
 * @struct TypeNameInfo
 * @brief PLC model and hardware information.
 */
struct TypeNameInfo {
    char model[17];             ///< Model name string (max 16 chars + null).
    uint16_t model_code;        ///< Internal model code.
    bool has_model_code;        ///< True if model code is valid.
};

/**
 * @struct ProfileRecommendation
 * @brief Recommended protocol settings based on PLC model.
 */
struct ProfileRecommendation {
    FrameType frame_type = FrameType::Frame4E;
    CompatibilityMode compatibility_mode = CompatibilityMode::iQR;
    ProfileClass profile_class = ProfileClass::Unknown;
    bool confident = false;     ///< True if recommendation is based on exact match.
};

/**
 * @class ITransport
 * @brief Abstract interface for the underlying transport layer (TCP/UDP/Serial).
 * 
 * Implement this interface to support custom communication stacks.
 * For Arduino, see @ref ArduinoClientTransport or @ref ArduinoUdpTransport.
 */
class ITransport {
  public:
    virtual ~ITransport() = default;

    /** @brief Connect to host. */
    virtual bool connect(const char* host, uint16_t port) = 0;
    /** @brief Close connection. */
    virtual void close() = 0;
    /** @brief Check connection status. */
    virtual bool connected() const = 0;
    /** @brief Block until all data is written. */
    virtual bool writeAll(const uint8_t* data, size_t length) = 0;
    /** @brief Block until exact length is read. */
    virtual bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) = 0;

    /** @brief Non-blocking write. */
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    /** @brief Non-blocking read. */
    virtual size_t read(uint8_t* data, size_t length) = 0;
    /** @brief Check pending read data length. */
    virtual size_t available() = 0;
};

/** @} */ // end of SLMP_Transport

/**
 * @class SlmpClient
 * @brief Main SLMP client implementation.
 * 
 * This class provides high-level device access methods. It is designed to be:
 * - **Memory Efficient**: No dynamic allocation. User provides buffers.
 * - **Flexible**: Supports synchronous (blocking) and asynchronous (non-blocking) calls.
 * - **Robust**: Validates buffer capacities and protocol state.
 * 
 * ### Synchronous Usage Example
 * @code
 * slmp::SlmpClient plc(transport, tx_buf, 256, rx_buf, 256);
 * uint16_t val;
 * if (plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), val) == slmp::Error::Ok) {
 *     // Success
 * }
 * @endcode
 * 
 * ### Asynchronous Usage Example
 * @code
 * if (plc.beginReadOneWord(slmp::dev::D(slmp::dev::dec(100)), val, millis()) == slmp::Error::Ok) {
 *     while(plc.isBusy()) {
 *         plc.update(millis());
 *     }
 * }
 * @endcode
 */
class SlmpClient {
  public:
    /**
     * @brief Initialize client with transport and buffers.
     * @param transport Reference to transport implementation (must remain valid).
     * @param tx_buffer Pointer to transmission buffer.
     * @param tx_capacity Capacity of tx_buffer in bytes.
     * @param rx_buffer Pointer to reception buffer.
     * @param rx_capacity Capacity of rx_buffer in bytes.
     */
    SlmpClient(
        ITransport& transport,
        uint8_t* tx_buffer,
        size_t tx_capacity,
        uint8_t* rx_buffer,
        size_t rx_capacity
    );

    /** @brief Connect transport to the specified PLC. */
    bool connect(const char* host, uint16_t port);
    /** @brief Close transport connection. */
    void close();
    /** @brief Check if transport is currently connected. */
    bool connected() const;

    /** @brief Set target station routing (e.g. for multi-network routing). */
    void setTarget(const TargetAddress& target);
    /** @brief Get current target station routing. */
    const TargetAddress& target() const;

    /** @brief Set frame format (3E/4E). Default is 4E. */
    void setFrameType(FrameType frame_type);
    /** @brief Get current frame format. */
    FrameType frameType() const;

    /** @brief Set device access mode (iQ-R/Legacy). Default is iQR. */
    void setCompatibilityMode(CompatibilityMode mode);
    /** @brief Get current compatibility mode. */
    CompatibilityMode compatibilityMode() const;

    /** @brief Set SLMP monitoring timer (units of 250ms). How long the PLC should wait for internal processing. */
    void setMonitoringTimer(uint16_t monitoring_timer);
    /** @brief Get current monitoring timer value. */
    uint16_t monitoringTimer() const;

    /** @brief Set internal transport timeout in milliseconds. */
    void setTimeoutMs(uint32_t timeout_ms);
    /** @brief Get current timeout value. */
    uint32_t timeoutMs() const;

    /** @brief Get the error code from the last operation. */
    Error lastError() const;
    /** @brief Get the PLC-specific end code from the last operation. Valid if lastError() is @ref Error::PlcError. */
    uint16_t lastEndCode() const;
    
    /** @brief Get pointer to the raw request frame buffer. */
    const uint8_t* lastRequestFrame() const;
    /** @brief Get the actual length of the last request frame. */
    size_t lastRequestFrameLength() const;
    /** @brief Get pointer to the raw response frame buffer. */
    const uint8_t* lastResponseFrame() const;
    /** @brief Get the actual length of the last response frame. */
    size_t lastResponseFrameLength() const;

    // --- Synchronous (Blocking) API ---

    /** @brief Read PLC model and type name (Model code and string). */
    Error readTypeName(TypeNameInfo& out);
    /** 
     * @brief Read contiguous word devices.
     * @param device Start address.
     * @param points Number of words to read.
     * @param values Buffer to store read values.
     * @param value_capacity Capacity of values buffer (in elements).
     */
    Error readWords(const DeviceAddress& device, uint16_t points, uint16_t* values, size_t value_capacity);
    /** @brief Write contiguous word devices. */
    Error writeWords(const DeviceAddress& device, const uint16_t* values, size_t count);
    /** @brief Read contiguous bit devices (booleans). */
    Error readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity);
    /** @brief Write contiguous bit devices (booleans). */
    Error writeBits(const DeviceAddress& device, const bool* values, size_t count);
    /** @brief Read contiguous 32-bit dwords (2 words per value). */
    Error readDWords(const DeviceAddress& device, uint16_t points, uint32_t* values, size_t value_capacity);
    /** @brief Write contiguous 32-bit dwords (2 words per value). */
    Error writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count);
    /** @brief Read contiguous IEEE-754 float32 values. */
    Error readFloat32s(const DeviceAddress& device, uint16_t points, float* values, size_t value_capacity);
    /** @brief Write contiguous IEEE-754 float32 values. */
    Error writeFloat32s(const DeviceAddress& device, const float* values, size_t count);

    /** @brief Read single word value helper. */
    Error readOneWord(const DeviceAddress& device, uint16_t& value);
    /** @brief Write single word value helper. */
    Error writeOneWord(const DeviceAddress& device, uint16_t value);
    /** @brief Read single bit value helper. */
    Error readOneBit(const DeviceAddress& device, bool& value);
    /** @brief Write single bit value helper. */
    Error writeOneBit(const DeviceAddress& device, bool value);
    /** @brief Read single 32-bit dword value helper. */
    Error readOneDWord(const DeviceAddress& device, uint32_t& value);
    /** @brief Write single 32-bit dword value helper. */
    Error writeOneDWord(const DeviceAddress& device, uint32_t value);
    /** @brief Read single IEEE-754 float32 value helper. */
    Error readOneFloat32(const DeviceAddress& device, float& value);
    /** @brief Write single IEEE-754 float32 value helper. */
    Error writeOneFloat32(const DeviceAddress& device, float value);

    /** 
     * @brief Perform random read of multiple word/dword devices.
     * Allows reading non-contiguous addresses in a single request.
     */
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
    /** @brief Perform random write of multiple word/dword devices. */
    Error writeRandomWords(
        const DeviceAddress* word_devices,
        const uint16_t* word_values,
        size_t word_count,
        const DeviceAddress* dword_devices,
        const uint32_t* dword_values,
        size_t dword_count
    );
    /** @brief Perform random write of multiple bit devices. */
    Error writeRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count);

    /** 
     * @brief Read multiple contiguous blocks in one request.
     * Blocks can be word-units or bit-units.
     */
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
    /** @brief Write multiple contiguous blocks in one request. */
    Error writeBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count
    );
    /** 
     * @brief Write multiple contiguous blocks with options.
     * Supports automatic splitting of mixed word/bit blocks if the PLC doesn't support them.
     */
    Error writeBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count,
        const BlockWriteOptions& options
    );

    /** @brief Remote RUN command. Set PLC to RUN state. */
    Error remoteRun(bool force = false, uint16_t clear_mode = 2U);
    /** @brief Remote STOP command. Set PLC to STOP state. */
    Error remoteStop();
    /** @brief Remote PAUSE command. Set PLC to PAUSE state. */
    Error remotePause(bool force = false);
    /** @brief Remote LATCH CLEAR command. */
    Error remoteLatchClear();
    /** @brief Remote RESET command. (Warning: Connection will likely be lost). */
    Error remoteReset(uint16_t subcommand = 0x0000U, bool expect_response = false);
    
    /** 
     * @brief Execute Self-test loopback.
     * Verifies communication path by having the PLC echo back the provided data.
     */
    Error selfTestLoopback(
        const uint8_t* data,
        size_t data_length,
        uint8_t* out,
        size_t out_capacity,
        size_t& out_length
    );
    
    /** @brief Clear PLC error state (Resets the error LED/status). */
    Error clearError();
    /** @brief Unlock remote password protected access. */
    Error remotePasswordUnlock(const char* password);
    /** @brief Lock remote password protected access. */
    Error remotePasswordLock(const char* password);

    // --- Asynchronous (Non-blocking) API ---

    /** 
     * @brief Advance asynchronous state machine.
     * Must be called frequently in your main loop to process pending I/O.
     * @param now_ms Current system time in milliseconds.
     */
    void update(uint32_t now_ms);
    /** @brief Check if an asynchronous operation is currently active. */
    bool isBusy() const;

    /** @brief Start async ReadTypeName. Result will be in @p out when busy becomes false. */
    Error beginReadTypeName(TypeNameInfo& out, uint32_t now_ms);
    /** @brief Start async ReadWords. Result will be in @p values when busy becomes false. */
    Error beginReadWords(const DeviceAddress& device, uint16_t points, uint16_t* values, size_t value_capacity, uint32_t now_ms);
    /** @brief Start async WriteWords. */
    Error beginWriteWords(const DeviceAddress& device, const uint16_t* values, size_t count, uint32_t now_ms);
    /** @brief Start async ReadBits. */
    Error beginReadBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity, uint32_t now_ms);
    /** @brief Start async WriteBits. */
    Error beginWriteBits(const DeviceAddress& device, const bool* values, size_t count, uint32_t now_ms);
    /** @brief Start async ReadDWords. */
    Error beginReadDWords(const DeviceAddress& device, uint16_t points, uint32_t* values, size_t value_capacity, uint32_t now_ms);
    /** @brief Start async WriteDWords. */
    Error beginWriteDWords(const DeviceAddress& device, const uint32_t* values, size_t count, uint32_t now_ms);
    /** @brief Start async ReadFloat32s. */
    Error beginReadFloat32s(const DeviceAddress& device, uint16_t points, float* values, size_t value_capacity, uint32_t now_ms);
    /** @brief Start async WriteFloat32s. */
    Error beginWriteFloat32s(const DeviceAddress& device, const float* values, size_t count, uint32_t now_ms);
    
    /** @brief Start async ReadRandom. */
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
    /** @brief Start async WriteRandomWords. */
    Error beginWriteRandomWords(
        const DeviceAddress* word_devices,
        const uint16_t* word_values,
        size_t word_count,
        const DeviceAddress* dword_devices,
        const uint32_t* dword_values,
        size_t dword_count,
        uint32_t now_ms
    );
    /** @brief Start async WriteRandomBits. */
    Error beginWriteRandomBits(const DeviceAddress* bit_devices, const bool* bit_values, size_t bit_count, uint32_t now_ms);
    /** @brief Start async ReadBlock. */
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
    /** @brief Start async WriteBlock. */
    Error beginWriteBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count,
        const BlockWriteOptions& options,
        uint32_t now_ms
    );
    /** @brief Start async RemoteRun. */
    Error beginRemoteRun(bool force, uint16_t clear_mode, uint32_t now_ms);
    /** @brief Start async RemoteStop. */
    Error beginRemoteStop(uint32_t now_ms);
    /** @brief Start async RemotePause. */
    Error beginRemotePause(bool force, uint32_t now_ms);
    /** @brief Start async RemoteLatchClear. */
    Error beginRemoteLatchClear(uint32_t now_ms);
    /** @brief Start async RemoteReset. */
    Error beginRemoteReset(uint16_t subcommand, bool expect_response, uint32_t now_ms);
    /** @brief Start async SelfTestLoopback. */
    Error beginSelfTestLoopback(
        const uint8_t* data,
        size_t data_length,
        uint8_t* out,
        size_t out_capacity,
        size_t* out_length,
        uint32_t now_ms
    );
    /** @brief Start async ClearError. */
    Error beginClearError(uint32_t now_ms);
    /** @brief Start async WriteBlock (simple version). */
    Error beginWriteBlock(
        const DeviceBlockWrite* word_blocks,
        size_t word_block_count,
        const DeviceBlockWrite* bit_blocks,
        size_t bit_block_count,
        uint32_t now_ms
    );
    /** @brief Start async RemotePasswordUnlock. */
    Error beginRemotePasswordUnlock(const char* password, uint32_t now_ms);
    /** @brief Start async RemotePasswordLock. */
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
            None, ReadTypeName, ReadWords, WriteWords, ReadBits, WriteBits,
            ReadDWords, WriteDWords, ReadFloat32s, WriteFloat32s, ReadRandom,
            WriteRandomWords, WriteRandomBits, ReadBlock, WriteBlock,
            RemoteRun, RemoteStop, RemotePause, RemoteLatchClear, RemoteReset,
            SelfTest, ClearError, PasswordUnlock, PasswordLock,
        };

        enum class WriteBlockStage : uint8_t {
            Direct, SplitWord, SplitBit,
        };

        Type type = Type::None;
        union {
            struct { void* values; uint16_t points; } common;
            struct { TypeNameInfo* out; } readTypeName;
            struct { uint16_t* word_values; uint16_t word_count; uint32_t* dword_values; uint16_t dword_count; } readRandom;
            struct { uint16_t* word_values; size_t total_word_points; uint16_t* bit_values; size_t total_bit_points; } readBlock;
            struct { uint16_t subcommand; bool expect_response; } remoteReset;
            struct { uint8_t* out; size_t out_capacity; size_t* out_length; } selfTest;
            struct {
                const DeviceBlockWrite* word_blocks;
                size_t word_block_count;
                const DeviceBlockWrite* bit_blocks;
                size_t bit_block_count;
                BlockWriteOptions options;
                WriteBlockStage stage;
                bool has_mixed_blocks;
            } writeBlock;
        } data;
    };

    Error startAsync(AsyncContext::Type type, size_t payload_length, uint32_t now_ms);
    Error beginWriteBlockRequest(
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
    );
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
    CompatibilityMode compatibility_mode_;
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

/**
 * @defgroup SLMP_Utils Utilities
 * @brief Helper functions for protocol management and debugging.
 * @{
 */

/** @brief Get descriptive string for @ref Error code. */
const char* errorString(Error error);
/** @brief Get descriptive string for SLMP end code (PLC error code). */
const char* endCodeString(uint16_t end_code);
/** @brief Get descriptive string for @ref ProfileClass. */
const char* profileClassString(ProfileClass profile_class);
/** @brief Automatically recommend protocol profile based on @ref TypeNameInfo. */
ProfileRecommendation recommendProfile(const TypeNameInfo& type_name);
/** @brief Automatically recommend protocol profile based on model name string. */
ProfileRecommendation recommendProfile(const char* model_name, uint16_t model_code = 0U, bool has_model_code = false);
/** @brief Apply recommended settings (FrameType, CompatibilityMode) to a client. */
void applyProfileRecommendation(SlmpClient& client, const ProfileRecommendation& recommendation);
/** @brief Helper to format bytes as a hex string (e.g. "01 02 AB"). Used for logging. */
size_t formatHexBytes(const uint8_t* data, size_t length, char* out, size_t out_capacity);

/** @} */ // end of SLMP_Utils

}  // namespace slmp

#endif
