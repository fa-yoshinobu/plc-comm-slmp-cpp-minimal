/**
 * @file slmp_minimal.h
 * @brief Ultra-lightweight SLMP (MC Protocol) Client for Embedded C++.
 * 
 * Provides a minimal, buffer-efficient implementation of the SLMP protocol
 * suitable for resource-constrained embedded systems like Arduino, ESP32, and RP2040.
 *
 * Design intent:
 * - keep the core API deterministic and buffer-oriented
 * - require caller-owned buffers instead of hidden heap allocation
 * - expose both blocking and cooperative non-blocking flows
 * - keep convenience helpers in @ref slmp_high_level.h so the core path stays small
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
 *
 * This is the low-level API surface. Use it when you want explicit
 * `DeviceAddress` values, caller-owned buffers, and direct control over
 * request scheduling and transport behavior.
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
 * This namespace is the recommended way to create @ref DeviceAddress values in
 * application code. It keeps the call site explicit about whether the PLC uses
 * decimal numbering or Mitsubishi hexadecimal numbering.
 *
 * Typical usage:
 * @code
 * auto d100 = slmp::dev::D(slmp::dev::dec(100));     // decimal-numbered D register
 * auto x1a  = slmp::dev::X(slmp::dev::hex(0x1A));    // hexadecimal-numbered X input
 * auto rd10 = slmp::dev::RD(slmp::dev::dec(10));     // refresh data register
 * auto ltn0 = slmp::dev::LTN(slmp::dev::dec(0));     // long timer current value
 * @endcode
 *
 * Device families exposed here match the generic direct-access helpers in
 * @ref SlmpClient. Extended devices such as `U\\G`, `U\\HG`, and `J\\device` use
 * @ref ExtDeviceSpec instead of these factory helpers.
 */
namespace dev {

/** @brief Wrapper type used to mark a device number as decimal. */
struct DecNo { uint32_t value; };
/** @brief Wrapper type used to mark a device number as hexadecimal. */
struct HexNo { uint32_t value; };

/**
 * @brief Mark a PLC device number as decimal.
 * @param value Decimal device number, for example `100` for `D100`.
 * @return Decimal-number wrapper consumed by the device helper factories.
 */
constexpr DecNo dec(uint32_t value) { return {value}; }
/**
 * @brief Mark a PLC device number as hexadecimal.
 * @param value Hexadecimal device number, for example `0x1A` for `X1A`.
 * @return Hexadecimal-number wrapper consumed by the device helper factories.
 */
constexpr HexNo hex(uint32_t value) { return {value}; }

#define SLMP_DEC_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(DecNo number) { \
        return {DeviceCode::name, number.value}; \
    }

#define SLMP_HEX_DEVICE_HELPER(name)           \
    constexpr DeviceAddress name(HexNo number) { \
        return {DeviceCode::name, number.value}; \
    }

/** @name Decimal Device Helpers
 * Helpers for devices that use Mitsubishi decimal numbering.
 *
 * This group includes standard relay/register devices, timer and counter
 * families, long timer / long retentive timer / long counter families, and
 * index/register families such as `Z`, `LZ`, `R`, `ZR`, and `RD`.
 *
 * Example:
 * @code
 * auto word = slmp::dev::D(slmp::dev::dec(100));
 * auto coil = slmp::dev::LCC(slmp::dev::dec(5));
 * auto reg  = slmp::dev::RD(slmp::dev::dec(10));
 * @endcode
 * @{
 */
SLMP_DEC_DEVICE_HELPER(SM)
SLMP_DEC_DEVICE_HELPER(SD)
SLMP_DEC_DEVICE_HELPER(M)
SLMP_DEC_DEVICE_HELPER(L)
SLMP_DEC_DEVICE_HELPER(V)
SLMP_DEC_DEVICE_HELPER(D)
SLMP_DEC_DEVICE_HELPER(TS)
SLMP_DEC_DEVICE_HELPER(TC)
SLMP_DEC_DEVICE_HELPER(TN)
SLMP_DEC_DEVICE_HELPER(LTS)
SLMP_DEC_DEVICE_HELPER(LTC)
SLMP_DEC_DEVICE_HELPER(LTN)
SLMP_DEC_DEVICE_HELPER(STS)
SLMP_DEC_DEVICE_HELPER(STC)
SLMP_DEC_DEVICE_HELPER(STN)
SLMP_DEC_DEVICE_HELPER(LSTS)
SLMP_DEC_DEVICE_HELPER(LSTC)
SLMP_DEC_DEVICE_HELPER(LSTN)
SLMP_DEC_DEVICE_HELPER(CS)
SLMP_DEC_DEVICE_HELPER(CC)
SLMP_DEC_DEVICE_HELPER(CN)
SLMP_DEC_DEVICE_HELPER(LCS)
SLMP_DEC_DEVICE_HELPER(LCC)
SLMP_DEC_DEVICE_HELPER(LCN)
SLMP_DEC_DEVICE_HELPER(Z)
SLMP_DEC_DEVICE_HELPER(LZ)
SLMP_DEC_DEVICE_HELPER(R)
SLMP_DEC_DEVICE_HELPER(ZR)
SLMP_DEC_DEVICE_HELPER(RD)
/** @} */

