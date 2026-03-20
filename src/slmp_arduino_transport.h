/**
 * @file slmp_arduino_transport.h
 * @brief Arduino-specific transport implementations for SLMP.
 * 
 * Provides ITransport adapters for Arduino's Client (TCP) and UDP interfaces.
 */

#ifndef SLMP_ARDUINO_TRANSPORT_H
#define SLMP_ARDUINO_TRANSPORT_H

#include <Arduino.h>
#include <Client.h>
#ifndef SLMP_ENABLE_UDP_TRANSPORT
#define SLMP_ENABLE_UDP_TRANSPORT 1
#endif

#if SLMP_ENABLE_UDP_TRANSPORT
#include <Udp.h>
#endif

#include "slmp_minimal.h"

namespace slmp {

/**
 * @class ArduinoClientTransport
 * @brief Transport adapter for Arduino 'Client' objects (typically TCP).
 * 
 * Use this with EthernetClient, WiFiClient, or similar classes.
 */
class ArduinoClientTransport : public ITransport {
  public:
    /** @brief Wrap an existing Arduino Client. */
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

    size_t write(const uint8_t* data, size_t length) override {
        return client_.write(data, length);
    }

    size_t read(uint8_t* data, size_t length) override {
        return client_.read(data, length);
    }

    size_t available() override {
        return client_.available();
    }

  private:
    ::Client& client_;
};

#if SLMP_ENABLE_UDP_TRANSPORT

/**
 * @class ArduinoUdpTransport
 * @brief Transport adapter for Arduino 'UDP' objects.
 * 
 * Wraps EthernetUDP, WiFiUDP, etc. Note that SLMP over UDP is connectionless,
 * but this class manages the remote endpoint information.
 */
class ArduinoUdpTransport : public ITransport {
  public:
    /**
     * @brief Wrap an existing Arduino UDP object.
     * @param udp Reference to UDP object.
     * @param local_port Local port to bind to (0 = use remote port).
     */
    explicit ArduinoUdpTransport(::UDP& udp, uint16_t local_port = 0)
        : udp_(udp), local_port_(local_port) {}

    bool connect(const char* host, uint16_t port) override {
        if (host == nullptr || port == 0U) {
            connected_ = false;
            return false;
        }

        host_ = host;
        remote_port_ = port;
        const uint16_t bind_port = (local_port_ == 0U) ? port : local_port_;
        connected_ = (udp_.begin(bind_port) == 1);
        packet_available_ = 0;
        return connected_;
    }

    void close() override {
        udp_.stop();
        connected_ = false;
        packet_available_ = 0;
    }

    bool connected() const override {
        return connected_;
    }

    bool writeAll(const uint8_t* data, size_t length) override {
        return sendPacket(data, length);
    }

    bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) override {
        const uint32_t started_at = millis();
        size_t received = 0;
        while (received < length) {
            if (available() > 0U) {
                const size_t read_now = read(data + received, length - received);
                if (read_now == 0U) {
                    return false;
                }
                received += read_now;
                continue;
            }

            if (!connected_) {
                return false;
            }
            if (static_cast<uint32_t>(millis() - started_at) >= timeout_ms) {
                return false;
            }
            delay(1);
        }
        return true;
    }

    size_t write(const uint8_t* data, size_t length) override {
        return sendPacket(data, length) ? length : 0U;
    }

    size_t read(uint8_t* data, size_t length) override {
        if (available() == 0U) {
            return 0U;
        }
        const int read_now = udp_.read(data, length);
        if (read_now <= 0) {
            return 0U;
        }
        const size_t read_size = static_cast<size_t>(read_now);
        packet_available_ = (packet_available_ > read_size) ? (packet_available_ - read_size) : 0U;
        return read_size;
    }

    size_t available() override {
        if (!connected_) {
            return 0U;
        }
        if (packet_available_ == 0U) {
            const int packet_size = udp_.parsePacket();
            if (packet_size > 0) {
                packet_available_ = static_cast<size_t>(packet_size);
            }
        }
        return packet_available_;
    }

  private:
    bool sendPacket(const uint8_t* data, size_t length) {
        if (!connected_ || data == nullptr || length == 0U) {
            return false;
        }
        if (udp_.beginPacket(host_.c_str(), remote_port_) != 1) {
            connected_ = false;
            return false;
        }
        const size_t written = udp_.write(data, length);
        if (written != length || udp_.endPacket() != 1) {
            connected_ = false;
            return false;
        }
        return true;
    }

    ::UDP& udp_;
    String host_;
    uint16_t remote_port_ = 0;
    uint16_t local_port_ = 0;
    size_t packet_available_ = 0;
    bool connected_ = false;
};

#endif

}  // namespace slmp

#endif
