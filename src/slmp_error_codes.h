/**
 * @file slmp_error_codes.h
 * @brief Optional SLMP end-code key helpers.
 *
 * Link slmp_error_codes.cpp for code-derived end-code keys and category
 * helpers. User-facing message text is intentionally not embedded in this
 * library; resolve endCodeString() in an application-owned catalog when text
 * is required.
 */

#ifndef SLMP_ERROR_CODES_H
#define SLMP_ERROR_CODES_H

#include <stddef.h>
#include <stdint.h>

namespace slmp {

/**
 * @enum EndCodeLanguage
 * @brief Language selector retained for optional external end-code catalogs.
 */
enum class EndCodeLanguage : uint8_t {
    English = 0,
    Japanese = 1,
};

/**
 * @brief Format the stable code-derived key for an SLMP end code.
 *
 * The output buffer must have room for at least 19 bytes, including the null
 * terminator for a string such as "slmp_end_code_c810". Returns buffer on
 * success, or nullptr when the buffer is missing or too small.
 */
char* formatEndCodeString(uint16_t end_code, char* buffer, size_t buffer_size);

/**
 * @brief Get the stable code-derived key for an SLMP end code.
 *
 * The returned pointer refers to an internal rotating buffer. Copy the string
 * or use formatEndCodeString() when the value must be retained long-term.
 */
const char* endCodeString(uint16_t end_code);

/** @brief Return true when the SLMP end code is related to remote password protection. */
bool isRemotePasswordEndCode(uint16_t end_code);

/**
 * @brief Get a user-facing SLMP end-code message.
 *
 * Localized message text is not embedded in this library; resolve
 * endCodeString(end_code) in an application-owned catalog. This function is
 * retained as a compatibility hook and returns nullptr.
 */
const char* endCodeMessage(uint16_t end_code, EndCodeLanguage language = EndCodeLanguage::English);

/** @brief Compatibility hook for English message lookup; returns nullptr. */
const char* endCodeMessageEnglish(uint16_t end_code);

/** @brief Compatibility hook for Japanese message lookup; returns nullptr. */
const char* endCodeMessageJapanese(uint16_t end_code);

}  // namespace slmp

#endif