/** @name Hexadecimal Device Helpers
 * Helpers for devices that follow Mitsubishi hexadecimal numbering.
 *
 * Use these factories together with @ref hex. This applies to `X`, `Y`, `B`,
 * `W`, `SB`, `SW`, `DX`, and `DY`.
 *
 * Example:
 * @code
 * auto x1a = slmp::dev::X(slmp::dev::hex(0x1A));
 * auto w20 = slmp::dev::W(slmp::dev::hex(0x20));
 * @endcode
 * @{
 */
SLMP_HEX_DEVICE_HELPER(X)
SLMP_HEX_DEVICE_HELPER(Y)
SLMP_HEX_DEVICE_HELPER(B)
SLMP_HEX_DEVICE_HELPER(W)
SLMP_HEX_DEVICE_HELPER(SB)
SLMP_HEX_DEVICE_HELPER(SW)
SLMP_HEX_DEVICE_HELPER(DX)
SLMP_HEX_DEVICE_HELPER(DY)
/** @} */

/**
 * @brief Create an annunciator (`F`) device address.
 * @param number Decimal annunciator number such as `slmp::dev::dec(10)`.
 * @return Device address for `F<number>`.
 *
 * @details This helper is named `FDevice` instead of `F` because some
 * embedded toolchains define `F` as a macro.
 */
constexpr DeviceAddress FDevice(DecNo number) {
    return {DeviceCode::F, number.value};
}

#undef SLMP_DEC_DEVICE_HELPER
#undef SLMP_HEX_DEVICE_HELPER

/**
 * @brief Create a contiguous block-read descriptor.
 * @param device First device in the block.
 * @param points Number of points to read from @p device onward.
 * @return Descriptor suitable for @ref SlmpClient::readBlock and related APIs.
 */
constexpr DeviceBlockRead blockRead(DeviceAddress device, uint16_t points) {
    return {device, points};
}

/**
 * @brief Create a contiguous block-write descriptor.
 * @param device First device in the block.
 * @param values Caller-owned word values in PLC order.
 * @param points Number of points to write from @p values.
 * @return Descriptor suitable for @ref SlmpClient::writeBlock and related APIs.
 *
 * @note Bit-block writes also use word-sized values. Each element should be
 * either `0` or `1` when targeting bit devices.
 */
constexpr DeviceBlockWrite blockWrite(DeviceAddress device, const uint16_t* values, uint16_t points) {
    return {device, values, points};
}

}  // namespace dev

/**
 * @struct LongTimerResult
 * @brief Decoded result for one long timer or long retentive timer entry.
 *
 * Each entry occupies 4 words in the device memory:
 * [current_lo, current_hi, status_word, reserved].
 *
 * This structure is returned by the dedicated long timer helpers instead of
 * exposing the raw 4-word payload directly. The helper keeps the wire format
 * available through @ref status_word while also decoding the current value and
 * the commonly-used contact / coil bits.
 */
struct LongTimerResult {
    uint32_t current_value; ///< Current timer value assembled from `current_lo` and `current_hi`.
    bool contact;           ///< Contact state (`LTS` / `LSTS`, decoded from `status_word & 0x0002`).
    bool coil;              ///< Coil state (`LTC` / `LSTC`, decoded from `status_word & 0x0001`).
    uint16_t status_word;   ///< Raw status word returned by the PLC for callers that need bit-exact inspection.
};

/**
 * @struct LabelName
 * @brief UTF-16-LE label name for label commands.
 *
 * Use 2-byte characters as-is from the label string.
 * @code
 * static const uint16_t kMyLabel[] = {'V','a','r','1'};
 * slmp::LabelName name = { kMyLabel, 4 };
 * @endcode
 */
struct LabelName {
    const uint16_t* chars;  ///< UTF-16-LE characters
    uint16_t length;        ///< Number of characters (not bytes)
};

/**
 * @struct LabelArrayReadPoint
 * @brief Single label read entry for @ref SlmpClient::readArrayLabels.
 */
