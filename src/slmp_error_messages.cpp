/**
 * @file slmp_error_messages.cpp
 * @brief Optional SLMP end-code message language dispatcher.
 */

#include "slmp_error_codes.h"

namespace slmp {

const char* endCodeMessage(uint16_t end_code, EndCodeLanguage language) {
    switch (language) {
        case EndCodeLanguage::English:
            return endCodeMessageEnglish(end_code);
        case EndCodeLanguage::Japanese:
            return endCodeMessageJapanese(end_code);
        default:
            return nullptr;
    }
}

}  // namespace slmp
