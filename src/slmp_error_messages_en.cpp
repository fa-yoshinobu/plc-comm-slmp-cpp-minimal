/**
 * @file slmp_error_messages_en.cpp
 * @brief Optional English SLMP end-code messages.
 */

#include "slmp_error_codes.h"

namespace slmp {

const char* endCodeMessageEnglish(uint16_t end_code) {
    switch (end_code) {
#define SLMP_END_CODE_MESSAGE(code, message) \
        case code:                           \
            return message;
#include "lang/slmp_end_code_messages_en.def"
#undef SLMP_END_CODE_MESSAGE
        default:
            return nullptr;
    }
}

}  // namespace slmp