struct LabelArrayReadPoint {
    LabelName label;
    uint8_t unit_specification;   ///< 0=word (2 bytes/element), 1=byte (1 byte/element)
    uint16_t array_data_length;   ///< Number of elements
};

/**
 * @struct LabelArrayReadResult
 * @brief Single label read result from @ref SlmpClient::readArrayLabels.
 *
 * @note @p data points into the internal rx_buffer and is valid only until the next operation.
 */
struct LabelArrayReadResult {
    uint8_t dt_id;                ///< Data type identifier
    uint8_t unit_specification;   ///< 0=word, 1=byte
    uint16_t array_data_length;   ///< Number of elements
    const uint8_t* data;          ///< Pointer into rx_buffer (valid until next operation)
    size_t data_bytes;            ///< Byte length of data
};

/**
 * @struct LabelArrayWritePoint
 * @brief Single label write entry for @ref SlmpClient::writeArrayLabels.
 */
struct LabelArrayWritePoint {
    LabelName label;
    uint8_t unit_specification;   ///< 0=word, 1=byte
    uint16_t array_data_length;   ///< Number of elements
    const uint8_t* data;          ///< Data to write
    size_t data_bytes;            ///< Byte length of data
};

/**
 * @struct LabelRandomReadResult
 * @brief Single result from @ref SlmpClient::readRandomLabels.
 *
 * @note @p data points into the internal rx_buffer and is valid only until the next operation.
 */
struct LabelRandomReadResult {
    uint8_t dt_id;          ///< Data type identifier
    uint8_t spare;          ///< Reserved byte
    uint16_t result_length; ///< Byte length of data
    const uint8_t* data;    ///< Pointer into rx_buffer (valid until next operation)
};

/**
 * @struct LabelRandomWritePoint
 * @brief Single write entry for @ref SlmpClient::writeRandomLabels.
 */
struct LabelRandomWritePoint {
    LabelName label;
    const uint8_t* data;    ///< Data to write
    uint16_t data_bytes;    ///< Byte length of data
};

/**
 * @struct ExtDeviceSpec
 * @brief Extended device address for Ext variants (readRandomExt, etc.).
 *
 * Represents either a module buffer (`U&#92;G` / `U&#92;HG`) or link direct
 * (`J&#92;device`) target.
 * Use the static factory methods to construct.
 */
struct ExtDeviceSpec {
    enum class Kind : uint8_t { ModuleBuf, LinkDirect } kind;
    union {
        struct { uint16_t slot; bool use_hg; uint32_t dev_no; } mod;
        struct { uint8_t j_net; DeviceCode code; uint32_t dev_no; } link;
    };
    /** @brief Construct a module buffer device (`U&#92;G` or `U&#92;HG`). */
    static inline ExtDeviceSpec moduleBuf(uint16_t slot, bool use_hg, uint32_t dev_no) noexcept {
        ExtDeviceSpec s{}; s.kind = Kind::ModuleBuf; s.mod.slot = slot; s.mod.use_hg = use_hg; s.mod.dev_no = dev_no; return s;
    }
    /** @brief Construct a link direct device (`J&#92;device`). */
    static inline ExtDeviceSpec linkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no) noexcept {
        ExtDeviceSpec s{}; s.kind = Kind::LinkDirect; s.link.j_net = j_net; s.link.code = code; s.link.dev_no = dev_no; return s;
    }
};

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
 * @enum CpuOperationStatus
 * @brief Decoded CPU operation state from the lower 4 bits of SD203.
 */
enum class CpuOperationStatus : uint8_t {
    Unknown = 0xFF,
    Run = 0x00,
    Stop = 0x02,
    Pause = 0x03,
};

/**
 * @struct CpuOperationState
 * @brief Decoded CPU operation state read from SD203.
 */
struct CpuOperationState {
    CpuOperationStatus status = CpuOperationStatus::Unknown; ///< Decoded operation state.
    uint16_t raw_status_word = 0U; ///< Full raw word value read from SD203.
    uint8_t raw_code = 0U; ///< Lower 4-bit masked status code from SD203.
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
 * This class is the core low-level client. It is designed to be:
 * - **Memory Efficient**: No dynamic allocation. User provides buffers.
 * - **Flexible**: Supports synchronous (blocking) and asynchronous (non-blocking) calls.
 * - **Robust**: Validates buffer capacities and protocol state.
 *
 * Use this class directly when you want deterministic firmware behavior.
 * If you prefer string device addresses such as `D100` or `D200:F`, see the
 * optional helper facade in @ref slmp_high_level.h.
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

