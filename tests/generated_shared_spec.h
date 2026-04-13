#ifndef SLMP_TESTS_GENERATED_SHARED_SPEC_H
#define SLMP_TESTS_GENERATED_SHARED_SPEC_H

#include <stddef.h>
#include <stdint.h>

namespace shared_spec {

namespace device_vectors {
struct DeviceVector {
    const char* id;
    slmp::DeviceCode code;
    uint32_t number;
    slmp::CompatibilityMode mode;
    bool bit_access;
    const uint8_t* expected;
    size_t expected_size;
};

static constexpr uint8_t kDeviceVectorBytes0[] = {
    0x00, 0x00, 0x00, 0x00, 0xA8, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes1[] = {
    0x00, 0x00, 0x00, 0xA8,
};

static constexpr uint8_t kDeviceVectorBytes2[] = {
    0x64, 0x00, 0x00, 0x00, 0xA8, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes3[] = {
    0x64, 0x00, 0x00, 0xA8,
};

static constexpr uint8_t kDeviceVectorBytes4[] = {
    0xF4, 0x01, 0x00, 0x00, 0x90, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes5[] = {
    0xF4, 0x01, 0x00, 0x90,
};

static constexpr uint8_t kDeviceVectorBytes6[] = {
    0x10, 0x00, 0x00, 0x00, 0x9C, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes7[] = {
    0x10, 0x00, 0x00, 0x9C,
};

static constexpr uint8_t kDeviceVectorBytes8[] = {
    0xA0, 0x01, 0x00, 0x00, 0xB4, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes9[] = {
    0xA0, 0x01, 0x00, 0xB4,
};

static constexpr uint8_t kDeviceVectorBytes10[] = {
    0x20, 0x00, 0x00, 0x00, 0x9D, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes11[] = {
    0x20, 0x00, 0x00, 0x9D,
};

static constexpr uint8_t kDeviceVectorBytes12[] = {
    0x10, 0x00, 0x00, 0x00, 0xA0, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes13[] = {
    0x10, 0x00, 0x00, 0xA0,
};

static constexpr uint8_t kDeviceVectorBytes14[] = {
    0x05, 0x00, 0x00, 0x00, 0xC2, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes15[] = {
    0x05, 0x00, 0x00, 0xC2,
};

static constexpr uint8_t kDeviceVectorBytes16[] = {
    0x00, 0x00, 0x00, 0x00, 0xA9, 0x00,
};

static constexpr uint8_t kDeviceVectorBytes17[] = {
    0x00, 0x00, 0x00, 0xA9,
};

static constexpr DeviceVector kCases[] = {
    {"d0_iqr", slmp::DeviceCode::D, 0U, slmp::CompatibilityMode::iQR, false, kDeviceVectorBytes0, sizeof(kDeviceVectorBytes0)},
    {"d0_legacy", slmp::DeviceCode::D, 0U, slmp::CompatibilityMode::Legacy, false, kDeviceVectorBytes1, sizeof(kDeviceVectorBytes1)},
    {"d100_iqr", slmp::DeviceCode::D, 100U, slmp::CompatibilityMode::iQR, false, kDeviceVectorBytes2, sizeof(kDeviceVectorBytes2)},
    {"d100_legacy", slmp::DeviceCode::D, 100U, slmp::CompatibilityMode::Legacy, false, kDeviceVectorBytes3, sizeof(kDeviceVectorBytes3)},
    {"m500_iqr", slmp::DeviceCode::M, 500U, slmp::CompatibilityMode::iQR, true, kDeviceVectorBytes4, sizeof(kDeviceVectorBytes4)},
    {"m500_legacy", slmp::DeviceCode::M, 500U, slmp::CompatibilityMode::Legacy, true, kDeviceVectorBytes5, sizeof(kDeviceVectorBytes5)},
    {"x10_iqr", slmp::DeviceCode::X, 16U, slmp::CompatibilityMode::iQR, true, kDeviceVectorBytes6, sizeof(kDeviceVectorBytes6)},
    {"x10_legacy", slmp::DeviceCode::X, 16U, slmp::CompatibilityMode::Legacy, true, kDeviceVectorBytes7, sizeof(kDeviceVectorBytes7)},
    {"w1a0_iqr", slmp::DeviceCode::W, 416U, slmp::CompatibilityMode::iQR, false, kDeviceVectorBytes8, sizeof(kDeviceVectorBytes8)},
    {"w1a0_legacy", slmp::DeviceCode::W, 416U, slmp::CompatibilityMode::Legacy, false, kDeviceVectorBytes9, sizeof(kDeviceVectorBytes9)},
    {"y20_iqr", slmp::DeviceCode::Y, 32U, slmp::CompatibilityMode::iQR, true, kDeviceVectorBytes10, sizeof(kDeviceVectorBytes10)},
    {"y20_legacy", slmp::DeviceCode::Y, 32U, slmp::CompatibilityMode::Legacy, true, kDeviceVectorBytes11, sizeof(kDeviceVectorBytes11)},
    {"b10_iqr", slmp::DeviceCode::B, 16U, slmp::CompatibilityMode::iQR, true, kDeviceVectorBytes12, sizeof(kDeviceVectorBytes12)},
    {"b10_legacy", slmp::DeviceCode::B, 16U, slmp::CompatibilityMode::Legacy, true, kDeviceVectorBytes13, sizeof(kDeviceVectorBytes13)},
    {"tn5_iqr", slmp::DeviceCode::TN, 5U, slmp::CompatibilityMode::iQR, false, kDeviceVectorBytes14, sizeof(kDeviceVectorBytes14)},
    {"tn5_legacy", slmp::DeviceCode::TN, 5U, slmp::CompatibilityMode::Legacy, false, kDeviceVectorBytes15, sizeof(kDeviceVectorBytes15)},
    {"sd0_iqr", slmp::DeviceCode::SD, 0U, slmp::CompatibilityMode::iQR, false, kDeviceVectorBytes16, sizeof(kDeviceVectorBytes16)},
    {"sd0_legacy", slmp::DeviceCode::SD, 0U, slmp::CompatibilityMode::Legacy, false, kDeviceVectorBytes17, sizeof(kDeviceVectorBytes17)},
};

}  // namespace device_vectors

namespace normalize_cases {
struct NormalizeCase {
    const char* id;
    const char* input;
    const char* expected;
};

static constexpr NormalizeCase kCases[] = {
    {"float_suffix", " d200:f ", "D200:F"},
    {"hex_device", " x1a ", "X1A"},
    {"bit_in_word_hex", "d50.a", "D50.A"},
    {"explicit_dword", "rd100:d", "RD100:D"},
};

}  // namespace normalize_cases

namespace cpp_parse_cases {
struct ParseCase {
    const char* id;
    const char* input;
    slmp::Error expected_error;
    slmp::DeviceCode code;
    uint32_t number;
    slmp::highlevel::ValueType value_type;
    bool explicit_type;
    int bit_index;
    bool has_value_expectation;
};

static constexpr ParseCase kCases[] = {
    {"plain_word", "D100", slmp::Error::Ok, slmp::DeviceCode::D, 100U, slmp::highlevel::ValueType::U16, false, -1, true},
    {"plain_bit_device", "M1000", slmp::Error::Ok, slmp::DeviceCode::M, 1000U, slmp::highlevel::ValueType::Bit, false, -1, true},
    {"float_suffix", "D200:F", slmp::Error::Ok, slmp::DeviceCode::D, 200U, slmp::highlevel::ValueType::Float32, true, -1, true},
    {"bit_in_word_hex", "D50.A", slmp::Error::Ok, slmp::DeviceCode::D, 50U, slmp::highlevel::ValueType::Bit, false, 10, true},
    {"bit_suffix_on_bit_device", "M1000.0", slmp::Error::InvalidArgument, slmp::DeviceCode::D, 0U, slmp::highlevel::ValueType::U16, false, -1, false},
};

}  // namespace cpp_parse_cases

namespace frame_vectors {
struct FrameVector {
    const char* id;
    const char* operation;
    const uint8_t* request;
    size_t request_size;
    const uint8_t* response_data;
    size_t response_data_size;
};

static constexpr uint8_t kFrameRequest0[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x06,
    0x00, 0x10, 0x00, 0x01, 0x01, 0x00, 0x00,
};

static constexpr uint8_t kFrameResponseData0[] = {
    0x51, 0x30, 0x33, 0x55, 0x44, 0x56, 0x43, 0x50, 0x55, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x34, 0x12,
};

static constexpr uint8_t kFrameRequest1[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E,
    0x00, 0x10, 0x00, 0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8,
    0x00, 0x02, 0x00,
};

static constexpr uint8_t kFrameResponseData1[] = {
    0x34, 0x12, 0x78, 0x56,
};

static constexpr uint8_t kFrameRequest2[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0F,
    0x00, 0x10, 0x00, 0x01, 0x14, 0x03, 0x00, 0x65, 0x00, 0x00, 0x00, 0x90,
    0x00, 0x01, 0x00, 0x10,
};

static constexpr uint8_t kFrameResponseData2[] = {
};

static constexpr uint8_t kFrameRequest3[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x1A,
    0x00, 0x10, 0x00, 0x03, 0x04, 0x02, 0x00, 0x02, 0x01, 0x64, 0x00, 0x00,
    0x00, 0xA8, 0x00, 0x65, 0x00, 0x00, 0x00, 0xA8, 0x00, 0xC8, 0x00, 0x00,
    0x00, 0xA8, 0x00,
};

static constexpr uint8_t kFrameResponseData3[] = {
    0x11, 0x11, 0x22, 0x22, 0x78, 0x56, 0x34, 0x12,
};

static constexpr uint8_t kFrameRequest4[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x17,
    0x00, 0x10, 0x00, 0x02, 0x14, 0x03, 0x00, 0x02, 0x64, 0x00, 0x00, 0x00,
    0x90, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x9D, 0x00, 0x00, 0x00,
};

static constexpr uint8_t kFrameResponseData4[] = {
};

static constexpr uint8_t kFrameRequest5[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x18,
    0x00, 0x10, 0x00, 0x06, 0x04, 0x02, 0x00, 0x01, 0x01, 0x2C, 0x01, 0x00,
    0x00, 0xA8, 0x00, 0x02, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x90, 0x00, 0x01,
    0x00,
};

static constexpr uint8_t kFrameResponseData5[] = {
    0x34, 0x12, 0x78, 0x56, 0x05, 0x00,
};

static constexpr uint8_t kFrameRequest6[] = {
    0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0F,
    0x00, 0x10, 0x00, 0x30, 0x16, 0x00, 0x00, 0x07, 0x00, 0x73, 0x65, 0x63,
    0x72, 0x65, 0x74, 0x31,
};

static constexpr uint8_t kFrameResponseData6[] = {
};

static constexpr FrameVector kCases[] = {
    {"read_type_name", "read_type_name", kFrameRequest0, sizeof(kFrameRequest0), kFrameResponseData0, sizeof(kFrameResponseData0)},
    {"read_words_d100_2", "read_words", kFrameRequest1, sizeof(kFrameRequest1), kFrameResponseData1, sizeof(kFrameResponseData1)},
    {"write_bits_m101_true", "write_bits", kFrameRequest2, sizeof(kFrameRequest2), kFrameResponseData2, sizeof(kFrameResponseData2)},
    {"read_random_d100_d101_d200", "read_random", kFrameRequest3, sizeof(kFrameRequest3), kFrameResponseData3, sizeof(kFrameResponseData3)},
    {"write_random_bits_m100_y20", "write_random_bits", kFrameRequest4, sizeof(kFrameRequest4), kFrameResponseData4, sizeof(kFrameResponseData4)},
    {"read_block_d300_2_m200_1", "read_block", kFrameRequest5, sizeof(kFrameRequest5), kFrameResponseData5, sizeof(kFrameResponseData5)},
    {"remote_password_unlock_secret1", "remote_password_unlock", kFrameRequest6, sizeof(kFrameRequest6), kFrameResponseData6, sizeof(kFrameResponseData6)},
};

}  // namespace frame_vectors

}  // namespace shared_spec

#endif
