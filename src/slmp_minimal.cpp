/**
 * @file slmp_minimal.cpp
 * @brief Lightweight SLMP client implementation.
 */

#include "slmp_minimal.h"

#include <new>
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
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
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

static bool isAligned(const void* pointer, size_t alignment) {
    return (reinterpret_cast<uintptr_t>(pointer) % alignment) == 0U;
}

static void initializeLongTimerResults(LongTimerResult* results, int points) {
    for (int i = 0; i < points; ++i) {
        ::new (static_cast<void*>(results + i)) LongTimerResult{};
    }
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

constexpr size_t kDirectWordPointLimit = 960U;
constexpr size_t kDirectBitPointLimit = 7168U;
constexpr size_t kDirectIqFBitPointLimit = 3584U;
constexpr size_t kMemoryWordLimit = 480U;
constexpr size_t kExtendUnitByteLimit = 1920U;

constexpr size_t kRequestHeaderSize4E = 19;
constexpr size_t kResponsePrefixSize4E = 13;

constexpr size_t kRequestHeaderSize3E = 15;
constexpr size_t kResponsePrefixSize3E = 9;

inline bool isSpecifiedPlcProfile(PlcProfile profile) {
    switch (profile) {
        case PlcProfile::IqF:
        case PlcProfile::IqR:
        case PlcProfile::IqRRj71En71:
        case PlcProfile::IqL:
        case PlcProfile::MxF:
        case PlcProfile::MxR:
        case PlcProfile::QCpu:
        case PlcProfile::QCpuQj71E71100:
        case PlcProfile::LCpu:
        case PlcProfile::LCpuLj71E71100:
        case PlcProfile::QnU:
        case PlcProfile::QnUQj71E71100:
        case PlcProfile::QnUDV:
        case PlcProfile::QnUDVQj71E71100:
            return true;
        case PlcProfile::Unspecified:
            return false;
    }
    return false;
}

inline bool isBasePlcProfile(PlcProfile profile) {
    return profile == PlcProfile::QCpu;
}

inline bool isConnectionSelectablePlcProfile(PlcProfile profile) {
    return isSpecifiedPlcProfile(profile) && !isBasePlcProfile(profile);
}

inline bool isValidFrameType(FrameType frame_type) {
    return frame_type == FrameType::Frame3E || frame_type == FrameType::Frame4E;
}

inline bool isValidCompatibilityMode(CompatibilityMode mode) {
    return mode == CompatibilityMode::iQR || mode == CompatibilityMode::Legacy;
}

inline bool profileDisablesBlockAccess(PlcProfile profile) {
    (void)profile;
    return false;
}

enum class ProfileLimitKey : uint8_t {
    DirectWordRead,
    DirectWordWrite,
    DirectBitRead,
    DirectBitWrite,
    RandomReadWord,
    RandomReadWordExt,
    RandomWriteWord,
    RandomWriteWordExt,
    RandomWriteBit,
    RandomWriteBitExt,
    MonitorRegisterWord,
    MonitorRegisterWordExt,
};

struct FeatureEntry {
    ProfileFeatureKey key;
    const char* state;
    const char* evidence;
};

struct LimitEntry {
    ProfileLimitKey key;
    size_t max;
    size_t weighted_max;
    bool has_weighted_max;
};

struct WritePolicyEntry {
    DeviceCode code;
};

struct CapabilityProfile {
    PlcProfile profile;
    const FeatureEntry* features;
    size_t feature_count;
    const LimitEntry* limits;
    size_t limit_count;
    const WritePolicyEntry* write_policy;
    size_t write_policy_count;
};

#define SLMP_FEATURE_SUPPORTED(key) {ProfileFeatureKey::key, "supported", nullptr}
#define SLMP_FEATURE_CONFIG(key) {ProfileFeatureKey::key, "config-dependent", nullptr}
#define SLMP_FEATURE_DELEGATED(key) {ProfileFeatureKey::key, "delegated", nullptr}
#define SLMP_FEATURE_BLOCKED(key, evidence) {ProfileFeatureKey::key, "blocked", evidence}
#define SLMP_FEATURE_UNVERIFIED(key, evidence) {ProfileFeatureKey::key, "unverified", evidence}

static const FeatureEntry kIqRFeatures[] = {
    SLMP_FEATURE_SUPPORTED(TypeName),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_SUPPORTED(Block),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_CONFIG(ExtModuleAccess),
    SLMP_FEATURE_CONFIG(ExtLinkDirect),
    SLMP_FEATURE_SUPPORTED(HgCpuBuffer),
    SLMP_FEATURE_SUPPORTED(LongDevicePath),
    SLMP_FEATURE_SUPPORTED(Lz32BitPath),
};

static const FeatureEntry kIqLFeatures[] = {
    SLMP_FEATURE_SUPPORTED(TypeName),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_SUPPORTED(Block),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_CONFIG(ExtModuleAccess),
    SLMP_FEATURE_CONFIG(ExtLinkDirect),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "iQ-R only route; not defined for iQ-L"),
    SLMP_FEATURE_SUPPORTED(LongDevicePath),
    SLMP_FEATURE_SUPPORTED(Lz32BitPath),
};

static const FeatureEntry kMxFeatures[] = {
    SLMP_FEATURE_SUPPORTED(TypeName),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_SUPPORTED(Block),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_CONFIG(ExtModuleAccess),
    SLMP_FEATURE_CONFIG(ExtLinkDirect),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "Unavailable by design because multi-CPU configuration is not available"),
    SLMP_FEATURE_SUPPORTED(LongDevicePath),
    SLMP_FEATURE_SUPPORTED(Lz32BitPath),
};

static const FeatureEntry kIqFFeatures[] = {
    SLMP_FEATURE_SUPPORTED(TypeName),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_SUPPORTED(Block),
    SLMP_FEATURE_BLOCKED(Monitor, "C059 even for D10 single registration"),
    SLMP_FEATURE_CONFIG(ExtModuleAccess),
    SLMP_FEATURE_BLOCKED(ExtLinkDirect, "J1 link-direct access returned a PLC error"),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "iQ-R only route; not defined for iQ-F"),
    SLMP_FEATURE_SUPPORTED(LongDevicePath),
    SLMP_FEATURE_SUPPORTED(Lz32BitPath),
};

static const FeatureEntry kLCpuFeatures[] = {
    SLMP_FEATURE_BLOCKED(TypeName, "0101/0000 returned C059"),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_BLOCKED(Block, "Raw send returned C059"),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_BLOCKED(ExtModuleAccess, "U0\\G10 / U2\\G1000 returned C070"),
    SLMP_FEATURE_BLOCKED(ExtLinkDirect, "Link-direct access is not available on the tested built-in CPU port"),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "iQ-R only route; not defined for LCPU"),
    SLMP_FEATURE_DELEGATED(LongDevicePath),
    SLMP_FEATURE_DELEGATED(Lz32BitPath),
};

static const FeatureEntry kQnUDVFeatures[] = {
    SLMP_FEATURE_BLOCKED(TypeName, "0101/0000 returned C059"),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_BLOCKED(Block, "Raw send returned C059; high-level API guards before transport"),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_BLOCKED(ExtModuleAccess, "U0\\G10 / U2\\G1000 returned C070"),
    SLMP_FEATURE_BLOCKED(ExtLinkDirect, "Link-direct access is not available on the tested built-in CPU port"),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "iQ-R only route; not defined for QnUDV"),
    SLMP_FEATURE_DELEGATED(LongDevicePath),
    SLMP_FEATURE_DELEGATED(Lz32BitPath),
};

static const FeatureEntry kQLUnitFeatures[] = {
    SLMP_FEATURE_SUPPORTED(TypeName),
    SLMP_FEATURE_SUPPORTED(Direct),
    SLMP_FEATURE_SUPPORTED(Random),
    SLMP_FEATURE_SUPPORTED(Block),
    SLMP_FEATURE_SUPPORTED(Monitor),
    SLMP_FEATURE_CONFIG(ExtModuleAccess),
    SLMP_FEATURE_CONFIG(ExtLinkDirect),
    SLMP_FEATURE_BLOCKED(HgCpuBuffer, "CPU-buffer HG is an iQ-R-only path"),
    SLMP_FEATURE_BLOCKED(LongDevicePath, "Blocked for Q/L Ethernet unit profiles"),
    SLMP_FEATURE_BLOCKED(Lz32BitPath, "Blocked for Q/L Ethernet unit profiles"),
};

#undef SLMP_FEATURE_SUPPORTED
#undef SLMP_FEATURE_CONFIG
#undef SLMP_FEATURE_DELEGATED
#undef SLMP_FEATURE_BLOCKED
#undef SLMP_FEATURE_UNVERIFIED

#define SLMP_LIMIT(key, max_value) {ProfileLimitKey::key, max_value, 0U, false}
#define SLMP_LIMIT_WEIGHTED(key, max_value, weighted) {ProfileLimitKey::key, max_value, weighted, true}

static const LimitEntry kIqRLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 96U),
    SLMP_LIMIT(RandomReadWordExt, 96U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 80U, 960U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 80U, 960U),
    SLMP_LIMIT(RandomWriteBit, 94U),
    SLMP_LIMIT(RandomWriteBitExt, 94U),
    SLMP_LIMIT(MonitorRegisterWord, 96U),
    SLMP_LIMIT(MonitorRegisterWordExt, 96U),
};

static const LimitEntry kIqLLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 96U),
    SLMP_LIMIT(RandomReadWordExt, 96U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 80U, 960U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 80U, 960U),
    SLMP_LIMIT(RandomWriteBit, 94U),
    SLMP_LIMIT(RandomWriteBitExt, 94U),
    SLMP_LIMIT(MonitorRegisterWord, 96U),
    SLMP_LIMIT(MonitorRegisterWordExt, 96U),
};

