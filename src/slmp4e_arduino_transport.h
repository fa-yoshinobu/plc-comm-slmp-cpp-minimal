#ifndef SLMP4E_ARDUINO_TRANSPORT_H
#define SLMP4E_ARDUINO_TRANSPORT_H

#include <Arduino.h>
#include <Client.h>

#include "slmp4e_minimal.h"

namespace slmp4e {

class ArduinoClientTransport : public ITransport {
  public:
    explicit ArduinoClientTransport(::Client& client) : client_(client) {}

    bool connect(const char* host, uint16_t port) override {
        return client_.connect(host, port) == 1;
    }

    void close() override {
        client_.stop();
    }

    bool connected() const override {
        return client_.connected() != 0;
    }

    bool writeAll(const uint8_t* data, size_t length) override {
        size_t written_total = 0;
        while (written_total < length) {
            size_t written_now = client_.write(data + written_total, length - written_total);
            if (written_now == 0) {
                return false;
            }
            written_total += written_now;
        }
        return true;
    }

    bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) override {
        uint32_t started_at = millis();
        size_t received = 0;
        while (received < length) {
            int available_now = client_.available();
            if (available_now > 0) {
                int read_now = client_.read(data + received, length - received);
                if (read_now <= 0) {
                    return false;
                }
                received += static_cast<size_t>(read_now);
                continue;
            }

            if (!connected()) {
                return false;
            }
            if (static_cast<uint32_t>(millis() - started_at) >= timeout_ms) {
                return false;
            }
            delay(1);
        }
        return true;
    }

  private:
    ::Client& client_;
};

}  // namespace slmp4e

#endif
