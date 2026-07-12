#ifndef SLMP_TEST_ARDUINO_H
#define SLMP_TEST_ARDUINO_H

#include <stddef.h>
#include <stdint.h>
#include <string>

class String {
  public:
    String() = default;
    String(const char* value) : value_(value == nullptr ? "" : value) {}

    String& operator=(const char* value) {
        value_ = value == nullptr ? "" : value;
        return *this;
    }

    const char* c_str() const { return value_.c_str(); }

  private:
    std::string value_;
};

class IPAddress {
  public:
    IPAddress() = default;
    explicit IPAddress(const char* value) { fromString(value); }

    bool fromString(const char* value) {
        value_ = value == nullptr ? "" : value;
        return !value_.empty();
    }

    bool operator==(const IPAddress& other) const { return value_ == other.value_; }
    bool operator!=(const IPAddress& other) const { return !(*this == other); }

  private:
    std::string value_;
};

inline uint32_t millis() {
    static uint32_t now = 0U;
    return now++;
}

inline void delay(uint32_t) {}

#endif