    /**
     * @brief Connect the configured transport to one PLC endpoint.
     * @param host PLC IP address or hostname.
     * @param port SLMP TCP or UDP port such as 1025.
     * @return True when the underlying transport reports a successful connect.
     *
     * Call this once before using the synchronous helper API such as
     * @ref readWords, @ref readOneDWord, or @ref writeOneFloat32.
     */
    bool connect(const char* host, uint16_t port);
    /** @brief Close the current transport connection and clear in-flight state. */
    void close();
    /** @brief Check whether the transport is currently connected. */
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

    /**
     * @brief Read PLC model information for profile selection and diagnostics.
     * @param out Receives the PLC model name and optional model code.
     * @return Operation result.
     */
    Error readTypeName(TypeNameInfo& out);
    /**
     * @brief Read SD203 and decode the CPU operation state from the lower 4 bits.
     * @param out Receives the decoded state and raw masked code.
     * @return Operation result.
     */
    Error readCpuOperationState(CpuOperationState& out);
    /** 
     * @brief Read contiguous word devices.
     * @param device Start address.
     * @param points Number of words to read.
     * @param values Buffer to store read values.
     * @param value_capacity Capacity of values buffer (in elements).
     */
    Error readWords(const DeviceAddress& device, uint16_t points, uint16_t* values, size_t value_capacity);
    /**
     * @brief Write a contiguous word-device range.
     * @param device First word device in the range.
     * @param values Word values in PLC order.
     * @param count Number of words to write.
     * @return Operation result.
     */
    Error writeWords(const DeviceAddress& device, const uint16_t* values, size_t count);
    /**
     * @brief Read a contiguous bit-device range as boolean values.
     * @param device First bit device in the range.
     * @param points Number of bit points to read.
     * @param values Output buffer for the returned bit states.
     * @param value_capacity Capacity of @p values in elements.
     * @return Operation result.
     */
    Error readBits(const DeviceAddress& device, uint16_t points, bool* values, size_t value_capacity);
    /**
     * @brief Write a contiguous bit-device range from boolean values.
     * @param device First bit device in the range.
     * @param values Bit values in PLC order.
     * @param count Number of bit points to write.
     * @return Operation result.
     */
    Error writeBits(const DeviceAddress& device, const bool* values, size_t count);
    /**
     * @brief Read a contiguous DWord range as unsigned 32-bit values.
     * @param device First device in the range.
     * @param points Number of 32-bit values to read.
     * @param values Output buffer for the returned values.
     * @param value_capacity Capacity of @p values in elements.
     * @return Operation result.
     */
    Error readDWords(const DeviceAddress& device, uint16_t points, uint32_t* values, size_t value_capacity);
    /**
     * @brief Write a contiguous DWord range from unsigned 32-bit values.
     * @param device First device in the range.
     * @param values 32-bit values in PLC order.
     * @param count Number of 32-bit values to write.
     * @return Operation result.
     */
    Error writeDWords(const DeviceAddress& device, const uint32_t* values, size_t count);
    /**
     * @brief Read a contiguous range of IEEE-754 float32 values.
     * @param device First device in the range.
     * @param points Number of float32 values to read.
     * @param values Output buffer for the returned values.
     * @param value_capacity Capacity of @p values in elements.
     * @return Operation result.
     */
    Error readFloat32s(const DeviceAddress& device, uint16_t points, float* values, size_t value_capacity);
    /**
     * @brief Write a contiguous range of IEEE-754 float32 values.
     * @param device First device in the range.
     * @param values Float32 values in PLC order.
     * @param count Number of float32 values to write.
     * @return Operation result.
     */
    Error writeFloat32s(const DeviceAddress& device, const float* values, size_t count);

    /** @brief Read one 16-bit word value through the synchronous helper API. */
    Error readOneWord(const DeviceAddress& device, uint16_t& value);
    /** @brief Write one 16-bit word value through the synchronous helper API. */
    Error writeOneWord(const DeviceAddress& device, uint16_t value);
    /** @brief Read one direct bit-device value through the synchronous helper API. */
    Error readOneBit(const DeviceAddress& device, bool& value);
    /** @brief Write one direct bit-device value through the synchronous helper API. */
    Error writeOneBit(const DeviceAddress& device, bool value);
    /** @brief Read one unsigned 32-bit value through the synchronous helper API. */
    Error readOneDWord(const DeviceAddress& device, uint32_t& value);
    /** @brief Write one unsigned 32-bit value through the synchronous helper API. */
    Error writeOneDWord(const DeviceAddress& device, uint32_t value);
    /** @brief Read one IEEE-754 float32 value through the synchronous helper API. */
    Error readOneFloat32(const DeviceAddress& device, float& value);
    /** @brief Write one IEEE-754 float32 value through the synchronous helper API. */
    Error writeOneFloat32(const DeviceAddress& device, float value);