static const LimitEntry kIqFLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 3584U),
    SLMP_LIMIT(DirectBitWrite, 3584U),
    SLMP_LIMIT(RandomReadWord, 192U),
    SLMP_LIMIT(RandomReadWordExt, 96U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 160U, 1920U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 80U, 960U),
    SLMP_LIMIT(RandomWriteBit, 188U),
    SLMP_LIMIT(RandomWriteBitExt, 94U),
    SLMP_LIMIT(MonitorRegisterWord, 192U),
    SLMP_LIMIT(MonitorRegisterWordExt, 96U),
};

static const LimitEntry kQLMeasuredLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBit, 188U),
    SLMP_LIMIT(MonitorRegisterWord, 192U),
    SLMP_LIMIT(RandomReadWordExt, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBitExt, 188U),
    SLMP_LIMIT(MonitorRegisterWordExt, 192U),
};

static const LimitEntry kQLUnitQCpuLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBit, 188U),
    SLMP_LIMIT(MonitorRegisterWord, 192U),
    SLMP_LIMIT(RandomReadWordExt, 185U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBitExt, 188U),
    SLMP_LIMIT(MonitorRegisterWordExt, 192U),
};

static const LimitEntry kQLUnitLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBit, 188U),
    SLMP_LIMIT(MonitorRegisterWord, 192U),
    SLMP_LIMIT(RandomReadWordExt, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBitExt, 188U),
    SLMP_LIMIT(MonitorRegisterWordExt, 192U),
};

static const LimitEntry kQLUnitLCpuLimits[] = {
    SLMP_LIMIT(DirectWordRead, 960U),
    SLMP_LIMIT(DirectWordWrite, 960U),
    SLMP_LIMIT(DirectBitRead, 7168U),
    SLMP_LIMIT(DirectBitWrite, 7168U),
    SLMP_LIMIT(RandomReadWord, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWord, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBit, 188U),
    SLMP_LIMIT(MonitorRegisterWord, 192U),
    SLMP_LIMIT(RandomReadWordExt, 192U),
    SLMP_LIMIT_WEIGHTED(RandomWriteWordExt, 160U, 1920U),
    SLMP_LIMIT(RandomWriteBitExt, 188U),
    SLMP_LIMIT(MonitorRegisterWordExt, 192U),
};

#undef SLMP_LIMIT
#undef SLMP_LIMIT_WEIGHTED

static const WritePolicyEntry kSWritePolicy[] = {
    {DeviceCode::S},
};

