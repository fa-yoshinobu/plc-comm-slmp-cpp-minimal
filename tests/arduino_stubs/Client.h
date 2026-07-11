#ifndef SLMP_TEST_CLIENT_H
#define SLMP_TEST_CLIENT_H

#include <stddef.h>
#include <stdint.h>

class Client {
  public:
    virtual ~Client() = default;
    virtual int connect(const char* host, uint16_t port) = 0;
    virtual void stop() = 0;
    virtual uint8_t connected() = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual int read(uint8_t* data, size_t length) = 0;
    virtual int available() = 0;
};

#endif
