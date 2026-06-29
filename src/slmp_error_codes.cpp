/**
 * @file slmp_error_codes.cpp
 * @brief SLMP end-code symbolic keys and categories.
 */

#include "slmp_error_codes.h"

namespace {

constexpr size_t kEndCodeStringBufferSize = sizeof("slmp_end_code_0000");
constexpr size_t kEndCodePrefixLength = 14;
constexpr size_t kEndCodeRingSize = 8;

void write_hex4(uint16_t value, char* out) {
    static constexpr char kHex[] = "0123456789abcdef";
    out[0] = kHex[(value >> 12) & 0x0F];
    out[1] = kHex[(value >> 8) & 0x0F];
    out[2] = kHex[(value >> 4) & 0x0F];
    out[3] = kHex[value & 0x0F];
}

void write_end_code_string(uint16_t end_code, char* out) {
    static constexpr char kPrefix[] = "slmp_end_code_";
    for (size_t i = 0; i < kEndCodePrefixLength; ++i) {
        out[i] = kPrefix[i];
    }
    write_hex4(end_code, &out[kEndCodePrefixLength]);
    out[kEndCodeStringBufferSize - 1] = '\0';
}

}  // namespace

namespace slmp {

char* formatEndCodeString(uint16_t end_code, char* buffer, size_t buffer_size) {
    if (buffer == nullptr || buffer_size < kEndCodeStringBufferSize) {
        return nullptr;
    }
    write_end_code_string(end_code, buffer);
    return buffer;
}

const char* endCodeString(uint16_t end_code) {
    static thread_local char labels[kEndCodeRingSize][kEndCodeStringBufferSize] = {};
    static thread_local size_t next_label = 0;
    char* label = labels[next_label];
    next_label = (next_label + 1) % kEndCodeRingSize;
    write_end_code_string(end_code, label);
    return label;
}

bool isRemotePasswordEndCode(uint16_t end_code) {
    switch (end_code) {
        case 0xC200:
        case 0xC201:
        case 0xC202:
        case 0xC203:
        case 0xC204:
        case 0xC205:
        case 0xC810:
        case 0xC811:
        case 0xC812:
        case 0xC813:
        case 0xC814:
        case 0xC815:
        case 0xC816:
            return true;
        default:
            return false;
    }
}

const char* endCodeMessage(uint16_t end_code, EndCodeLanguage language) {
    (void)end_code;
    (void)language;
    return nullptr;
}

const char* endCodeMessageEnglish(uint16_t end_code) {
    (void)end_code;
    return nullptr;
}

const char* endCodeMessageJapanese(uint16_t end_code) {
    (void)end_code;
    return nullptr;
}

}  // namespace slmp
