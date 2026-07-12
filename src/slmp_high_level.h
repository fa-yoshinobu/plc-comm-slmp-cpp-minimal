/**
 * @file slmp_high_level.h
 * @brief Optional high-level helper layer for the minimal SLMP client.
 *
 * This header adds a user-facing facade similar to the Python and .NET helper
 * APIs while keeping the core transport and codec in @ref slmp_minimal.h
 * unchanged. The high-level layer accepts string device addresses, converts
 * them into typed values, and can compile mixed snapshot plans for repeated
 * reads.
 *
 * Design intent:
 * - keep @ref slmp_minimal.h deterministic and buffer-oriented
 * - add convenience only in this optional layer
 * - allow applications to start with string addresses and typed values
 * - keep it easy to drop back to the core API when tighter control is needed
 */

#ifndef SLMP_HIGH_LEVEL_H
#define SLMP_HIGH_LEVEL_H

#ifndef SLMP_MINIMAL_ENABLE_HIGH_LEVEL
#define SLMP_MINIMAL_ENABLE_HIGH_LEVEL 1
#endif

#if SLMP_MINIMAL_ENABLE_HIGH_LEVEL

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "slmp_minimal.h"

namespace slmp {
namespace highlevel {

/**
 * @defgroup SLMP_HighLevel High-Level Helper API
 * @brief User-facing helpers layered on top of @ref slmp::SlmpClient.
 * @{
 */

/**
 * @enum ValueType
 * @brief Logical value kind used by the high-level facade.
 */
enum class ValueType : uint8_t {
    Bit,        ///< One logical bit.
    U16,        ///< Unsigned 16-bit integer.
    S16,        ///< Signed 16-bit integer.
    U32,        ///< Unsigned 32-bit integer.
    S32,        ///< Signed 32-bit integer.
    Float32,    ///< IEEE-754 float32 value.
};

/**
 * @struct Value
 * @brief Tagged logical value used by typed read, write, and named snapshots.
 *
 * Only the field that matches @ref type is meaningful. This struct is
 * intentionally simple so it can be stack-allocated, copied into snapshots,
 * and passed through firmware code without pulling in a heavier variant type.
 */
struct Value {
    ValueType type = ValueType::U16; ///< Declared logical type of this value.
    bool bit = false;                ///< Boolean payload for @ref ValueType::Bit.
    uint16_t u16 = 0U;               ///< Unsigned 16-bit payload.
    int16_t s16 = 0;                 ///< Signed 16-bit payload.
    uint32_t u32 = 0UL;              ///< Unsigned 32-bit payload.
    int32_t s32 = 0L;                ///< Signed 32-bit payload.
    float f32 = 0.0f;                ///< Float32 payload.

