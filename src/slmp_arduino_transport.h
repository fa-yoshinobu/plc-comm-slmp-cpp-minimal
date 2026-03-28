/**
 * @file slmp_arduino_transport.h
 * @brief Arduino-specific transport implementations for SLMP.
 * 
 * Provides ITransport adapters for Arduino's standard networking classes.
 * Includes support for both Stream-based (TCP) and Packet-based (UDP) communication.
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
 * @ingroup SLMP_Transport
 * @brief Transport adapter for Arduino 'Client' objects (TCP).
 * 
 * Wraps classes like `WiFiClient`, `EthernetClient`, or `GSMClient`.
 * Implements reliable stream-based communication for SLMP 3E/4E frames.
 */
class ArduinoClientTransport : public ITransport {
  public:
    /** 
     * @brief Wrap an existing Arduino Client. 
     * @param client Reference to an Arduino Client object (e.g., WiFiClient).
     */
    explicit ArduinoClientTransport(::Client& client) : client_(client) {}

    /** @brief Connect to PLC via TCP. */
    bool connect(const char* host, uint16_t port) override {
        return client_.connect(host, port) == 1;
    }

    /** @brief Stop the client and close connection. */
    void close() override {
        client_.stop();
    }

    /** @brief Check if the TCP socket is connected. */
    bool connected() const override {
        return client_.connected() != 0;
    }

    /** @brief Block until all data is written to the stream. */
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

    /** @brief Block until exact length is read from the stream or timeout occurs. */
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

    /** @brief Non-blocking write to the stream. */
    size_t write(const uint8_t* data, size_t length) override {
        return client_.write(data, length);
    }

    /** @brief Non-blocking read from the stream. */
    size_t read(uint8_t* data, size_t length) override {
        return client_.read(data, length);
    }

    /** @brief Check number of bytes available in the stream. */
    size_t available() override {
        return client_.available();
    }

  private:
    ::Client& client_;
};

#if SLMP_ENABLE_UDP_TRANSPORT

/**
 * @class ArduinoUdpTransport
 * @ingroup SLMP_Transport
 * @brief Transport adapter for Arduino 'UDP' objects.
 * 
 * Wraps classes like `WiFiUDP` or `EthernetUDP`.
 * Manages remote endpoint (host/port) and packet framing for SLMP.
 * 
 * @note UDP is connectionless; "connected" state in this class simply means 
 *       that `begin()` has been called and remote endpoint is known.
 */
class ArduinoUdpTransport : public ITransport {
  public:
    /**
     * @brief Wrap an existing Arduino UDP object.
     * @param udp Reference to UDP object.
     * @param local_port Local port to bind to (0 = use same as remote port).
     */
    explicit ArduinoUdpTransport(::UDP& udp, uint16_t local_port = 0)
        : udp_(udp), local_port_(local_port) {}

    /** @brief Set remote host and begin listening on local port. */
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

    /** @brief Stop listening and clear remote endpoint. */
    void close() override {
        udp_.stop();
        connected_ = false;
        packet_available_ = 0;
    }

    /** @brief True if the transport is initialized. */
    bool connected() const override {
        return connected_;
    }

    /** @brief Send a single UDP packet containing the frame. */
    bool writeAll(const uint8_t* data, size_t length) override {
        return sendPacket(data, length);
    }

    /** @brief Block until a UDP packet of exact length is received or timeout. */
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

    /** @brief Non-blocking packet send. Returns @p length on success. */
    size_t write(const uint8_t* data, size_t length) override {
        return sendPacket(data, length) ? length : 0U;
    }

    /** @brief Non-blocking read from the current received packet. */
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

    /** @brief Check if a new packet has arrived and return its size. */
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