static const CapabilityProfile kCapabilityProfiles[] = {
    {PlcProfile::IqR, kIqRFeatures, sizeof(kIqRFeatures) / sizeof(kIqRFeatures[0]), kIqRLimits, sizeof(kIqRLimits) / sizeof(kIqRLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::IqRRj71En71, kIqRFeatures, sizeof(kIqRFeatures) / sizeof(kIqRFeatures[0]), kIqRLimits, sizeof(kIqRLimits) / sizeof(kIqRLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::IqL, kIqLFeatures, sizeof(kIqLFeatures) / sizeof(kIqLFeatures[0]), kIqLLimits, sizeof(kIqLLimits) / sizeof(kIqLLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::MxR, kMxFeatures, sizeof(kMxFeatures) / sizeof(kMxFeatures[0]), kIqRLimits, sizeof(kIqRLimits) / sizeof(kIqRLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::MxF, kMxFeatures, sizeof(kMxFeatures) / sizeof(kMxFeatures[0]), kIqRLimits, sizeof(kIqRLimits) / sizeof(kIqRLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::IqF, kIqFFeatures, sizeof(kIqFFeatures) / sizeof(kIqFFeatures[0]), kIqFLimits, sizeof(kIqFLimits) / sizeof(kIqFLimits[0]), nullptr, 0U},
    {PlcProfile::QCpu, kQnUDVFeatures, sizeof(kQnUDVFeatures) / sizeof(kQnUDVFeatures[0]), kQLUnitQCpuLimits, sizeof(kQLUnitQCpuLimits) / sizeof(kQLUnitQCpuLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::QCpuQj71E71100, kQLUnitFeatures, sizeof(kQLUnitFeatures) / sizeof(kQLUnitFeatures[0]), kQLUnitQCpuLimits, sizeof(kQLUnitQCpuLimits) / sizeof(kQLUnitQCpuLimits[0]), nullptr, 0U},
    {PlcProfile::LCpu, kLCpuFeatures, sizeof(kLCpuFeatures) / sizeof(kLCpuFeatures[0]), kQLMeasuredLimits, sizeof(kQLMeasuredLimits) / sizeof(kQLMeasuredLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::LCpuLj71E71100, kQLUnitFeatures, sizeof(kQLUnitFeatures) / sizeof(kQLUnitFeatures[0]), kQLUnitLCpuLimits, sizeof(kQLUnitLCpuLimits) / sizeof(kQLUnitLCpuLimits[0]), nullptr, 0U},
    {PlcProfile::QnU, kQnUDVFeatures, sizeof(kQnUDVFeatures) / sizeof(kQnUDVFeatures[0]), kQLMeasuredLimits, sizeof(kQLMeasuredLimits) / sizeof(kQLMeasuredLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::QnUQj71E71100, kQLUnitFeatures, sizeof(kQLUnitFeatures) / sizeof(kQLUnitFeatures[0]), kQLUnitLimits, sizeof(kQLUnitLimits) / sizeof(kQLUnitLimits[0]), nullptr, 0U},
    {PlcProfile::QnUDV, kQnUDVFeatures, sizeof(kQnUDVFeatures) / sizeof(kQnUDVFeatures[0]), kQLMeasuredLimits, sizeof(kQLMeasuredLimits) / sizeof(kQLMeasuredLimits[0]), kSWritePolicy, sizeof(kSWritePolicy) / sizeof(kSWritePolicy[0])},
    {PlcProfile::QnUDVQj71E71100, kQLUnitFeatures, sizeof(kQLUnitFeatures) / sizeof(kQLUnitFeatures[0]), kQLUnitLimits, sizeof(kQLUnitLimits) / sizeof(kQLUnitLimits[0]), nullptr, 0U},
};

static const char* profileLabel(PlcProfile profile) {
    switch (profile) {
        case PlcProfile::IqF: return "melsec:iq-f";
        case PlcProfile::IqR: return "melsec:iq-r";
        case PlcProfile::IqRRj71En71: return "melsec:iq-r:rj71en71";
        case PlcProfile::IqL: return "melsec:iq-l";
        case PlcProfile::MxF: return "melsec:mx-f";
        case PlcProfile::MxR: return "melsec:mx-r";
        case PlcProfile::QCpu: return "melsec:qcpu";
        case PlcProfile::QCpuQj71E71100: return "melsec:qcpu:qj71e71-100";
        case PlcProfile::LCpu: return "melsec:lcpu";
        case PlcProfile::LCpuLj71E71100: return "melsec:lcpu:lj71e71-100";
        case PlcProfile::QnU: return "melsec:qnu";
        case PlcProfile::QnUQj71E71100: return "melsec:qnu:qj71e71-100";
        case PlcProfile::QnUDV: return "melsec:qnudv";
        case PlcProfile::QnUDVQj71E71100: return "melsec:qnudv:qj71e71-100";
        case PlcProfile::Unspecified:
            return "";
    }
    return "";
}

static const char* featureKeyName(ProfileFeatureKey key) {
    switch (key) {
        case ProfileFeatureKey::TypeName: return "type_name";
        case ProfileFeatureKey::Direct: return "direct";
        case ProfileFeatureKey::Random: return "random";
        case ProfileFeatureKey::Block: return "block";
        case ProfileFeatureKey::Monitor: return "monitor";
        case ProfileFeatureKey::ExtModuleAccess: return "ext_module_access";
        case ProfileFeatureKey::ExtLinkDirect: return "ext_link_direct";
        case ProfileFeatureKey::HgCpuBuffer: return "hg_cpu_buffer";
        case ProfileFeatureKey::LongDevicePath: return "long_device_path";
        case ProfileFeatureKey::Lz32BitPath: return "lz_32bit_path";
        default: return "";
    }
}

static const CapabilityProfile* findCapabilityProfile(PlcProfile profile) {
    for (size_t i = 0; i < sizeof(kCapabilityProfiles) / sizeof(kCapabilityProfiles[0]); ++i) {
        if (kCapabilityProfiles[i].profile == profile) return &kCapabilityProfiles[i];
    }
    return nullptr;
}

static const FeatureEntry* findFeatureEntry(PlcProfile profile, ProfileFeatureKey key) {
    const CapabilityProfile* capability = findCapabilityProfile(profile);
    if (capability == nullptr) return nullptr;
    for (size_t i = 0; i < capability->feature_count; ++i) {
        if (capability->features[i].key == key) return &capability->features[i];
    }
    return nullptr;
}

static const LimitEntry* findLimitEntry(PlcProfile profile, ProfileLimitKey key) {
    const CapabilityProfile* capability = findCapabilityProfile(profile);
    if (capability == nullptr) return nullptr;
    for (size_t i = 0; i < capability->limit_count; ++i) {
        if (capability->limits[i].key == key) return &capability->limits[i];
    }
    return nullptr;
}

static size_t profileLimitOr(PlcProfile profile, ProfileLimitKey key, size_t fallback) {
    const LimitEntry* limit = findLimitEntry(profile, key);
    return limit == nullptr ? fallback : limit->max;
}

inline ProfileLimitKey extendedProfileLimitKey(ProfileLimitKey key) {
    switch (key) {
        case ProfileLimitKey::RandomReadWord:
            return ProfileLimitKey::RandomReadWordExt;
        case ProfileLimitKey::MonitorRegisterWord:
            return ProfileLimitKey::MonitorRegisterWordExt;
        default:
            return key;
    }
}

static bool isGuardedFeatureState(const char* state) {
    return strcmp(state, "blocked") == 0 || strcmp(state, "unverified") == 0;
}

static bool profileFeatureWouldBlock(PlcProfile profile, ProfileFeatureKey key) {
    const FeatureEntry* feature = findFeatureEntry(profile, key);
    return feature != nullptr && isGuardedFeatureState(feature->state);
}

static bool isProfileReadOnlyDevice(PlcProfile profile, DeviceCode code) {
    const CapabilityProfile* capability = findCapabilityProfile(profile);
    if (capability == nullptr || capability->write_policy == nullptr) return false;
    for (size_t i = 0; i < capability->write_policy_count; ++i) {
        if (capability->write_policy[i].code == code) return true;
    }
    return false;
}

inline FrameType frameTypeForPlcProfile(PlcProfile profile) {
    switch (profile) {
        case PlcProfile::IqR:
        case PlcProfile::IqRRj71En71:
        case PlcProfile::IqL:
        case PlcProfile::MxF:
        case PlcProfile::MxR:
        case PlcProfile::QCpuQj71E71100:
        case PlcProfile::LCpuLj71E71100:
        case PlcProfile::QnUQj71E71100:
        case PlcProfile::QnUDVQj71E71100:
            return FrameType::Frame4E;
        case PlcProfile::IqF:
        case PlcProfile::QCpu:
        case PlcProfile::LCpu:
        case PlcProfile::QnU:
        case PlcProfile::QnUDV:
        case PlcProfile::Unspecified:
            return FrameType::Frame3E;
    }
    return FrameType::Frame3E;
}

inline CompatibilityMode compatibilityModeForPlcProfile(PlcProfile profile) {
    switch (profile) {
        case PlcProfile::IqR:
        case PlcProfile::IqRRj71En71:
        case PlcProfile::IqL:
        case PlcProfile::MxF:
        case PlcProfile::MxR:
            return CompatibilityMode::iQR;
        case PlcProfile::IqF:
        case PlcProfile::QCpu:
        case PlcProfile::QCpuQj71E71100:
        case PlcProfile::LCpu:
        case PlcProfile::LCpuLj71E71100:
        case PlcProfile::QnU:
        case PlcProfile::QnUQj71E71100:
        case PlcProfile::QnUDV:
        case PlcProfile::QnUDVQj71E71100:
        case PlcProfile::Unspecified:
            return CompatibilityMode::Legacy;
    }
    return CompatibilityMode::Legacy;
}

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

inline size_t randomReadLikeLimit(CompatibilityMode mode, bool extended = false) {
    return (extended || mode == CompatibilityMode::iQR) ? 96U : 192U;
}

inline size_t randomBitWriteLimit(CompatibilityMode mode, bool extended = false) {
    return (extended || mode == CompatibilityMode::iQR) ? 94U : 188U;
}

inline size_t randomWriteWordWeightLimit(CompatibilityMode mode, bool extended = false) {
    return (extended || mode == CompatibilityMode::iQR) ? 960U : 1920U;
}

inline size_t blockCountLimit(CompatibilityMode mode) {
    return (mode == CompatibilityMode::iQR) ? 60U : 120U;
}

inline size_t blockWriteOverheadPerBlock(CompatibilityMode mode) {
    return (mode == CompatibilityMode::iQR) ? 9U : 4U;
}

inline Error validateDirectWordAccessPoints(size_t points, PlcProfile profile, bool write) {
    const size_t limit = profileLimitOr(
        profile,
        write ? ProfileLimitKey::DirectWordWrite : ProfileLimitKey::DirectWordRead,
        kDirectWordPointLimit
    );
    return (points >= 1U && points <= limit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateDirectDWordAccessPoints(size_t points, PlcProfile profile, bool write) {
    const size_t word_limit = profileLimitOr(
        profile,
        write ? ProfileLimitKey::DirectWordWrite : ProfileLimitKey::DirectWordRead,
        kDirectWordPointLimit
    );
    return (points >= 1U && points <= (word_limit / 2U)) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateDirectBitAccessPoints(size_t points, PlcProfile profile, bool write) {
    const size_t limit = profileLimitOr(
        profile,
        write ? ProfileLimitKey::DirectBitWrite : ProfileLimitKey::DirectBitRead,
        kDirectBitPointLimit
    );
    return (points >= 1U && points <= limit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateRandomReadLikeCounts(
    size_t word_count,
    size_t dword_count,
    CompatibilityMode mode,
    bool extended = false,
    PlcProfile profile = PlcProfile::Unspecified,
    ProfileLimitKey limit_key = ProfileLimitKey::RandomReadWord
) {
    if (word_count > 0xFFU || dword_count > 0xFFU) {
        return Error::InvalidArgument;
    }
    const size_t total = word_count + dword_count;
    const size_t fallback = randomReadLikeLimit(mode, extended);
    const size_t limit = profileLimitOr(profile, extended ? extendedProfileLimitKey(limit_key) : limit_key, fallback);
    return (total >= 1U && total <= limit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateRandomWriteWordCounts(
    size_t word_count,
    size_t dword_count,
    CompatibilityMode mode,
    bool extended = false,
    PlcProfile profile = PlcProfile::Unspecified
) {
    if (word_count > 0xFFU || dword_count > 0xFFU) {
        return Error::InvalidArgument;
    }
    const size_t total = word_count + dword_count;
    if (total == 0U) {
        return Error::InvalidArgument;
    }
    const ProfileLimitKey limit_key = extended ? ProfileLimitKey::RandomWriteWordExt : ProfileLimitKey::RandomWriteWord;
    const LimitEntry* limit = findLimitEntry(profile, limit_key);
    if (limit != nullptr) {
        if (total > limit->max) {
            return Error::InvalidArgument;
        }
        if (!limit->has_weighted_max) {
            return Error::Ok;
        }
        const size_t weighted = (word_count * 12U) + (dword_count * 14U);
        return (weighted <= limit->weighted_max) ? Error::Ok : Error::InvalidArgument;
    }
    const size_t weighted = (word_count * 12U) + (dword_count * 14U);
    return (weighted <= randomWriteWordWeightLimit(mode, extended)) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateRandomBitWriteCount(
    size_t bit_count,
    CompatibilityMode mode,
    bool extended = false,
    PlcProfile profile = PlcProfile::Unspecified
) {
    const size_t fallback = randomBitWriteLimit(mode, extended);
    const ProfileLimitKey limit_key = extended ? ProfileLimitKey::RandomWriteBitExt : ProfileLimitKey::RandomWriteBit;
    const size_t limit = profileLimitOr(profile, limit_key, fallback);
    return (bit_count >= 1U && bit_count <= limit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateBlockRequestLimits(size_t word_block_count, size_t bit_block_count, size_t total_points, CompatibilityMode mode) {
    const size_t total_blocks = word_block_count + bit_block_count;
    if (total_blocks < 1U || total_blocks > blockCountLimit(mode)) {
        return Error::InvalidArgument;
    }
    return (total_points <= kDirectWordPointLimit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateBlockWriteRequestLimits(size_t word_block_count, size_t bit_block_count, size_t total_points, CompatibilityMode mode) {
    const size_t total_blocks = word_block_count + bit_block_count;
    Error err = validateBlockRequestLimits(word_block_count, bit_block_count, total_points, mode);
    if (err != Error::Ok) {
        return err;
    }
    const size_t weighted = total_points + (total_blocks * blockWriteOverheadPerBlock(mode));
    return (weighted <= kDirectWordPointLimit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateMemoryWordLength(size_t word_length) {
    return (word_length >= 1U && word_length <= kMemoryWordLimit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateExtendUnitByteLength(size_t byte_length) {
    return (byte_length >= 2U && byte_length <= kExtendUnitByteLimit) ? Error::Ok : Error::InvalidArgument;
}

inline Error validateExtendUnitWordLength(size_t word_length) {
    return (word_length >= 1U && word_length <= kDirectWordPointLimit) ? Error::Ok : Error::InvalidArgument;
}

inline size_t encodeDeviceSpec(const DeviceAddress& device, CompatibilityMode mode, uint8_t* out, size_t capacity) {
    if (mode == CompatibilityMode::Legacy) {
        if (capacity < 4) return 0;
        out[0] = (uint8_t)(device.number() & 0xFFU);
        out[1] = (uint8_t)((device.number() >> 8U) & 0xFFU);
        out[2] = (uint8_t)((device.number() >> 16U) & 0xFFU);
        out[3] = (uint8_t)device.code();
        return 4;
    } else {
        if (capacity < 6) return 0;
        writeLe32(out, device.number());
        writeLe16(out + 4, static_cast<uint16_t>(device.code()));
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

inline bool isReadOnlyDevice(DeviceCode code, PlcProfile profile = PlcProfile::Unspecified) {
    return isProfileReadOnlyDevice(profile, code);
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

inline bool isLongCurrentValueDevice(DeviceCode code) {
    switch (code) {
        case DeviceCode::LTN:
        case DeviceCode::LSTN:
        case DeviceCode::LCN:
            return true;
        default:
            return false;
    }
}

inline bool isDwordOnlyDirectWordDevice(DeviceCode code) {
    return code == DeviceCode::LCN || code == DeviceCode::LZ;
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

inline bool isLongFamilyStateWriteDevice(DeviceCode code) {
    return isLongTimerStateReadOnlyDevice(code) || isLongCounterContactDevice(code);
}

inline bool isLongCurrentOrDwordOnlyDevice(DeviceCode code) {
    return isLongCurrentValueDevice(code) || isDwordOnlyDirectWordDevice(code);
}

inline Error validateDirectWordReadDevice(const DeviceAddress& device, uint16_t points, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    if (isLongTimerCurrentBlockDevice(device.code()) && (points == 0U || (points % 4U) != 0U)) {
        return Error::UnsupportedDevice;
    }
    if (isDwordOnlyDirectWordDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectBitReadDevice(const DeviceAddress& device, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code()) || isLongTimerStateReadOnlyDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectBitWriteDevice(const DeviceAddress& device, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code()) || isReadOnlyDevice(device.code(), profile) || isLongFamilyStateWriteDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectDWordReadDevice(const DeviceAddress& device, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code()) || isLongTimerCurrentBlockDevice(device.code()) || isDwordOnlyDirectWordDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectWordWriteDevice(const DeviceAddress& device, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code()) || isReadOnlyDevice(device.code(), profile) ||
        isLongTimerCurrentBlockDevice(device.code()) || isDwordOnlyDirectWordDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectDWordWriteDevice(const DeviceAddress& device, PlcProfile profile) {
    if (device.profile() != profile || profile == PlcProfile::Unspecified) return Error::InvalidArgument;
    if (isUnsupportedDirectDevice(device.code()) || isReadOnlyDevice(device.code(), profile) ||
        isLongTimerCurrentBlockDevice(device.code()) || isDwordOnlyDirectWordDevice(device.code())) {
        return Error::UnsupportedDevice;
    }
    return Error::Ok;
}

inline Error validateDirectDeviceList(const DeviceAddress* devices, size_t count, PlcProfile profile) {
    if (count == 0) {
        return Error::Ok;
    }
    if (devices == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (devices[i].profile() != profile || profile == PlcProfile::Unspecified) {
            return Error::InvalidArgument;
        }
        if (isUnsupportedDirectDevice(devices[i].code())) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline Error validateLongTimerReadRange(int head_no, int points, PlcProfile profile) {
    if (head_no < 0 || points <= 0) return Error::InvalidArgument;
    const size_t word_points = static_cast<size_t>(points) * 4U;
    return validateDirectWordAccessPoints(word_points, profile, false);
}

inline bool addressRangesOverlap(const DeviceAddress& left, uint32_t left_points,
                                 const DeviceAddress& right, uint32_t right_points) {
    if (left.profile() != right.profile() || left.code() != right.code() || left_points == 0U || right_points == 0U) {
        return false;
    }
    const uint64_t left_end = static_cast<uint64_t>(left.number()) + left_points - 1U;
    const uint64_t right_end = static_cast<uint64_t>(right.number()) + right_points - 1U;
    return static_cast<uint64_t>(left.number()) <= right_end && static_cast<uint64_t>(right.number()) <= left_end;
}

inline Error validateNoRandomWriteOverlap(const DeviceAddress* word_devices, size_t word_count,
                                          const DeviceAddress* dword_devices, size_t dword_count) {
    for (size_t i = 0; i < word_count; ++i) {
        for (size_t j = i + 1U; j < word_count; ++j) {
            if (addressRangesOverlap(word_devices[i], 1U, word_devices[j], 1U)) return Error::InvalidArgument;
        }
        for (size_t j = 0; j < dword_count; ++j) {
            if (addressRangesOverlap(word_devices[i], 1U, dword_devices[j], 2U)) return Error::InvalidArgument;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        for (size_t j = i + 1U; j < dword_count; ++j) {
            if (addressRangesOverlap(dword_devices[i], 2U, dword_devices[j], 2U)) return Error::InvalidArgument;
        }
    }
    return Error::Ok;
}

inline Error validateNoBitWriteDuplicates(const DeviceAddress* devices, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1U; j < count; ++j) {
            if (addressRangesOverlap(devices[i], 1U, devices[j], 1U)) return Error::InvalidArgument;
        }
    }
    return Error::Ok;
}

inline Error validateNoBlockWriteOverlap(const DeviceBlockWrite* word_blocks, size_t word_count,
                                         const DeviceBlockWrite* bit_blocks, size_t bit_count) {
    for (size_t i = 0; i < word_count; ++i) {
        for (size_t j = i + 1U; j < word_count; ++j) {
            if (addressRangesOverlap(word_blocks[i].device, word_blocks[i].points,
                                     word_blocks[j].device, word_blocks[j].points)) return Error::InvalidArgument;
        }
        for (size_t j = 0; j < bit_count; ++j) {
            if (addressRangesOverlap(word_blocks[i].device, word_blocks[i].points,
                                     bit_blocks[j].device, bit_blocks[j].points)) return Error::InvalidArgument;
        }
    }
    for (size_t i = 0; i < bit_count; ++i) {
        for (size_t j = i + 1U; j < bit_count; ++j) {
            if (addressRangesOverlap(bit_blocks[i].device, bit_blocks[i].points,
                                     bit_blocks[j].device, bit_blocks[j].points)) return Error::InvalidArgument;
        }
    }
    return Error::Ok;
}

inline bool isValidHgModuleSlot(uint16_t slot) {
    return slot >= 0x03E0U && slot <= 0x03E3U;
}

inline Error validateExtDeviceSpec(const ExtDeviceSpec& spec) {
    if (spec.kind == ExtDeviceSpec::Kind::ModuleBuf && spec.mod.use_hg && !isValidHgModuleSlot(spec.mod.slot)) {
        return Error::InvalidArgument;
    }
    return Error::Ok;
}

inline bool isLinkDirectDeviceCode(DeviceCode code) {
    return code == DeviceCode::X ||
           code == DeviceCode::Y ||
           code == DeviceCode::B ||
           code == DeviceCode::SB ||
           code == DeviceCode::W ||
           code == DeviceCode::SW;
}

inline Error validateExtRandomReadDevices(
    const ExtDeviceSpec* word_devices,
    size_t word_count,
    const ExtDeviceSpec* dword_devices,
    size_t dword_count
) {
    if ((word_count > 0 && word_devices == nullptr) || (dword_count > 0 && dword_devices == nullptr)) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < word_count; ++i) {
        Error spec_error = validateExtDeviceSpec(word_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (word_devices[i].kind != ExtDeviceSpec::Kind::LinkDirect) {
            continue;
        }
        DeviceCode code = word_devices[i].link.code;
        if (!isLinkDirectDeviceCode(code)) {
            return Error::UnsupportedDevice;
        }
        if (isLongTimerStateReadOnlyDevice(code) ||
            isLongCounterContactDevice(code) ||
            isLongCurrentOrDwordOnlyDevice(code)) {
            return Error::UnsupportedDevice;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        Error spec_error = validateExtDeviceSpec(dword_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (dword_devices[i].kind != ExtDeviceSpec::Kind::LinkDirect) {
            continue;
        }
        DeviceCode code = dword_devices[i].link.code;
        if (!isLinkDirectDeviceCode(code)) {
            return Error::UnsupportedDevice;
        }
        if (isLongTimerStateReadOnlyDevice(code) || isLongCounterContactDevice(code)) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline Error validateExtRandomWriteWordDevices(const ExtDeviceSpec* word_devices, size_t word_count, PlcProfile profile) {
    if (word_count == 0) {
        return Error::Ok;
    }
    if (word_devices == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < word_count; ++i) {
        Error spec_error = validateExtDeviceSpec(word_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (word_devices[i].kind == ExtDeviceSpec::Kind::LinkDirect &&
            (!isLinkDirectDeviceCode(word_devices[i].link.code) ||
             isReadOnlyDevice(word_devices[i].link.code, profile) ||
             isLongCurrentOrDwordOnlyDevice(word_devices[i].link.code))) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline Error validateExtRandomBitWriteDevices(const ExtDeviceSpec* bit_devices, size_t bit_count, PlcProfile profile) {
    if (bit_count == 0) {
        return Error::Ok;
    }
    if (bit_devices == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < bit_count; ++i) {
        Error spec_error = validateExtDeviceSpec(bit_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (bit_devices[i].kind == ExtDeviceSpec::Kind::ModuleBuf) {
            return Error::UnsupportedDevice;
        }
        if (bit_devices[i].kind == ExtDeviceSpec::Kind::LinkDirect &&
            (!isLinkDirectDeviceCode(bit_devices[i].link.code) ||
             isReadOnlyDevice(bit_devices[i].link.code, profile) ||
             bit_devices[i].link.code == DeviceCode::G ||
             bit_devices[i].link.code == DeviceCode::HG)) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline bool extAddressRangesOverlap(const ExtDeviceSpec& left, uint32_t left_points,
                                    const ExtDeviceSpec& right, uint32_t right_points) {
    if (left.kind != right.kind || left_points == 0U || right_points == 0U) return false;
    uint32_t left_start = 0U;
    uint32_t right_start = 0U;
    if (left.kind == ExtDeviceSpec::Kind::ModuleBuf) {
        if (left.mod.slot != right.mod.slot || left.mod.use_hg != right.mod.use_hg) return false;
        left_start = left.mod.dev_no;
        right_start = right.mod.dev_no;
    } else {
        if (left.link.j_net != right.link.j_net || left.link.code != right.link.code) return false;
        left_start = left.link.dev_no;
        right_start = right.link.dev_no;
    }
    const uint64_t left_end = static_cast<uint64_t>(left_start) + left_points - 1U;
    const uint64_t right_end = static_cast<uint64_t>(right_start) + right_points - 1U;
    return static_cast<uint64_t>(left_start) <= right_end &&
           static_cast<uint64_t>(right_start) <= left_end;
}

inline Error validateNoExtRandomWriteOverlap(const ExtDeviceSpec* word_devices, size_t word_count,
                                             const ExtDeviceSpec* dword_devices, size_t dword_count) {
    for (size_t i = 0; i < word_count; ++i) {
        for (size_t j = i + 1U; j < word_count; ++j) {
            if (extAddressRangesOverlap(word_devices[i], 1U, word_devices[j], 1U)) return Error::InvalidArgument;
        }
        for (size_t j = 0; j < dword_count; ++j) {
            if (extAddressRangesOverlap(word_devices[i], 1U, dword_devices[j], 2U)) return Error::InvalidArgument;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        for (size_t j = i + 1U; j < dword_count; ++j) {
            if (extAddressRangesOverlap(dword_devices[i], 2U, dword_devices[j], 2U)) return Error::InvalidArgument;
        }
    }
    return Error::Ok;
}

inline Error validateNoExtBitWriteDuplicates(const ExtDeviceSpec* bit_devices, size_t bit_count) {
    for (size_t i = 0; i < bit_count; ++i) {
        for (size_t j = i + 1U; j < bit_count; ++j) {
            if (extAddressRangesOverlap(bit_devices[i], 1U, bit_devices[j], 1U)) return Error::InvalidArgument;
        }
    }
    return Error::Ok;
}

inline Error validateExtMonitorDevices(
    const ExtDeviceSpec* word_devices,
    size_t word_count,
    const ExtDeviceSpec* dword_devices,
    size_t dword_count
) {
    if ((word_count > 0 && word_devices == nullptr) || (dword_count > 0 && dword_devices == nullptr)) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < word_count; ++i) {
        Error spec_error = validateExtDeviceSpec(word_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (word_devices[i].kind == ExtDeviceSpec::Kind::LinkDirect &&
            isLongCounterContactDevice(word_devices[i].link.code)) {
            return Error::UnsupportedDevice;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        Error spec_error = validateExtDeviceSpec(dword_devices[i]);
        if (spec_error != Error::Ok) {
            return spec_error;
        }
        if (dword_devices[i].kind == ExtDeviceSpec::Kind::LinkDirect &&
            isLongCounterContactDevice(dword_devices[i].link.code)) {
            return Error::UnsupportedDevice;
        }
    }
    return Error::Ok;
}

inline Error summarizeBlockReadList(const DeviceBlockRead* blocks, size_t count, size_t& total_points, PlcProfile profile) {
    total_points = 0;
    if (count == 0) {
        return Error::Ok;
    }
    if (blocks == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (blocks[i].device.profile() != profile || profile == PlcProfile::Unspecified) {
            return Error::InvalidArgument;
        }
        if (blocks[i].points == 0) {
            return Error::InvalidArgument;
        }
        if (isUnsupportedDirectDevice(blocks[i].device.code()) ||
            isLongCounterContactDevice(blocks[i].device.code()) ||
            isDwordOnlyDirectWordDevice(blocks[i].device.code()) ||
            (isLongTimerCurrentBlockDevice(blocks[i].device.code()) && ((blocks[i].points % 4U) != 0U))) {
            return Error::UnsupportedDevice;
        }
        total_points += blocks[i].points;
    }
    return Error::Ok;
}

inline Error summarizeBlockWriteList(const DeviceBlockWrite* blocks, size_t count, size_t& total_points, PlcProfile profile) {
    total_points = 0;
    if (count == 0) {
        return Error::Ok;
    }
    if (blocks == nullptr) {
        return Error::InvalidArgument;
    }
    for (size_t i = 0; i < count; ++i) {
        if (blocks[i].device.profile() != profile || profile == PlcProfile::Unspecified) {
            return Error::InvalidArgument;
        }
        if (blocks[i].points == 0 || blocks[i].values == nullptr) {
            return Error::InvalidArgument;
        }
        if (isUnsupportedDirectDevice(blocks[i].device.code()) ||
            isReadOnlyDevice(blocks[i].device.code(), profile) ||
            isLongCounterContactDevice(blocks[i].device.code()) ||
            isLongCurrentOrDwordOnlyDevice(blocks[i].device.code())) {
            return Error::UnsupportedDevice;
        }
        total_points += blocks[i].points;
    }
    return Error::Ok;
}

inline Error encodeRemotePasswordPayload(
    const char* password,
    CompatibilityMode mode,
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

    if (mode == CompatibilityMode::Legacy && password_length != 4U) {
        return Error::InvalidArgument;
    }
    if (mode == CompatibilityMode::iQR && password_length < 6U) {
        return Error::InvalidArgument;
    }
    if (capacity < (2U + password_length)) {
        return Error::BufferTooSmall;
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
    for (size_t i = 0; i < data_length; ++i) {
        const uint8_t value = data[i];
        if (!((value >= static_cast<uint8_t>('0') && value <= static_cast<uint8_t>('9')) ||
              (value >= static_cast<uint8_t>('A') && value <= static_cast<uint8_t>('F')))) {
            return Error::InvalidArgument;
        }
    }
    if (out == nullptr || capacity < (2U + data_length)) {
        return Error::BufferTooSmall;
    }
    writeLe16(out, static_cast<uint16_t>(data_length));
    memcpy(out + 2, data, data_length);
    payload_length = 2U + data_length;
    return Error::Ok;
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
    DeviceAddress dev{PlcProfile::Unspecified, use_hg ? DeviceCode::HG : DeviceCode::G, dev_no};
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
    PlcProfile profile,
    const TargetAddress& target,
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
      target_(target),
      plc_profile_(isConnectionSelectablePlcProfile(profile) ? profile : PlcProfile::Unspecified),
      frame_type_(frameTypeForPlcProfile(profile)),
      compatibility_mode_(compatibilityModeForPlcProfile(profile)),
      block_access_enabled_(true),
      strict_profile_(true),
      monitoring_timer_(0x0010),
      timeout_ms_(3000),
      serial_(0),
      last_error_(Error::Ok),
      last_end_code_(0),
      last_error_info_(),
      last_profile_feature_error_info_(),
      last_request_length_(0),
      last_response_length_(0),
      request_count_(0),
      tx_bytes_(0),
      rx_bytes_(0),
      state_(State::Idle),
      bytes_transferred_(0),
      last_activity_ms_(0),
      async_ctx_{} {
    if (!isConnectionSelectablePlcProfile(profile)) {
        setError(Error::InvalidArgument);
    }
}

inline bool isValidLabelName(const LabelName& name) {
    if (name.chars == nullptr || name.length == 0U) return false;
    for (uint16_t i = 0U; i < name.length; ++i) {
        const uint16_t ch = name.chars[i];
        if (ch != 0x0020U && (ch < 0x0009U || ch > 0x000DU)) return true;
    }
    return false;
}

inline bool isValidLabelList(const LabelName* labels, size_t count) {
    if (count == 0U) return true;
    if (labels == nullptr || count > 0xFFFFU) return false;
    for (size_t i = 0; i < count; ++i) {
        if (!isValidLabelName(labels[i])) return false;
    }
    return true;
}

inline bool hasValidAbbreviationReferences(const LabelName& name, size_t abbreviation_count) {
    if (!isValidLabelName(name)) return false;
    size_t index = 0U;
    while (index < name.length) {
        if (name.chars[index] != static_cast<uint16_t>('%')) {
            ++index;
            continue;
        }
        size_t digit_end = index + 1U;
        size_t reference = 0U;
        while (digit_end < name.length && name.chars[digit_end] >= static_cast<uint16_t>('0') &&
               name.chars[digit_end] <= static_cast<uint16_t>('9')) {
            const size_t digit = static_cast<size_t>(name.chars[digit_end] - static_cast<uint16_t>('0'));
            if (reference > (0xFFFFU - digit) / 10U) return false;
            reference = reference * 10U + digit;
            ++digit_end;
        }
        if (digit_end == index + 1U || reference == 0U || reference > abbreviation_count) return false;
        index = digit_end;
    }
    return true;
}

/** @brief Returns true if an asynchronous operation is currently active. */
bool SlmpClient::isBusy() const {
    return state_ != State::Idle;
}

bool SlmpClient::connect(const char* host, uint16_t port) {
    if (isBusy()) {
        transport_.close();
        resetAsyncState();
        setError(Error::TransportError);
    }
    if (host == nullptr || host[0] == '\0' || port == 0 ||
        !isConnectionSelectablePlcProfile(plc_profile_) ||
        tx_buffer_ == nullptr || rx_buffer_ == nullptr ||
        tx_capacity_ < kRequestHeaderSize4E || rx_capacity_ < kResponsePrefixSize4E + 2U) {
        setError(Error::InvalidArgument);
        return false;
    }
    if (!transport_.connect(host, port)) {
        resetAsyncState();
        setError(Error::TransportError);
        return false;
    }
    resetAsyncState();
    setError(Error::Ok);
    return true;
}

void SlmpClient::close() {
    const bool interrupted = isBusy();
    transport_.close();
    resetAsyncState();
    if (interrupted) {
        setError(Error::TransportError);
    }
}

bool SlmpClient::connected() const {
    return transport_.connected();
}

const TargetAddress& SlmpClient::target() const {
    return target_;
}

FrameType SlmpClient::frameType() const {
    return frame_type_;
}

CompatibilityMode SlmpClient::compatibilityMode() const {
    return compatibility_mode_;
}

Error SlmpClient::setPlcProfile(PlcProfile profile) {
    if (!isConnectionSelectablePlcProfile(profile)) {
        return Error::InvalidArgument;
    }
    if (isBusy()) {
        return Error::Busy;
    }
    plc_profile_ = profile;
    frame_type_ = frameTypeForPlcProfile(profile);
    compatibility_mode_ = compatibilityModeForPlcProfile(profile);
    block_access_enabled_ = !profileDisablesBlockAccess(profile);
    return Error::Ok;
}

Error SlmpClient::setManualProfile(PlcProfile profile, FrameType frame_type, CompatibilityMode mode) {
    if (!isConnectionSelectablePlcProfile(profile) ||
        !isValidFrameType(frame_type) ||
        !isValidCompatibilityMode(mode)) {
        return Error::InvalidArgument;
    }
    if (isBusy()) {
        return Error::Busy;
    }
    plc_profile_ = profile;
    frame_type_ = frame_type;
    compatibility_mode_ = mode;
    block_access_enabled_ = !profileDisablesBlockAccess(profile);
    return Error::Ok;
}

PlcProfile SlmpClient::plcProfile() const {
    return plc_profile_;
}

void SlmpClient::setBlockAccessEnabled(bool enabled) {
    block_access_enabled_ = enabled;
}

bool SlmpClient::blockAccessEnabled() const {
    return block_access_enabled_ &&
           !profileDisablesBlockAccess(plc_profile_) &&
           !(strict_profile_ && profileFeatureWouldBlock(plc_profile_, ProfileFeatureKey::Block));
}

void SlmpClient::setMonitoringTimer(uint16_t monitoring_timer) {
    monitoring_timer_ = monitoring_timer;
}

uint16_t SlmpClient::monitoringTimer() const {
    return monitoring_timer_;
}

Error SlmpClient::setTimeoutMs(uint32_t timeout_ms) {
    if (timeout_ms == 0U) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    timeout_ms_ = timeout_ms;
    setError(Error::Ok);
    return last_error_;
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

bool SlmpClient::hasLastErrorInfo() const {
    return last_error_info_.present;
}

const SlmpErrorInfo& SlmpClient::lastErrorInfo() const {
    return last_error_info_;
}

bool SlmpClient::hasLastProfileFeatureErrorInfo() const {
    return last_profile_feature_error_info_.present;
}

const ProfileFeatureErrorInfo& SlmpClient::lastProfileFeatureErrorInfo() const {
    return last_profile_feature_error_info_;
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

TrafficStats SlmpClient::trafficStats() const {
    return TrafficStats{request_count_, tx_bytes_, rx_bytes_};
}

Error SlmpClient::startAsync(AsyncContext::Type type, size_t payload_length, uint32_t now_ms) {
    if (!isConnectionSelectablePlcProfile(plc_profile_)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (isBusy()) {
        setError(Error::Busy);
        return last_error_;
    }
    if (!transport_.connected()) {
        setError(Error::NotConnected);
        return last_error_;
    }
    if (!isSpecifiedPlcProfile(plc_profile_)) {
        setError(Error::InvalidArgument);
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
            subcommand = 0x0000U;
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

Error SlmpClient::ensureBeginIdle() const {
    return isBusy() ? Error::Busy : Error::Ok;
}

void SlmpClient::resetAsyncState() {
    state_ = State::Idle;
    bytes_transferred_ = 0U;
    async_ctx_ = AsyncContext{};
}

void SlmpClient::update(uint32_t now_ms) {
    if (state_ == State::Idle) return;

    if (now_ms - last_activity_ms_ >= timeout_ms_) {
        transport_.close();
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
                ++request_count_;
                tx_bytes_ += static_cast<uint64_t>(last_request_length_);
                if (async_ctx_.type == AsyncContext::Type::RemoteReset) {
                    transport_.close();
                    resetAsyncState();
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
                        transport_.close();
                        setError(Error::ProtocolError);
                        state_ = State::Idle;
                        return;
                    }
                    response_data_length = readLe16(rx_buffer_ + 11);
                } else {
                    if (rx_buffer_[0] != kResponseSubheader3E0 || rx_buffer_[1] != kResponseSubheader3E1) {
                        transport_.close();
                        setError(Error::ProtocolError);
                        state_ = State::Idle;
                        return;
                    }
                    response_data_length = readLe16(rx_buffer_ + 7);
                }

                if (response_data_length < 2U) {
                    transport_.close();
                    setError(Error::ProtocolError);
                    state_ = State::Idle;
                    return;
                }
                last_response_length_ = response_prefix_size + response_data_length;
                if (rx_capacity_ < last_response_length_) {
                    transport_.close();
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
                rx_bytes_ += static_cast<uint64_t>(last_response_length_);
                completeAsync();
            }
        }
    }
}

void SlmpClient::completeAsync() {
    size_t response_prefix_size = (frame_type_ == FrameType::Frame4E) ? kResponsePrefixSize4E : kResponsePrefixSize3E;
    if (frame_type_ == FrameType::Frame4E) {
        if (readLe16(rx_buffer_ + 2) != readLe16(tx_buffer_ + 2)) {
            transport_.close();
            setError(Error::ProtocolError);
            return;
        }
    }
    uint16_t end_code = readLe16(rx_buffer_ + response_prefix_size);
    if (end_code != 0) {
        const uint8_t* error_data = rx_buffer_ + response_prefix_size + 2U;
        const size_t error_length = last_response_length_ - (response_prefix_size + 2U);
        setPlcError(end_code, error_data, error_length);
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
            const size_t request_header_size =
                (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
            if (last_request_length_ < request_header_size + 2U) {
                setError(Error::ProtocolError);
                return;
            }
            const size_t expected_length = readLe16(tx_buffer_ + request_header_size);
            if (loopback_size != expected_length ||
                last_request_length_ != request_header_size + 2U + expected_length) {
                setError(Error::ProtocolError);
                return;
            }
            if (memcmp(response_data + 2U, tx_buffer_ + request_header_size + 2U, loopback_size) != 0) {
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
            if (response_length != byte_length) {
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::TypeName);
    if (guard_error != Error::Ok) return guard_error;
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
    Error err = readWords(dev::SD(plc_profile_, dev::dec(203)), 1, &status_word, 1);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || value_capacity < points || validateDirectWordAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectWordReadDevice(device, points, plc_profile_);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectWordAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectWordWriteDevice(device, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || value_capacity < points || validateDirectBitAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectBitReadDevice(device, plc_profile_);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectBitAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectBitWriteDevice(device, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || value_capacity < points || validateDirectDWordAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectDWordReadDevice(device, plc_profile_);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectDWordAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectDWordWriteDevice(device, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || value_capacity < points || validateDirectDWordAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectDWordReadDevice(device, plc_profile_);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Direct);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectDWordAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    Error validate_error = validateDirectDWordWriteDevice(device, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::LongDevicePath);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 8U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    uint16_t word_points = static_cast<uint16_t>(static_cast<size_t>(points) * 4U);
    DeviceAddress dev{plc_profile_, DeviceCode::LTN, static_cast<uint32_t>(head_no)};
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::LongDevicePath);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (tx_capacity_ < 8U) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    uint16_t word_points = static_cast<uint16_t>(static_cast<size_t>(points) * 4U);
    DeviceAddress dev{plc_profile_, DeviceCode::LSTN, static_cast<uint32_t>(head_no)};
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
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isAligned(rx_buffer_, alignof(LongTimerResult))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    initializeLongTimerResults(tmp, points);
    Error err = readLongTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].coil;
    return last_error_;
}

Error SlmpClient::readLtsStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isAligned(rx_buffer_, alignof(LongTimerResult))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    initializeLongTimerResults(tmp, points);
    Error err = readLongTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].contact;
    return last_error_;
}

Error SlmpClient::readLstcStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isAligned(rx_buffer_, alignof(LongTimerResult))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    initializeLongTimerResults(tmp, points);
    Error err = readLongRetentiveTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].coil;
    return last_error_;
}

Error SlmpClient::readLstsStates(int head_no, int points, bool* out, size_t capacity) {
    if (out == nullptr || validateLongTimerReadRange(head_no, points, plc_profile_) != Error::Ok ||
        capacity < static_cast<size_t>(points)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isAligned(rx_buffer_, alignof(LongTimerResult))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    LongTimerResult* tmp = reinterpret_cast<LongTimerResult*>(rx_buffer_);
    size_t tmp_capacity = rx_capacity_ / sizeof(LongTimerResult);
    if (tmp_capacity < static_cast<size_t>(points)) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }
    initializeLongTimerResults(tmp, points);
    Error err = readLongRetentiveTimer(head_no, points, tmp, tmp_capacity);
    if (err != Error::Ok) return err;
    for (int i = 0; i < points; ++i) out[i] = tmp[i].contact;
    return last_error_;
}

// -----------------------------------------------------------------------
// Module Buffer (Intelligent Module Ux\G / Ux\HG)
// -----------------------------------------------------------------------

Error SlmpClient::beginReadWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t* out, size_t capacity, uint32_t now_ms) {
    Error guard_error = ensureProfileFeatureAllowed(use_hg ? ProfileFeatureKey::HgCpuBuffer : ProfileFeatureKey::ExtModuleAccess);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || capacity < points || validateDirectWordAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (use_hg && !isValidHgModuleSlot(slot)) {
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
    Error guard_error = ensureProfileFeatureAllowed(use_hg ? ProfileFeatureKey::HgCpuBuffer : ProfileFeatureKey::ExtModuleAccess);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectWordAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (use_hg && !isValidHgModuleSlot(slot)) {
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
    Error guard_error = ensureProfileFeatureAllowed(use_hg ? ProfileFeatureKey::HgCpuBuffer : ProfileFeatureKey::ExtModuleAccess);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || capacity < points || validateDirectBitAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (use_hg && !isValidHgModuleSlot(slot)) {
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
    Error guard_error = ensureProfileFeatureAllowed(use_hg ? ProfileFeatureKey::HgCpuBuffer : ProfileFeatureKey::ExtModuleAccess);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectBitAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (use_hg && !isValidHgModuleSlot(slot)) {
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::ExtLinkDirect);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || capacity < points || validateDirectWordAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isLinkDirectDeviceCode(code)) {
        setError(Error::UnsupportedDevice);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::ExtLinkDirect);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectWordAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isLinkDirectDeviceCode(code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }
    // Link-direct writes use the same device families as direct writes; keep
    // them from bypassing read-only and qualified-only policy checks.
    Error validate_error = validateDirectWordWriteDevice({plc_profile_, code, dev_no}, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::ExtLinkDirect);
    if (guard_error != Error::Ok) return guard_error;
    if (out == nullptr || capacity < points || validateDirectBitAccessPoints(points, plc_profile_, false) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isLinkDirectDeviceCode(code)) {
        setError(Error::UnsupportedDevice);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::ExtLinkDirect);
    if (guard_error != Error::Ok) return guard_error;
    if (values == nullptr || validateDirectBitAccessPoints(count, plc_profile_, true) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if (!isLinkDirectDeviceCode(code)) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }
    // Link-direct writes use the same device families as direct writes; keep
    // them from bypassing read-only and qualified-only policy checks.
    Error validate_error = validateDirectBitWriteDevice({plc_profile_, code, dev_no}, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (out == nullptr || capacity < word_length || validateMemoryWordLength(word_length) != Error::Ok) {
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (values == nullptr || validateMemoryWordLength(count) != Error::Ok) {
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (out == nullptr || capacity < byte_length || validateExtendUnitByteLength(byte_length) != Error::Ok) {
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
    if (out == nullptr || capacity < word_length || validateExtendUnitWordLength(word_length) != Error::Ok) {
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (data == nullptr || validateExtendUnitByteLength(byte_length) != Error::Ok) {
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
    if (values == nullptr || validateExtendUnitWordLength(count) != Error::Ok) {
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

// -----------------------------------------------------------------------
// Label Read / Write
// -----------------------------------------------------------------------

Error SlmpClient::beginReadArrayLabels(
    const LabelArrayReadPoint* points, size_t point_count,
    LabelArrayReadResult* out, size_t out_capacity, size_t* out_count,
    const LabelName* abbrevs, size_t abbrev_count,
    uint32_t now_ms
) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (points == nullptr || point_count == 0 || point_count > 0xFFFFU ||
        out == nullptr || out_capacity < point_count || !isValidLabelList(abbrevs, abbrev_count)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    for (size_t i = 0; i < point_count; ++i) {
        if (!hasValidAbbreviationReferences(points[i].label, abbrev_count) || points[i].unit_specification > 1U ||
            points[i].array_data_length == 0U) {
            setError(Error::InvalidArgument);
            return last_error_;
        }
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (points == nullptr || point_count == 0 || point_count > 0xFFFFU ||
        !isValidLabelList(abbrevs, abbrev_count)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    for (size_t i = 0; i < point_count; ++i) {
        const size_t expected_bytes = points[i].unit_specification == 0U
            ? static_cast<size_t>(points[i].array_data_length) * 2U
            : static_cast<size_t>(points[i].array_data_length);
        if (!hasValidAbbreviationReferences(points[i].label, abbrev_count) || points[i].unit_specification > 1U ||
            points[i].array_data_length == 0U || points[i].data == nullptr ||
            points[i].data_bytes != expected_bytes) {
            setError(Error::InvalidArgument);
            return last_error_;
        }
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (!isValidLabelList(labels, label_count) || label_count == 0U ||
        out == nullptr || out_capacity < label_count || !isValidLabelList(abbrevs, abbrev_count)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    for (size_t i = 0U; i < label_count; ++i) {
        if (!hasValidAbbreviationReferences(labels[i], abbrev_count)) {
            setError(Error::InvalidArgument);
            return last_error_;
        }
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (points == nullptr || point_count == 0 || point_count > 0xFFFFU ||
        !isValidLabelList(abbrevs, abbrev_count)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    for (size_t i = 0; i < point_count; ++i) {
        if (!hasValidAbbreviationReferences(points[i].label, abbrev_count) || points[i].data == nullptr || points[i].data_bytes == 0U) {
            setError(Error::InvalidArgument);
            return last_error_;
        }
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomReadLikeCounts(word_count, dword_count, compatibility_mode_, false, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if ((word_count > 0 && (word_values == nullptr || word_value_capacity < word_count)) ||
        (dword_count > 0 && (dword_values == nullptr || dword_value_capacity < dword_count))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateDirectDeviceList(dword_devices, dword_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        if (isLongTimerStateReadOnlyDevice(word_devices[i].code()) ||
            isLongCounterContactDevice(word_devices[i].code()) ||
            isLongCurrentOrDwordOnlyDevice(word_devices[i].code())) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (isLongTimerStateReadOnlyDevice(dword_devices[i].code()) || isLongCounterContactDevice(dword_devices[i].code())) {
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomWriteWordCounts(word_count, dword_count, compatibility_mode_, false, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr))) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        if (isReadOnlyDevice(word_devices[i].code(), plc_profile_) || isLongCurrentOrDwordOnlyDevice(word_devices[i].code())) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    validate_error = validateDirectDeviceList(dword_devices, dword_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (isReadOnlyDevice(dword_devices[i].code(), plc_profile_)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    validate_error = validateNoRandomWriteOverlap(word_devices, word_count, dword_devices, dword_count);
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (bit_devices == nullptr || bit_values == nullptr ||
        validateRandomBitWriteCount(bit_count, compatibility_mode_, false, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    Error validate_error = validateDirectDeviceList(bit_devices, bit_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < bit_count; ++i) {
        if (isReadOnlyDevice(bit_devices[i].code(), plc_profile_)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    validate_error = validateNoBitWriteDuplicates(bit_devices, bit_count);
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (profileDisablesBlockAccess(plc_profile_) || !block_access_enabled_) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Block);
    if (guard_error != Error::Ok) return guard_error;
    if ((word_block_count == 0 && bit_block_count == 0) || word_block_count > 0xFFU || bit_block_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t total_word_points = 0;
    size_t total_bit_points = 0;
    Error validate_error = summarizeBlockReadList(word_blocks, word_block_count, total_word_points, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = summarizeBlockReadList(bit_blocks, bit_block_count, total_bit_points, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateBlockRequestLimits(
        word_block_count,
        bit_block_count,
        total_word_points + total_bit_points,
        compatibility_mode_
    );
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
    uint32_t now_ms
) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (profileDisablesBlockAccess(plc_profile_) || !block_access_enabled_) {
        setError(Error::UnsupportedDevice);
        return last_error_;
    }
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Block);
    if (guard_error != Error::Ok) return guard_error;
    if ((word_block_count == 0 && bit_block_count == 0) || word_block_count > 0xFFU ||
        bit_block_count > 0xFFU) {
        setError(Error::InvalidArgument);
        return last_error_;
    }

    size_t total_word_points = 0;
    size_t total_bit_points = 0;
    Error validate_error = summarizeBlockWriteList(word_blocks, word_block_count, total_word_points, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = summarizeBlockWriteList(bit_blocks, bit_block_count, total_bit_points, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateBlockWriteRequestLimits(
        word_block_count,
        bit_block_count,
        total_word_points + total_bit_points,
        compatibility_mode_
    );
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateNoBlockWriteOverlap(word_blocks, word_block_count, bit_blocks, bit_block_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    size_t spec_size = (compatibility_mode_ == CompatibilityMode::Legacy) ? 4U : 6U;
    size_t payload_length =
        2U + ((word_block_count + bit_block_count) * (spec_size + 2U)) +
        ((total_word_points + total_bit_points) * 2U);
    size_t request_header_size = (frame_type_ == FrameType::Frame4E) ? kRequestHeaderSize4E : kRequestHeaderSize3E;
    if (tx_capacity_ < request_header_size + payload_length) {
        setError(Error::BufferTooSmall);
        return last_error_;
    }

    tx_buffer_[0] = static_cast<uint8_t>(word_block_count);
    tx_buffer_[1] = static_cast<uint8_t>(bit_block_count);

    // SLMP Write Block places each block's data immediately after that
    // block's device spec and point count. Keeping all specs first and all
    // data last only works accidentally for single-block requests.
    size_t offset = 2U;
    for (size_t i = 0; i < word_block_count; ++i) {
        size_t written =
            encodeDeviceSpec(word_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, word_blocks[i].points);
        offset += written + 2U;
        for (size_t j = 0; j < word_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, word_blocks[i].values[j]);
            offset += 2U;
        }
    }
    for (size_t i = 0; i < bit_block_count; ++i) {
        size_t written =
            encodeDeviceSpec(bit_blocks[i].device, compatibility_mode_, tx_buffer_ + offset, tx_capacity_ - offset);
        if (written == 0) {
            setError(Error::BufferTooSmall);
            return last_error_;
        }
        writeLe16(tx_buffer_ + offset + written, bit_blocks[i].points);
        offset += written + 2U;
        for (size_t j = 0; j < bit_blocks[i].points; ++j) {
            writeLe16(tx_buffer_ + offset, bit_blocks[i].values[j]);
            offset += 2U;
        }
    }

    return startAsync(AsyncContext::Type::WriteBlock, payload_length, now_ms);
}

Error SlmpClient::writeBlock(
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

Error SlmpClient::beginRemoteRun(RemoteMode mode, RemoteClearMode clear_mode, uint32_t now_ms) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if ((mode != RemoteMode::Normal && mode != RemoteMode::Force) ||
        (clear_mode != RemoteClearMode::NoClear &&
         clear_mode != RemoteClearMode::ClearExceptLatch &&
         clear_mode != RemoteClearMode::ClearAll)) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t payload_length = 0;
    const bool force = mode == RemoteMode::Force;
    Error encode_error = encodeRemoteRunPayload(
        force,
        static_cast<uint16_t>(clear_mode),
        tx_buffer_,
        tx_capacity_,
        payload_length
    );
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemoteRun, payload_length, now_ms);
}

Error SlmpClient::remoteRun(RemoteMode mode, RemoteClearMode clear_mode) {
    Error err = beginRemoteRun(mode, clear_mode, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteStop(uint32_t now_ms) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
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

Error SlmpClient::beginRemotePause(RemoteMode mode, uint32_t now_ms) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    if (mode != RemoteMode::Normal && mode != RemoteMode::Force) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
    size_t payload_length = 0;
    Error encode_error = encodeRemoteModePayload(static_cast<uint16_t>(mode), tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }

    return startAsync(AsyncContext::Type::RemotePause, payload_length, now_ms);
}

Error SlmpClient::remotePause(RemoteMode mode) {
    Error err = beginRemotePause(mode, getTimeMs());
    if (err != Error::Ok) return err;
    while (isBusy()) {
        update(getTimeMs());
    }
    return last_error_;
}

Error SlmpClient::beginRemoteLatchClear(uint32_t now_ms) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
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

Error SlmpClient::beginRemoteReset(uint32_t now_ms) {
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    size_t payload_length = 0;
    Error encode_error = encodeRemoteModePayload(0x0001U, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }
    return startAsync(AsyncContext::Type::RemoteReset, payload_length, now_ms);
}

Error SlmpClient::remoteReset() {
    Error err = beginRemoteReset(getTimeMs());
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    size_t payload_length = 0;
    Error encode_error = encodeSelfTestPayload(data, data_length, tx_buffer_, tx_capacity_, payload_length);
    if (encode_error != Error::Ok) {
        setError(encode_error);
        return last_error_;
    }
    if (out == nullptr || out_length == nullptr || out_capacity < data_length) {
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(
        password,
        compatibility_mode_,
        tx_buffer_,
        tx_capacity_,
        payload_length
    );
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
    if (ensureBeginIdle() != Error::Ok) return Error::Busy;
    size_t payload_length = 0;
    Error encode_error = encodeRemotePasswordPayload(
        password,
        compatibility_mode_,
        tx_buffer_,
        tx_capacity_,
        payload_length
    );
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
    last_error_info_ = SlmpErrorInfo{};
    last_profile_feature_error_info_ = ProfileFeatureErrorInfo{};
}

void SlmpClient::setPlcError(uint16_t end_code, const uint8_t* response_data, size_t response_length) {
    setError(Error::PlcError, end_code);
    if (response_data == nullptr || response_length < 9U) {
        return;
    }
    last_error_info_.present = true;
    last_error_info_.network = response_data[0];
    last_error_info_.station = response_data[1];
    last_error_info_.module_io = readLe16(response_data + 2U);
    last_error_info_.multidrop = response_data[4];
    last_error_info_.command = readLe16(response_data + 5U);
    last_error_info_.subcommand = readLe16(response_data + 7U);
    memcpy(last_error_info_.raw, response_data, 9U);
}

void SlmpClient::setProfileFeatureError(ProfileFeatureKey feature_key, const char* state, const char* evidence) {
    last_error_ = Error::ProfileFeatureBlocked;
    last_end_code_ = 0U;
    last_error_info_ = SlmpErrorInfo{};
    last_profile_feature_error_info_.present = true;
    last_profile_feature_error_info_.profile = plc_profile_;
    last_profile_feature_error_info_.profile_id = profileLabel(plc_profile_);
    last_profile_feature_error_info_.feature_key = featureKeyName(feature_key);
    last_profile_feature_error_info_.state = state;
    last_profile_feature_error_info_.evidence = evidence;
}

Error SlmpClient::ensureProfileFeatureAllowed(ProfileFeatureKey feature_key) {
    const Error idle_error = ensureBeginIdle();
    if (idle_error != Error::Ok) return idle_error;
    const FeatureEntry* feature = findFeatureEntry(plc_profile_, feature_key);
    if (feature == nullptr || !strict_profile_ || !isGuardedFeatureState(feature->state)) {
        return Error::Ok;
    }
    setProfileFeatureError(feature_key, feature->state, feature->evidence);
    return last_error_;
}

Error SlmpClient::ensureExtProfileFeatureAllowed(const ExtDeviceSpec& spec) {
    if (spec.kind == ExtDeviceSpec::Kind::LinkDirect) {
        return ensureProfileFeatureAllowed(ProfileFeatureKey::ExtLinkDirect);
    }
    if (spec.kind == ExtDeviceSpec::Kind::ModuleBuf && spec.mod.use_hg) {
        return ensureProfileFeatureAllowed(ProfileFeatureKey::HgCpuBuffer);
    }
    return ensureProfileFeatureAllowed(ProfileFeatureKey::ExtModuleAccess);
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
        case Error::ProfileFeatureBlocked:
            return "profile_feature_blocked";
        default:
            return "unknown_error";
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomReadLikeCounts(word_count, dword_count, compatibility_mode_, true, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument); return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr || word_value_capacity < word_count)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr || dword_value_capacity < dword_count))) {
        setError(Error::InvalidArgument); return last_error_;
    }
    Error validate_error = validateExtRandomReadDevices(word_devices, word_count, dword_devices, dword_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(word_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(dword_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }

    if (tx_capacity_ < 2U) { setError(Error::BufferTooSmall); return last_error_; }
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomWriteWordCounts(word_count, dword_count, compatibility_mode_, true, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument); return last_error_;
    }
    if ((word_count > 0 && (word_devices == nullptr || word_values == nullptr)) ||
        (dword_count > 0 && (dword_devices == nullptr || dword_values == nullptr))) {
        setError(Error::InvalidArgument); return last_error_;
    }
    Error validate_error = validateExtRandomWriteWordDevices(word_devices, word_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(word_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        validate_error = validateExtDeviceSpec(dword_devices[i]);
        if (validate_error != Error::Ok) {
            setError(validate_error);
            return last_error_;
        }
        guard_error = ensureExtProfileFeatureAllowed(dword_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
        if (dword_devices[i].kind == ExtDeviceSpec::Kind::LinkDirect &&
            isReadOnlyDevice(dword_devices[i].link.code, plc_profile_)) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    validate_error = validateNoExtRandomWriteOverlap(word_devices, word_count, dword_devices, dword_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }

    if (tx_capacity_ < 2U) { setError(Error::BufferTooSmall); return last_error_; }
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Random);
    if (guard_error != Error::Ok) return guard_error;
    if (bit_devices == nullptr || bit_values == nullptr ||
        validateRandomBitWriteCount(bit_count, compatibility_mode_, true, plc_profile_) != Error::Ok) {
        setError(Error::InvalidArgument); return last_error_;
    }
    Error validate_error = validateExtRandomBitWriteDevices(bit_devices, bit_count, plc_profile_);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    validate_error = validateNoExtBitWriteDuplicates(bit_devices, bit_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < bit_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(bit_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }

    if (tx_capacity_ < 2U) { setError(Error::BufferTooSmall); return last_error_; }
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Monitor);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomReadLikeCounts(word_count, dword_count, compatibility_mode_, false, plc_profile_, ProfileLimitKey::MonitorRegisterWord) != Error::Ok) {
        setError(Error::InvalidArgument); return last_error_;
    }

    Error validate_error = validateDirectDeviceList(word_devices, word_count, plc_profile_);
    if (validate_error != Error::Ok) { setError(validate_error); return last_error_; }
    validate_error = validateDirectDeviceList(dword_devices, dword_count, plc_profile_);
    if (validate_error != Error::Ok) { setError(validate_error); return last_error_; }
    for (size_t i = 0; i < word_count; ++i) {
        if (isLongCounterContactDevice(word_devices[i].code())) {
            setError(Error::UnsupportedDevice);
            return last_error_;
        }
    }
    for (size_t i = 0; i < dword_count; ++i) {
        if (isLongCounterContactDevice(dword_devices[i].code())) {
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Monitor);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomReadLikeCounts(word_count, dword_count, compatibility_mode_, true, plc_profile_, ProfileLimitKey::MonitorRegisterWord) != Error::Ok) {
        setError(Error::InvalidArgument); return last_error_;
    }
    Error validate_error = validateExtMonitorDevices(word_devices, word_count, dword_devices, dword_count);
    if (validate_error != Error::Ok) {
        setError(validate_error);
        return last_error_;
    }
    for (size_t i = 0; i < word_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(word_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }
    for (size_t i = 0; i < dword_count; ++i) {
        guard_error = ensureExtProfileFeatureAllowed(dword_devices[i]);
        if (guard_error != Error::Ok) return guard_error;
    }

    if (tx_capacity_ < 2U) { setError(Error::BufferTooSmall); return last_error_; }
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
    Error guard_error = ensureProfileFeatureAllowed(ProfileFeatureKey::Monitor);
    if (guard_error != Error::Ok) return guard_error;
    if (validateRandomReadLikeCounts(
            word_count,
            dword_count,
            compatibility_mode_,
            false,
            plc_profile_,
            ProfileLimitKey::MonitorRegisterWord) != Error::Ok) {
        setError(Error::InvalidArgument);
        return last_error_;
    }
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