    /**
     * @brief Perform random read of multiple word and dword devices.
     *
     * Allows reading non-contiguous addresses in one request. This is one of
     * the main low-level batching primitives behind the optional high-level
     * snapshot helpers.
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
    /**
     * @brief Perform random write of multiple word and dword devices.
     *
     * This is the low-level counterpart to mixed typed writes in the optional
     * helper layer.
     */
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

    // --- Extended random read / write (Extended Device: module buffer or link direct) ---

    /** @brief Random read using extended device specs (module buffer or link direct). */
    Error readRandomExt(
        const ExtDeviceSpec* word_devices, size_t word_count,
        uint16_t* word_values, size_t word_value_capacity,
        const ExtDeviceSpec* dword_devices, size_t dword_count,
        uint32_t* dword_values, size_t dword_value_capacity
    );
    /** @brief Random write words using extended device specs. */
    Error writeRandomWordsExt(
        const ExtDeviceSpec* word_devices, const uint16_t* word_values, size_t word_count,
        const ExtDeviceSpec* dword_devices, const uint32_t* dword_values, size_t dword_count
    );
    /** @brief Random write bits using extended device specs. */
    Error writeRandomBitsExt(const ExtDeviceSpec* bit_devices, const bool* bit_values, size_t bit_count);

    // --- Monitor (0x0801 / 0x0802) ---

    /**
     * @brief Register devices for monitoring (command 0x0801).
     *
     * Register once, then call @ref runMonitorCycle repeatedly to read current
     * values without rebuilding the monitor list every cycle.
     */
    Error registerMonitorDevices(
        const DeviceAddress* word_devices, size_t word_count,
        const DeviceAddress* dword_devices, size_t dword_count
    );
    /** @brief Register devices for monitoring using extended device specs (command 0x0801). */
    Error registerMonitorDevicesExt(
        const ExtDeviceSpec* word_devices, size_t word_count,
        const ExtDeviceSpec* dword_devices, size_t dword_count
    );
    /**
     * @brief Execute monitor cycle (command 0x0802).
     *
     * Returns values for the devices previously registered by
     * @ref registerMonitorDevices or @ref registerMonitorDevicesExt.
     */
    Error runMonitorCycle(
        uint16_t* word_values, uint16_t word_count,
        uint32_t* dword_values, uint16_t dword_count
    );

    // --- Profile probing ---

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
    /** @brief Start async ReadRandomExt. */
    Error beginReadRandomExt(
        const ExtDeviceSpec* word_devices, size_t word_count,
        uint16_t* word_values, size_t word_value_capacity,
        const ExtDeviceSpec* dword_devices, size_t dword_count,
        uint32_t* dword_values, size_t dword_value_capacity,
        uint32_t now_ms
    );
    /** @brief Start async WriteRandomWordsExt. */
    Error beginWriteRandomWordsExt(
        const ExtDeviceSpec* word_devices, const uint16_t* word_values, size_t word_count,
        const ExtDeviceSpec* dword_devices, const uint32_t* dword_values, size_t dword_count,
        uint32_t now_ms
    );
    /** @brief Start async WriteRandomBitsExt. */
    Error beginWriteRandomBitsExt(const ExtDeviceSpec* bit_devices, const bool* bit_values, size_t bit_count, uint32_t now_ms);
    /** @brief Start async RegisterMonitorDevices. */
    Error beginRegisterMonitorDevices(
        const DeviceAddress* word_devices, size_t word_count,
        const DeviceAddress* dword_devices, size_t dword_count,
        uint32_t now_ms
    );
    /** @brief Start async RegisterMonitorDevicesExt. */
    Error beginRegisterMonitorDevicesExt(
        const ExtDeviceSpec* word_devices, size_t word_count,
        const ExtDeviceSpec* dword_devices, size_t dword_count,
        uint32_t now_ms
    );
    /** @brief Start async RunMonitorCycle. */
    Error beginRunMonitorCycle(
        uint16_t* word_values, uint16_t word_count,
        uint32_t* dword_values, uint16_t dword_count,
        uint32_t now_ms
    );
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

    // --- Long timer / long retentive timer helpers (iQ-R, command 0x0401) ---

