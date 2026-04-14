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
 * - `D100`
 * - `D100:S`
 * - `D200:F`
 * - `D50.3`
 * - `M1000`
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
    DeviceAddress device{};      ///< Base device address used by the low-level API.
    ValueType type = ValueType::U16; ///< Logical value type requested by the caller.
    int bit_index = -1;          ///< Bit index for `D50.3` style access, otherwise -1.
    bool explicit_type = false;  ///< True when the input used an explicit suffix such as `:F`.
};

/**
 * @enum BatchKind
 * @brief Read-plan batching strategy for one named address.
 */
enum class BatchKind : uint8_t {
    None,       ///< Read individually through the fallback path.
    Word,       ///< Read through batched random word access.
    Dword,      ///< Read through batched random dword access.
    BitInWord,  ///< Read one bit from a batched word value.
    LongTimer,  ///< Read one long-family point from a helper-backed 4-word block.
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
 */
struct ReadPlan {
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
 * structure convenient both for result presentation and for deterministic
 * replay of write sequences.
 */
using Snapshot = std::vector<NamedValue>;

/**
 * @enum DeviceRangeFamily
 * @brief Explicit PLC family used for device-range SD block reads.
 *
 * The caller selects the family. This helper does not call `ReadTypeName`.
 */
enum class DeviceRangeFamily : uint8_t {
    IqR,
    MxF,
    MxR,
    IqF,
    QCpu,
    LCpu,
    QnU,
    QnUDV,
};

/**
 * @enum DeviceRangeCategory
 * @brief Logical grouping for one device-range entry.
 */
enum class DeviceRangeCategory : uint8_t {
    Bit,
    Word,
    TimerCounter,
    Index,
    FileRefresh,
};

/**
 * @enum DeviceRangeNotation
 * @brief Rendered public address numbering style for one device family.
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
    bool supported = false;             ///< False when the family does not support this device.
    uint32_t lower_bound = 0U;          ///< Always zero in the current family rules.
    uint32_t upper_bound = 0U;          ///< Inclusive last address when @ref has_upper_bound is true.
    bool has_upper_bound = false;       ///< True when a finite last address is known.
    uint32_t point_count = 0U;          ///< Configured point count when @ref has_point_count is true.
    bool has_point_count = false;       ///< True when the PLC/configuration exposes a usable count.
    std::string address_range;          ///< Preformatted range such as `X0000-X1777` or empty when unavailable.
    DeviceRangeNotation notation = DeviceRangeNotation::Base10; ///< Rendered numbering style.
    std::string source;                 ///< Rule source such as `SD300` or `Fixed family limit`.
    std::string notes;                  ///< Optional family-specific note.
};

/**
 * @struct DeviceRangeCatalog
 * @brief Resolved device-range catalog for one explicit family selection.
 */
struct DeviceRangeCatalog {
    std::string model;                  ///< Synthetic family label such as `IQ-F`.
    uint16_t model_code = 0U;           ///< Always zero for explicit-family reads.
    bool has_model_code = false;        ///< False because no type-name query is used here.
    DeviceRangeFamily family = DeviceRangeFamily::IqR; ///< Caller-selected PLC family.
    std::vector<DeviceRangeEntry> entries; ///< Device rows in stable output order.
};

/**
 * @brief Return the stable label for one explicit device-range family.
 * @param family Selected device-range family.
 * @return Family label text such as `IQ-F`.
 */
const char* deviceRangeFamilyLabel(DeviceRangeFamily family);

/**
 * @brief Read the configured device-range catalog for one explicit PLC family.
 *
 * This helper reads the family-specific `SD` block with exactly one PLC request
 * and formats entries such as `points=1024` and `range=X0000-X1777`.
 *
 * The caller chooses the PLC family explicitly. This helper does not call
 * `ReadTypeName`.
 *
 * @param client Connected low-level client instance.
 * @param family Explicit PLC family to use for the `SD` block mapping.
 * @param out Receives the resolved device-range catalog.
 * @return @ref Error::Ok on success.
 */
Error readDeviceRangeCatalogForFamily(SlmpClient& client, DeviceRangeFamily family, DeviceRangeCatalog& out);

/**
 * @brief Parse a high-level address string.
 *
 * Accepted forms:
 * - `D100`
 * - `D100:S`
 * - `D200:D`
 * - `D200:L`
 * - `D200:F`
 * - `D50.3`
 * - `M1000`
 *
 * Direct bit devices such as `M1000` default to @ref ValueType::Bit.
 * Word devices default to @ref ValueType::U16, except plain `LTN`, `LSTN`,
 * and `LCN`, which default to @ref ValueType::U32 current-value access.
 * Dtype suffixes are interpreted as:
 * - `:S` signed 16-bit
 * - `:D` unsigned 32-bit
 * - `:L` signed 32-bit
 * - `:F` float32
 *
 * For addresses without an explicit suffix, @ref AddressSpec::explicit_type is `false`,
 * which allows higher layers to preserve the user's original intent.
 *
 * @param address Address string to parse.
 * @param out Parsed result.
 * @return @ref Error::Ok on success, otherwise an error describing why the
 * address cannot be interpreted by the helper layer.
 */
Error parseAddressSpec(const char* address, AddressSpec& out);

/**
 * @brief Format one parsed high-level address into canonical uppercase text.
 *
 * The formatted result uses Mitsubishi numbering rules for the target device
 * family and preserves explicit dtype suffixes when the address needs them to
 * round-trip one logical value.
 *
 * Examples:
 * - `D100`
 * - `D200:F`
 * - `D50.A`
 * - `X1A`
 *
 * @param spec Parsed address specification to format.
 * @param out Caller-provided destination buffer.
 * @param out_size Destination buffer size in bytes.
 * @note This helper does not communicate with the PLC. It only formats the
 * parsed logical address into canonical text.
 * @return @ref Error::Ok on success.
 */
Error formatAddressSpec(const AddressSpec& spec, char* out, size_t out_size);

/**
 * @brief Parse and immediately format one user-facing address into canonical text.
 *
 * This helper is useful when firmware accepts free-form user input and wants
 * one normalized spelling for logging, caching, or configuration storage.
 *
 * Example:
 * @code
 * char normalized[32] = {};
 * slmp::highlevel::normalizeAddress(" d200:f ", normalized, sizeof(normalized));
 * // normalized -> "D200:F"
 * @endcode
 *
 * @param address User-facing address string.
 * @param out Caller-provided destination buffer.
 * @param out_size Destination buffer size in bytes.
 * @note This helper performs parse + format only. No PLC request is issued.
 * @return @ref Error::Ok on success.
 */
Error normalizeAddress(const char* address, char* out, size_t out_size);

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
 * slmp::highlevel::Value value;
 * slmp::highlevel::readTyped(plc, "D100", "U", value);
 * @endcode
 *
 * @param client Connected low-level client instance.
 * @param device Base device string such as `D100`.
 * @param dtype Logical type name such as `U` or `F`.
 * @param out Receives the decoded logical value.
 * @return @ref Error::Ok on success.
 */
Error readTyped(SlmpClient& client, const char* device, const char* dtype, Value& out);

/**
 * @brief Read one logical value using one address string such as `D100` or `D200:F`.
 *
 * This overload is the most convenient form for application code because the
 * type can be inferred from the address string.
 *
 * Examples:
 * - `Z100`
 * - `RD100:D`
 * - `LTS10`
 * - `D50.3`
 *
 * @param client Connected low-level client instance.
 * @param address High-level address string.
 * @param out Receives the decoded logical value.
 * @return @ref Error::Ok on success.
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
 */
Error writeTyped(SlmpClient& client, const char* device, const char* dtype, const Value& value);

/**
 * @brief Write one logical value using one address string such as `D100`, `D200:F`, or `D50.3`.
 *
 * This overload supports both direct bit devices and word-bit expressions.
 * When the address uses `.bit`, the helper performs a read-modify-write of the
 * underlying word device.
 *
 * @param client Connected low-level client instance.
 * @param address High-level address string.
 * @param value Logical value to encode and write.
 * @return @ref Error::Ok on success.
 */
Error writeTyped(SlmpClient& client, const char* address, const Value& value);

/**
 * @brief Read a large contiguous word range and optionally split it into multiple requests.
 *
 * Use this when the logical range is larger than one practical request and you
 * still want one helper call in user code.
 *
 * When @p allow_split is `false`, oversize requests fail instead of silently
 * issuing multiple protocol frames.
 *
 * @note The helper never splits one 32-bit logical value across requests.
 *
 * @param client Connected low-level client instance.
 * @param start Start address such as `D1000`.
 * @param count Number of words to read.
 * @param out Receives the decoded words in PLC order.
 * @param max_per_request Maximum words per low-level request.
 * @param allow_split When true, the helper may issue multiple requests.
 * @return @ref Error::Ok on success.
 */
Error readWordsChunked(
    SlmpClient& client,
    const char* start,
    uint16_t count,
    std::vector<uint16_t>& out,
    uint16_t max_per_request = 960U,
    bool allow_split = false
);

/**
 * @brief Read a large contiguous dword range and optionally split it into multiple requests.
 *
 * Chunk boundaries are aligned to full dwords so each returned element still
 * maps cleanly to one logical 32-bit value.
 *
 * When @p allow_split is `false`, oversize requests fail instead of silently
 * issuing multiple protocol frames.
 *
 * @param client Connected low-level client instance.
 * @param start Start address such as `D2000`.
 * @param count Number of dwords to read.
 * @param out Receives the decoded dwords in PLC order.
 * @param max_dwords_per_request Maximum dwords per low-level request.
 * @param allow_split When true, the helper may issue multiple requests.
 * @return @ref Error::Ok on success.
 */
Error readDWordsChunked(
    SlmpClient& client,
    const char* start,
    uint16_t count,
    std::vector<uint32_t>& out,
    uint16_t max_dwords_per_request = 480U,
    bool allow_split = false
);

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
 */
Error writeBitInWord(SlmpClient& client, const char* device, int bit_index, bool value);

/**
 * @brief Compile a reusable mixed read plan.
 *
 * This separates address parsing from repeated polling. The resulting plan can
 * be reused by @ref readNamed or @ref Poller.
 *
 * Batchable word and dword devices are deduplicated automatically, so
 * requesting `D100`, `D100:S`, and `D100.3` results in one word-device entry
 * in the compiled plan.
 *
 * @param addresses Caller-provided high-level addresses.
 * @param out Receives the compiled plan.
 * @return @ref Error::Ok on success.
 */
Error compileReadPlan(const std::vector<std::string>& addresses, ReadPlan& out);

/**
 * @brief Read a mixed logical snapshot by address strings.
 *
 * Example:
 * @code
 * slmp::highlevel::Snapshot snapshot;
 * slmp::highlevel::readNamed(plc, {"SM400", "D100", "D200:F", "D50.3"}, snapshot);
 * @endcode
 *
 * Where possible, the helper batches word and dword devices through the random
 * read API and only falls back to individual reads for expressions that cannot
 * share one batch safely.
 *
 * @param client Connected low-level client instance.
 * @param addresses Caller-provided address list.
 * @param out Receives the logical values in caller order.
 * @return @ref Error::Ok on success.
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
 * Each entry is written independently in snapshot order.
 *
 * @note The operation is not transactional. If one entry fails, earlier writes
 * may already have been applied on the PLC.
 *
 * @param client Connected low-level client instance.
 * @param updates Ordered address/value pairs to write.
 * @return @ref Error::Ok on success.
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

    /**
     * @brief Compile and store one reusable read plan.
     * @param addresses Caller-provided high-level addresses.
     * @return @ref Error::Ok on success.
     */
    Error compile(const std::vector<std::string>& addresses);
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
