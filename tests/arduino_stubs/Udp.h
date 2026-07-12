#ifndef SLMP_TEST_UDP_H
#define SLMP_TEST_UDP_H

#include <stddef.h>
#include <stdint.h>

class UDP {
  public:
    virtual ~UDP() = default;
    virtual uint8_t begin(uint16_t port) = 0;
    virtual void stop() = 0;
    virtual int beginPacket(const char* host, uint16_t port) = 0;
    virtual int endPacket() = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual int parsePacket() = 0;
    virtual int available() = 0;
    virtual int read(uint8_t* data, size_t length) = 0;
};

#endif