    /**
     * @brief Read one or more long timers (LTN device, 4 words per entry).
     * @param head_no Starting LTN device number (e.g. 0 for LTN0).
     * @param points Number of timers to read.
     * @param out Output buffer for decoded results.
     * @param capacity Capacity of out buffer (in elements).
     */
    Error readLongTimer(int head_no, int points, LongTimerResult* out, size_t capacity);
    /** @brief Start async readLongTimer. */
    Error beginReadLongTimer(int head_no, int points, LongTimerResult* out, size_t capacity, uint32_t now_ms);

    /** @brief Read one or more long retentive timers (LSTN device, 4 words per entry). */
    Error readLongRetentiveTimer(int head_no, int points, LongTimerResult* out, size_t capacity);
    /** @brief Start async readLongRetentiveTimer. */
    Error beginReadLongRetentiveTimer(int head_no, int points, LongTimerResult* out, size_t capacity, uint32_t now_ms);

    /** @brief Read coil states (LTC) for long timers at headNo..headNo+points-1. */
    Error readLtcStates(int head_no, int points, bool* out, size_t capacity);
    /** @brief Read contact states (LTS) for long timers. */
    Error readLtsStates(int head_no, int points, bool* out, size_t capacity);
    /** @brief Read coil states (LSTC) for long retentive timers. */
    Error readLstcStates(int head_no, int points, bool* out, size_t capacity);
    /** @brief Read contact states (LSTS) for long retentive timers. */
    Error readLstsStates(int head_no, int points, bool* out, size_t capacity);

    // --- Module Buffer (Intelligent Module U/G or U/HG, command 0x0401 sub 0x0080/0x0082) ---

