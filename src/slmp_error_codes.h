/**
 * @file slmp_error_codes.h
 * @brief Optional SLMP end-code text helpers.
 *
 * Link slmp_error_codes.cpp for code-derived end-code labels and category
 * helpers. Link
 * slmp_error_messages.cpp plus the language source files you need when
 * user-facing messages are required.
 */

#ifndef SLMP_ERROR_CODES_H
#define SLMP_ERROR_CODES_H

#include <stdint.h>

namespace slmp {

/**
 * @enum EndCodeLanguage
 * @brief Language selector for optional end-code messages.
 */
enum class EndCodeLanguage : uint8_t {
    English = 0,
    Japanese = 1,
};

/** @brief Get a compact code-derived label for an SLMP end code. */
const char* endCodeString(uint16_t end_code);

/** @brief Return true when the SLMP end code is related to remote password protection. */
bool isRemotePasswordEndCode(uint16_t end_code);

/**
 * @brief Get a user-facing SLMP end-code message.
 *
 * Messages are based on the error detail/cause text. Remote-password retry
 * delay codes include the wait time when that is the useful distinction.
 * Returns nullptr when the code is not in the linked message table.
 */
const char* endCodeMessage(uint16_t end_code, EndCodeLanguage language = EndCodeLanguage::English);

/** @brief Get the English message for an SLMP end code, or nullptr. */
const char* endCodeMessageEnglish(uint16_t end_code);

/** @brief Get the Japanese message for an SLMP end code, or nullptr. */
const char* endCodeMessageJapanese(uint16_t end_code);

}  // namespace slmp

#endif
