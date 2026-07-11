#ifndef SLMP_TEST_ARDUINO_H
#define SLMP_TEST_ARDUINO_H

#include <stddef.h>
#include <stdint.h>

inline uint32_t millis() {
    static uint32_t now = 0U;
    return now++;
}

inline void delay(uint32_t) {}

#endif