    /**
     * @brief Read words from intelligent module buffer memory.
     * @param slot Module slot number in hex (e.g. 3 for U3, 0x3E0 for U3E0).
     * @param use_hg true for HG (extended buffer), false for G (normal buffer).
     * @param dev_no Device number (buffer memory address).
     * @param points Number of words to read.
     * @param out Output buffer.
     * @param capacity Capacity of out buffer (in elements).
     */
    Error readWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity);
    /** @brief Start async readWordsModuleBuf. */
    Error beginReadWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity, uint32_t now_ms);
    /** @brief Write words to intelligent module buffer memory. */
    Error writeWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t* values, size_t count);
    /** @brief Start async writeWordsModuleBuf. */
    Error beginWriteWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t* values, size_t count, uint32_t now_ms);
    /** @brief Read bits from intelligent module buffer memory (sub 0x0081/0x0083). */
    Error readBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool* out, size_t capacity);
    /** @brief Start async readBitsModuleBuf. */
    Error beginReadBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool* out, size_t capacity, uint32_t now_ms);
    /** @brief Write bits to intelligent module buffer memory (sub 0x0081/0x0083). */
    Error writeBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool* values, size_t count);
    /** @brief Start async writeBitsModuleBuf. */
    Error beginWriteBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool* values, size_t count, uint32_t now_ms);

    // --- Link Direct Device (CC-Link IE J/device, command 0x0401 sub 0x0080/0x0081) ---

    /**
     * @brief Read words from a link direct device (CC-Link IE `J&#92;device`).
     * @param j_net J-network number (0..255).
     * @param code Device code.
     * @param dev_no Device number.
     * @param points Number of words to read.
     * @param out Output buffer.
     * @param capacity Capacity of out buffer (in elements).
     */
    Error readWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity);
    /** @brief Start async readWordsLinkDirect. */
    Error beginReadWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity, uint32_t now_ms);
    /** @brief Write words to a link direct device. */
    Error writeWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t* values, size_t count);
    /** @brief Start async writeWordsLinkDirect. */
    Error beginWriteWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t* values, size_t count, uint32_t now_ms);
    /** @brief Read bits from a link direct device (16-point units). */
    Error readBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool* out, size_t capacity);
    /** @brief Start async readBitsLinkDirect. */
    Error beginReadBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool* out, size_t capacity, uint32_t now_ms);
    /** @brief Write bits to a link direct device (16-point units). */
    Error writeBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool* values, size_t count);
    /** @brief Start async writeBitsLinkDirect. */
    Error beginWriteBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool* values, size_t count, uint32_t now_ms);

    // --- Memory Read / Write (command 0x0613 / 0x1613) ---

    /**
     * @brief Read words from PLC memory (command 0x0613).
     * @param head_address 32-bit starting memory address.
     * @param word_length Number of words to read.
     * @param out Output buffer.
     * @param capacity Capacity of out buffer (in elements).
     */
    Error readMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity);
    /** @brief Start async readMemoryWords. */
    Error beginReadMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity, uint32_t now_ms);
    /** @brief Write words to PLC memory (command 0x1613). */
    Error writeMemoryWords(uint32_t head_address, const uint16_t* values, size_t count);
    /** @brief Start async writeMemoryWords. */
    Error beginWriteMemoryWords(uint32_t head_address, const uint16_t* values, size_t count, uint32_t now_ms);

    // --- Extend Unit Read / Write (command 0x0601 / 0x1601) ---

    /**
     * @brief Read raw bytes from an extend unit (command 0x0601).
     * @param head_address 32-bit starting address.
     * @param byte_length Number of bytes to read.
     * @param module_no Extend unit module I/O number (e.g. 0x03E0 for CPU buffer).
     * @param out Output buffer.
     * @param capacity Capacity of out buffer in bytes.
     */
    Error readExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t* out, size_t capacity);
    /** @brief Start async readExtendUnitBytes. */
    Error beginReadExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t* out, size_t capacity, uint32_t now_ms);
    /** @brief Read words from an extend unit (command 0x0601). */
    Error readExtendUnitWords(uint32_t head_address, uint16_t word_length, uint16_t module_no, uint16_t* out, size_t capacity);
    /** @brief Read single word from an extend unit. */
    Error readExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t& value);
    /** @brief Read single DWord (2 words, little-endian) from an extend unit. */
    Error readExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t& value);
    /** @brief Write raw bytes to an extend unit (command 0x1601). */
    Error writeExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t* data, size_t byte_length);
    /** @brief Start async writeExtendUnitBytes. */
    Error beginWriteExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t* data, size_t byte_length, uint32_t now_ms);
    /** @brief Write words to an extend unit (command 0x1601). */
    Error writeExtendUnitWords(uint32_t head_address, uint16_t module_no, const uint16_t* values, size_t count);
    /** @brief Write single word to an extend unit. */
    Error writeExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t value);
    /** @brief Write single DWord (2 words, little-endian) to an extend unit. */
    Error writeExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t value);

    // --- CPU Buffer convenience wrappers (extend unit module 0x03E0) ---

    /** @brief Read bytes from the CPU buffer (extend unit 0x03E0). */
    Error readCpuBufferBytes(uint32_t head_address, uint16_t byte_length, uint8_t* out, size_t capacity);
    /** @brief Read words from the CPU buffer (extend unit 0x03E0). */
    Error readCpuBufferWords(uint32_t head_address, uint16_t word_length, uint16_t* out, size_t capacity);
    /** @brief Read single word from the CPU buffer. */
    Error readCpuBufferWord(uint32_t head_address, uint16_t& value);
    /** @brief Read single DWord (2 words, little-endian) from the CPU buffer. */
    Error readCpuBufferDWord(uint32_t head_address, uint32_t& value);
    /** @brief Write bytes to the CPU buffer (extend unit 0x03E0). */
    Error writeCpuBufferBytes(uint32_t head_address, const uint8_t* data, size_t byte_length);
    /** @brief Write words to the CPU buffer (extend unit 0x03E0). */
    Error writeCpuBufferWords(uint32_t head_address, const uint16_t* values, size_t count);
    /** @brief Write single word to the CPU buffer. */
    Error writeCpuBufferWord(uint32_t head_address, uint16_t value);
    /** @brief Write single DWord (2 words, little-endian) to the CPU buffer. */
    Error writeCpuBufferDWord(uint32_t head_address, uint32_t value);

    // --- Label Read / Write (commands 0x041A / 0x141A / 0x041C / 0x141B) ---

    /**
     * @brief Read array labels from the PLC (command 0x041A).
     * @param points Array of read point descriptors.
     * @param point_count Number of points.
     * @param out Output buffer for results. result.data pointers are valid until the next operation.
     * @param out_capacity Capacity of out buffer (in elements).
     * @param out_count Receives the number of results written.
     * @param abbrevs Optional abbreviation label names (may be nullptr).
     * @param abbrev_count Number of abbreviation labels.
     */
    Error readArrayLabels(
        const LabelArrayReadPoint* points, size_t point_count,
        LabelArrayReadResult* out, size_t out_capacity, size_t* out_count,
        const LabelName* abbrevs = nullptr, size_t abbrev_count = 0
    );
    /** @brief Start async readArrayLabels. */
    Error beginReadArrayLabels(
        const LabelArrayReadPoint* points, size_t point_count,
        LabelArrayReadResult* out, size_t out_capacity, size_t* out_count,
        const LabelName* abbrevs, size_t abbrev_count,
        uint32_t now_ms
    );

    /** @brief Write array labels to the PLC (command 0x141A). */
    Error writeArrayLabels(
        const LabelArrayWritePoint* points, size_t point_count,
        const LabelName* abbrevs = nullptr, size_t abbrev_count = 0
    );
    /** @brief Start async writeArrayLabels. */
    Error beginWriteArrayLabels(
        const LabelArrayWritePoint* points, size_t point_count,
        const LabelName* abbrevs, size_t abbrev_count,
        uint32_t now_ms
    );

    /**
     * @brief Read random labels from the PLC (command 0x041C).
     * @param labels Array of label names.
     * @param label_count Number of labels.
     * @param out Output buffer for results.
     * @param out_capacity Capacity of out buffer.
     * @param out_count Receives the number of results written.
     * @param abbrevs Optional abbreviation labels.
     * @param abbrev_count Number of abbreviation labels.
     */
    Error readRandomLabels(
        const LabelName* labels, size_t label_count,
        LabelRandomReadResult* out, size_t out_capacity, size_t* out_count,
        const LabelName* abbrevs = nullptr, size_t abbrev_count = 0
    );
    /** @brief Start async readRandomLabels. */
    Error beginReadRandomLabels(
        const LabelName* labels, size_t label_count,
        LabelRandomReadResult* out, size_t out_capacity, size_t* out_count,
        const LabelName* abbrevs, size_t abbrev_count,
        uint32_t now_ms
    );

    /** @brief Write random labels to the PLC (command 0x141B). */
    Error writeRandomLabels(
        const LabelRandomWritePoint* points, size_t point_count,
        const LabelName* abbrevs = nullptr, size_t abbrev_count = 0
    );
    /** @brief Start async writeRandomLabels. */
    Error beginWriteRandomLabels(
        const LabelRandomWritePoint* points, size_t point_count,
        const LabelName* abbrevs, size_t abbrev_count,
        uint32_t now_ms
    );

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
            // Extended operations
            ReadLongTimer,          ///< Reads LTN/LSTN words and decodes to LongTimerResult[]
            ReadModuleBufWords,     ///< Extended device read (module buffer), word subcommand (0x0080/0x0082)
            WriteModuleBufWords,    ///< Extended device write (module buffer), word subcommand (0x0080/0x0082)
            ReadModuleBufBits,      ///< Extended device read (module buffer), bit subcommand (0x0081/0x0083)
            WriteModuleBufBits,     ///< Extended device write (module buffer), bit subcommand (0x0081/0x0083)
            ReadLinkDirectWords,    ///< Link direct read, sub 0x0080
            WriteWordsLinkDirect,   ///< Link direct write words, sub 0x0080
            ReadLinkDirectBits,     ///< Link direct read bits, sub 0x0081
            WriteBitsLinkDirect,    ///< Link direct write bits, sub 0x0081
            ReadMemoryWords,        ///< Memory read (0x0613)
            WriteMemoryWords,       ///< Memory write (0x1613)
            ReadExtendUnitBytes,    ///< Extend unit read (0x0601)
            WriteExtendUnitBytes,   ///< Extend unit write (0x1601)
            ReadArrayLabels,           ///< Label array read (0x041A)
            WriteArrayLabels,          ///< Label array write (0x141A)
            ReadRandomLabels,          ///< Label random read (0x041C)
            WriteRandomLabels,         ///< Label random write (0x141B)
            ReadRandomExt,             ///< Extended random read (0x0403 sub 0x0080/0x0082)
            WriteRandomWordsExt,       ///< Extended random write words (0x1402 sub 0x0080/0x0082)
            WriteRandomBitsExt,        ///< Extended random write bits (0x1402 sub 0x0081/0x0083)
            RegisterMonitorDevices,    ///< Register monitor devices (0x0801 sub 0x0000/0x0002)
            RegisterMonitorDevicesExt, ///< Register monitor devices ext (0x0801 sub 0x0080/0x0082)
            RunMonitorCycle,           ///< Run monitor cycle (0x0802)
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
            struct { LongTimerResult* out; uint16_t points; } readLongTimer;
            struct { LabelArrayReadResult* out; size_t count; size_t capacity; size_t* out_count; } readArrayLabels;
            struct { LabelRandomReadResult* out; size_t count; size_t capacity; size_t* out_count; } readRandomLabels;
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
/** @brief Helper to format bytes as a hex string (e.g. "01 02 AB"). Used for logging. */
size_t formatHexBytes(const uint8_t* data, size_t length, char* out, size_t out_capacity);

/** @} */ // end of SLMP_Utils

}  // namespace slmp

#endif