    /**
     * @brief Create a boolean logical value.
     * @param value Boolean payload.
     * @return Tagged @ref Value initialized as @ref ValueType::Bit.
     */
    static Value bitValue(bool value);
    /**
     * @brief Create an unsigned 16-bit logical value.
     * @param value Unsigned 16-bit payload.
     * @return Tagged @ref Value initialized as @ref ValueType::U16.
     */
    static Value u16Value(uint16_t value);
    /**
     * @brief Create a signed 16-bit logical value.
     * @param value Signed 16-bit payload.
     * @return Tagged @ref Value initialized as @ref ValueType::S16.
     */
    static Value s16Value(int16_t value);
    /**
     * @brief Create an unsigned 32-bit logical value.
     * @param value Unsigned 32-bit payload.
     * @return Tagged @ref Value initialized as @ref ValueType::U32.
     */
    static Value u32Value(uint32_t value);
    /**
     * @brief Create a signed 32-bit logical value.
     * @param value Signed 32-bit payload.
     * @return Tagged @ref Value initialized as @ref ValueType::S32.
     */
    static Value s32Value(int32_t value);
    /**
     * @brief Create an IEEE-754 float32 logical value.
     * @param value Float payload.
     * @return Tagged @ref Value initialized as @ref ValueType::Float32.
     */
    static Value float32Value(float value);
};

/**
 * @struct AddressSpec
 * @brief Parsed logical address accepted by the high-level helpers.
 *
 * Examples:
 * - `D100:U`
 * - `D100:S`
 * - `D200:F`
 * - `D50.3`
 * - `M1000:BIT`
 *
 * Device families accepted by the parser include:
 * - standard relay/register devices such as `D`, `M`, `X`, `Y`, `R`, `ZR`
 * - long timer families `LTN`, `LTS`, `LTC`
 * - long retentive timer families `LSTN`, `LSTS`, `LSTC`
 * - long counter families `LCN`, `LCS`, `LCC`
 * - index/register families `Z`, `LZ`, and `RD`
 *
 * The `.bit` notation is valid only for word devices and maps to a
 * read-modify-write flow on writes.
 */
struct AddressSpec {
    DeviceAddress device{PlcProfile::Unspecified, DeviceCode::D, 0U}; ///< Populated by the profile-required parser.
    ValueType type = ValueType::U16; ///< Logical value type requested by the caller.
    int bit_index = -1;          ///< Bit index for `D50.3` style access, otherwise -1.
    bool explicit_type = false;  ///< True when the input used an explicit suffix such as `:F`.
};

/**
 * @enum BatchKind
 * @brief Read-plan batching strategy for one named address.
 */
enum class BatchKind : uint8_t {
    None,       ///< Not eligible for the single-request named-read contract.
    Word,       ///< Read through batched random word access.
    Dword,      ///< Read through batched random dword access.
    BitInWord,  ///< Read one bit from a batched word value.
    LongTimer,  ///< Long-family route; rejected by the single-request named-read contract.
};

/**
 * @struct ReadPlanEntry
 * @brief One compiled entry in a named-read or poll plan.
 *
 * A plan entry keeps both the original caller string and the normalized
 * low-level interpretation so repeated polling does not need to re-parse the
 * address expression.
 */
struct ReadPlanEntry {
    std::string address; ///< Original caller-provided address string.
    AddressSpec spec;    ///< Parsed logical address.
    BatchKind kind = BatchKind::None; ///< Batching strategy selected for this address.
};

/**
 * @struct ReadPlan
 * @brief Compiled snapshot plan reused by @ref readNamed and @ref Poller.
 *
 * The `entries` vector preserves caller order for user-facing results. The
 * `word_devices` and `dword_devices` vectors hold deduplicated batchable
 * devices in the order chosen by the compiler so the runtime can perform one
 * random-read request for many named addresses.
 * Hand-built plans are validated at execution; an entry absent from its batch
 * vector is rejected instead of being synthesized as value zero.
 */
struct ReadPlan {
    PlcProfile profile = PlcProfile::Unspecified; ///< Profile used to compile every address in this plan.
    std::vector<ReadPlanEntry> entries;   ///< Snapshot entries in caller order.
    std::vector<DeviceAddress> word_devices;  ///< Unique batched word devices.
    std::vector<DeviceAddress> dword_devices; ///< Unique batched dword devices.
};

/**
 * @struct NamedValue
 * @brief One result item in a named snapshot.
 */
struct NamedValue {
    std::string address; ///< Original caller-provided address string.
    Value value;         ///< Decoded logical value.
};

/**
 * @brief Ordered list returned by @ref readNamed and consumed by @ref writeNamed.
 *
 * The order matches the caller-provided address order. This makes the
 * structure convenient for result presentation and for one-request named
 * writes. Word and DWord values may share one random-write request; bit values
 * use one random-bit request. Mixed command families and bit-in-word
 * read-modify-write addresses are rejected before transport.
 */
using Snapshot = std::vector<NamedValue>;

using PlcProfile = ::slmp::PlcProfile;

/**
 * @struct PlcProfileDefaults
 * @brief Resolved fixed defaults for one @ref PlcProfile.
 */
struct PlcProfileDefaults {
    FrameType frame_type;                     ///< Derived SLMP frame type.
    CompatibilityMode compatibility_mode;     ///< Derived low-level compatibility mode.
    PlcProfile address_profile;               ///< String-address profile used by the helper parser.
    PlcProfile range_profile;                 ///< PLC profile used for SD-range helpers.
};

/**
 * @struct PlcProfileDescriptor
 * @brief Canonical metadata used to select and describe one PLC profile.
 *
 * A null @ref base_profile means that the canonical profile has no declared
 * base profile.
 */
struct PlcProfileDescriptor {
    const char* canonical_name;  ///< Stable configuration identifier.
    const char* display_name;    ///< Human-readable UI label.
    bool connectable;            ///< True when standard connection helpers accept it.
    const char* base_profile;    ///< Canonical base profile, or null.
};

/**
 * @brief Return the canonical lowercase profile name.
 * @param family Selected PLC profile.
 * @return Profile name such as `melsec:iq-f`.
 */
const char* plcProfileCanonicalName(PlcProfile family);

/**
 * @brief Parse one canonical profile name.
 * @param text Canonical profile name such as `melsec:iq-r`.
 * @param out_profile Receives the parsed profile.
 * @return @ref Error::Ok on success, or @ref Error::InvalidArgument for null,
 * unknown, or unspecified names.
 */
Error parsePlcProfile(const char* text, PlcProfile& out_profile);

/**
 * @brief Return the connection-selectable built-in profiles.
 * @param count Receives the number of entries in the returned array.
 * @return Pointer to a stable, library-owned array. The array excludes
 * `Unspecified` and the abstract `melsec:qcpu` base profile.
 */
const PlcProfile* availablePlcProfiles(size_t& count);

/**
 * @brief Return all canonical profiles with display, connection, and base-profile metadata.
 * @param count Receives the number of descriptors in the returned array.
 * @return Pointer to a stable, library-owned descriptor array. The abstract
 * `melsec:qcpu` entry is included with @ref PlcProfileDescriptor::connectable
 * set to false.
 */
const PlcProfileDescriptor* plcProfileDescriptors(size_t& count);

/**
 * @brief Return the canonical human-readable display name for a PLC profile.
 * @param family Selected PLC profile.
 * @return Profile display name such as `MELSEC iQ-R (built-in)`.
 */
const char* plcProfileDisplayName(PlcProfile family);

/**
 * @brief Resolve the fixed defaults for one PLC profile.
 * @param family Selected PLC profile.
 * @return Derived frame, compatibility mode, address profile, and range profile.
 */
PlcProfileDefaults plcProfileDefaults(PlcProfile family);

/**
 * @enum DeviceRangeCategory
 * @brief Logical grouping for one device-range entry.
 */
enum class DeviceRangeCategory : uint8_t {
    Bit,
    Word,
    TimerCounter,
    Index,
    FileRegister,
};

/**
 * @enum DeviceRangeNotation
 * @brief Rendered public address numbering style for one PLC profile.
 */
enum class DeviceRangeNotation : uint8_t {
    Base10,
    Base8,
    Base16,
};

/**
 * @struct DeviceRangeEntry
 * @brief One device-range row for one public device code such as `D` or `STS`.
 */
struct DeviceRangeEntry {
    std::string device;                 ///< Public device code such as `D` or `X`.
    DeviceRangeCategory category = DeviceRangeCategory::Word; ///< Logical category for grouping.
    bool is_bit_device = false;         ///< True when this device is normally bit-addressed.
    bool supported = false;             ///< False when the PLC profile does not support this device.
    uint32_t lower_bound = 0U;          ///< Always zero in the current profile rules.
    uint32_t upper_bound = 0U;          ///< Inclusive last address when @ref has_upper_bound is true.
    bool has_upper_bound = false;       ///< True when a finite last address is known.
    uint32_t point_count = 0U;          ///< Configured point count when @ref has_point_count is true.
    bool has_point_count = false;       ///< True when the PLC/configuration exposes a usable count.
    std::string address_range;          ///< Preformatted range such as `X0000-X1777` or empty when unavailable.
    DeviceRangeNotation notation = DeviceRangeNotation::Base10; ///< Rendered numbering style.
    std::string source;                 ///< Rule source such as `SD300` or `Fixed family limit`.
    std::string notes;                  ///< Optional profile-specific note.
};

/**
 * @struct DeviceRangeCatalog
 * @brief Resolved device-range catalog for one explicit PLC profile selection.
 */
struct DeviceRangeCatalog {
    std::string model;                  ///< Synthetic profile label such as `IQ-F`.
    uint16_t model_code = 0U;           ///< Always zero for explicit profile reads.
    bool has_model_code = false;        ///< False because no type-name query is used here.
    PlcProfile plc_profile = PlcProfile::IqR; ///< Caller-selected PLC profile.
    std::vector<DeviceRangeEntry> entries; ///< Device rows in stable output order.
};

/**
 * @brief Return the stable display label for one explicit device-range PLC profile.
 * @param profile Selected PLC profile.
 * @return Profile label text such as `IQ-F`.
 */
const char* deviceRangeProfileLabel(PlcProfile profile);

/**
 * @brief Read the configured device-range catalog for one explicit PLC profile.
 *
 * This helper reads the profile-specific `SD` block and formats entries such as
 * `points=1024` and `range=X0000-X1777`.
 *
 * `QCPU`, `LCPU`, `QnU`, and `QnUDV` also run direct read probes for runtime
 * `Z`/`ZR` behavior after the `SD` block is loaded.
 *
 * The caller chooses the PLC profile explicitly. This helper does not call
 * `ReadTypeName`.
 *
 * @param client Connected low-level client instance.
 * @param profile Explicit PLC profile to use for the `SD` block mapping.
 * @param out Receives the resolved device-range catalog.
 * @return @ref Error::Ok on success.
 */
Error readDeviceRangeCatalogForPlcProfile(SlmpClient& client, PlcProfile profile, DeviceRangeCatalog& out);

/**
 * @brief Parse a high-level address string.
 *
 * Accepted forms:
 * - `D100:U`
 * - `D100:S`
 * - `D200:D`
 * - `D200:L`
 * - `D200:F`
 * - `D50.3`
 * - `M1000:BIT`
 *
 * Dtype suffixes are interpreted as:
 * - `:BIT` direct bit device
 * - `:U` unsigned 16-bit
 * - `:S` signed 16-bit
 * - `:D` unsigned 32-bit
 * - `:L` signed 32-bit
 * - `:F` float32
 *
 * Addresses without an explicit suffix are rejected. Use `.n` notation for a
 * bit inside a word device.
 *
 * @param address Address string to parse.
 * @param family Explicit PLC profile used for string-device interpretation.
 * @param out Parsed result.
 * @return @ref Error::Ok on success.
 */
Error parseAddressSpec(const char* address, PlcProfile family, AddressSpec& out);

/**
 * @brief Format one parsed high-level address into canonical uppercase text.
 *
 * The formatted result uses MELSEC numbering rules for the target device
 * family and preserves explicit dtype suffixes when the address needs them to
 * round-trip one logical value.
 *
 * Examples:
 * - `D100:U`
 * - `D200:F`
 * - `D50.A`
 * - `X1A:BIT`
 *
 * @param spec Parsed address specification to format.
 * @param family Explicit PLC profile used for string-device formatting.
 * @param out Caller-provided destination buffer.
 * @param out_size Destination buffer size in bytes.
 * @return @ref Error::Ok on success.
 */
Error formatAddressSpec(const AddressSpec& spec, PlcProfile family, char* out, size_t out_size);

/**
 * @brief Parse and immediately format one user-facing address into canonical text.
 *
 * This helper is useful when firmware accepts free-form user input and wants
 * one normalized spelling for logging, caching, or configuration storage.
 *
 * Example:
 * @code
 * char normalized[32] = {};
 * slmp::highlevel::normalizeAddress(" d200:f ", slmp::PlcProfile::IqR, normalized, sizeof(normalized));
 * // normalized -> "D200:F"
 * @endcode
 *
 * @param address User-facing address string.
 * @param family Explicit PLC profile used for string-device interpretation.
 * @param out Caller-provided destination buffer.
 * @param out_size Destination buffer size in bytes.
 * @return @ref Error::Ok on success.
 */
Error normalizeAddress(const char* address, PlcProfile family, char* out, size_t out_size);

/**
 * @brief Read one logical value by device string and explicit dtype.
 *
 * Supported dtypes: `BIT`, `U`, `S`, `D`, `L`, `F`.
 *
 * `D`, `L`, and `F` consume two consecutive PLC words in little-endian order.
 * `BIT` is intended for direct bit devices such as `M1000`; to access a bit
 * inside a word device use the address-form overload with `.bit` notation.
 *
 * Example:
 * @code
 * plc.setPlcProfile(slmp::PlcProfile::IqR);
 * slmp::highlevel::Value value;
 * slmp::highlevel::readTyped(plc, "D100", "U", value);
 * @endcode
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 *
 * @param client Connected low-level client instance.
 * @param device Base device string such as `D100`.
 * @param dtype Logical type name such as `U` or `F`.
 * @param out Receives the decoded logical value.
 * @return @ref Error::Ok on success.
 */
Error readTyped(SlmpClient& client, const char* device, const char* dtype, Value& out);

/**
 * @brief Read one logical value using one address string such as `D100:U` or `D200:F`.
 *
 * This overload is the most convenient form for application code because the
 * type is declared in the address string.
 *
 * Examples:
 * - `Z100:U`
 * - `RD100:D`
 * - `LTS10:BIT`
 * - `D50.3`
 *
 * @param client Connected low-level client instance.
 * @param address High-level address string.
 * @param out Receives the decoded logical value.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error readTyped(SlmpClient& client, const char* address, Value& out);

/**
 * @brief Write one logical value by device string and explicit dtype.
 *
 * Supported dtypes: `BIT`, `U`, `S`, `D`, `L`, `F`.
 *
 * For 32-bit values, the helper writes two consecutive PLC words in
 * little-endian order. This helper does not implicitly mask or range-limit the
 * payload beyond the target dtype conversion.
 *
 * @param client Connected low-level client instance.
 * @param device Base device string such as `D100`.
 * @param dtype Logical type name such as `U` or `F`.
 * @param value Logical value to encode and write.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error writeTyped(SlmpClient& client, const char* device, const char* dtype, const Value& value);

/**
 * @brief Write one logical value using one address string such as `D100:U`, `D200:F`, or `D50.3`.
 *
 * This overload supports both direct bit devices and word-bit expressions.
 * When the address uses `.bit`, the helper performs a read-modify-write of the
 * underlying word device.
 *
 * @param client Connected low-level client instance.
 * @param address High-level address string.
 * @param value Logical value to encode and write.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error writeTyped(SlmpClient& client, const char* address, const Value& value);

/**
 * @brief Update one bit inside a word device using read-modify-write.
 *
 * Example: `D50.3`.
 *
 * The helper first reads the source word, updates one bit in memory, and then
 * writes the modified word back to the PLC.
 *
 * @param client Connected low-level client instance.
 * @param device Base word device string such as `D50`.
 * @param bit_index Bit index within the word, `0..15`.
 * @param value Bit state to write.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error writeBitInWord(SlmpClient& client, const char* device, int bit_index, bool value);

/**
 * @brief Compile a reusable mixed read plan.
 *
 * This separates address parsing from repeated polling. The resulting plan can
 * be reused by @ref readNamed or @ref Poller.
 *
 * Batchable word and dword devices are deduplicated automatically, so
 * requesting `D100:U`, `D100:S`, and `D100.3` results in one word-device entry
 * in the compiled plan.
 *
 * @param addresses Caller-provided high-level addresses.
 * @param family Explicit PLC profile used to parse every address.
 * @param out Receives the compiled plan.
 * @return @ref Error::Ok on success.
 *
 */
Error compileReadPlan(const std::vector<std::string>& addresses, PlcProfile family, ReadPlan& out);

/**
 * @brief Read a mixed logical snapshot by address strings.
 *
 * Example:
 * @code
 * slmp::highlevel::Snapshot snapshot;
 * slmp::highlevel::readNamed(plc, {"SM400:BIT", "D100:U", "D200:F", "D50.3"}, snapshot);
 * @endcode
 *
 * The helper emits exactly one random-read request. Expressions that cannot be
 * represented by that request, and plans that exceed one-request limits, are
 * rejected before transport rather than being read in multiple requests.
 *
 * @param client Connected low-level client instance.
 * @param addresses Caller-provided address list.
 * @param out Receives the logical values in caller order.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error readNamed(SlmpClient& client, const std::vector<std::string>& addresses, Snapshot& out);

/**
 * @brief Read a mixed logical snapshot using a previously compiled plan.
 *
 * This overload is the preferred fast path for repeated sampling because the
 * parsing and batching decision has already been done by @ref compileReadPlan.
 *
 * @param client Connected low-level client instance.
 * @param plan Previously compiled read plan.
 * @param out Receives the logical values in plan order.
 * @return @ref Error::Ok on success.
 */
Error readNamed(SlmpClient& client, const ReadPlan& plan, Snapshot& out);

/**
 * @brief Write a mixed logical snapshot.
 *
 * The complete collection is encoded as one random-write request. Word and
 * DWord values may share that request; bit values use the random-bit command.
 * Mixed command families and bit-in-word read-modify-write are rejected before
 * transport.
 *
 * @param client Connected low-level client instance.
 * @param updates Ordered address/value pairs to write.
 * @return @ref Error::Ok on success.
 *
 * The operation derives its PLC profile from @p client and returns
 * @ref Error::InvalidArgument when the client profile is unspecified.
 */
Error writeNamed(SlmpClient& client, const Snapshot& updates);

/**
 * @class Poller
 * @brief Reusable snapshot poller that keeps one compiled read plan.
 *
 * This class does not sleep. The caller controls timing and repeatedly calls
 * @ref readOnce from a scheduler, main loop, or RTOS task.
 */
class Poller {
  public:
    Poller() = default;
    /** @brief Construct a poller from an already-compiled plan. */
    explicit Poller(const ReadPlan& plan) : plan_(plan) {}

    /** @brief Compile and store one reusable read plan with an explicit PLC profile. */
    Error compile(const std::vector<std::string>& addresses, PlcProfile family);
    /**
     * @brief Execute one snapshot read with the stored compiled plan.
     * @param client Connected low-level client instance.
     * @param out Receives the logical values in plan order.
     * @return @ref Error::Ok on success.
     */
    Error readOnce(SlmpClient& client, Snapshot& out) const;
    /** @brief Return the currently stored compiled plan for inspection or reuse. */
    const ReadPlan& plan() const { return plan_; }

  private:
    ReadPlan plan_;
};

/** @} */ // end of SLMP_HighLevel

}  // namespace highlevel
}  // namespace slmp

#endif

#endif
