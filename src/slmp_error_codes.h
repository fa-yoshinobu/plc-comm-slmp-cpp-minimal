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

}  // namespace slmp

#endif
