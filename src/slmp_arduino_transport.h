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
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFiClient.h>
#include <lwip/sockets.h>
#endif
#ifndef SLMP_ENABLE_UDP_TRANSPORT
#define SLMP_ENABLE_UDP_TRANSPORT 1
#endif

#if SLMP_ENABLE_UDP_TRANSPORT
#include <Udp.h>
#endif

#include "slmp_minimal.h"

namespace slmp {

using TcpKeepAliveConfigurator = bool (*)(::Client& client, uint32_t idle_seconds);

#if defined(ARDUINO_ARCH_ESP32)
/** @brief Configure the approved 30-second TCP keepalive idle on ESP32 WiFiClient. */
inline bool configureEsp32WifiClientKeepAlive(::Client& base_client, uint32_t idle_seconds) {
    ::WiFiClient& client = static_cast<::WiFiClient&>(base_client);
    const int enabled = 1;
    const int idle = static_cast<int>(idle_seconds);
    return client.setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)) == 0 &&
           client.setSocketOption(IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) == 0;
}
#endif

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
     * @param keepalive_configurator Platform adapter that applies the fixed
     *        30-second TCP keepalive idle after connection.
     */
    ArduinoClientTransport(::Client& client, TcpKeepAliveConfigurator keepalive_configurator)
        : client_(client), keepalive_configurator_(keepalive_configurator) {}

    /** @brief Connect to PLC via TCP. */
    bool connect(const char* host, uint16_t port) override {
        if (client_.connect(host, port) != 1) {
            return false;
        }
        if (keepalive_configurator_ == nullptr ||
            !keepalive_configurator_(client_, kKeepAliveIdleSeconds)) {
            client_.stop();
            return false;
        }
        return true;
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
    static constexpr uint32_t kKeepAliveIdleSeconds = 30U;
    ::Client& client_;
    TcpKeepAliveConfigurator keepalive_configurator_;
};

#if SLMP_ENABLE_UDP_TRANSPORT

/**
 * @class ArduinoUdpTransport
 * @ingroup SLMP_Transport
 * @brief Transport adapter for Arduino 'UDP' objects.
 * 
 * Wraps classes like `WiFiUDP` or `EthernetUDP`.
 * Manages remote endpoint (host/port) and packet framing for SLMP.
 * Only datagrams whose source IP address and port match the configured numeric
 * remote endpoint are exposed to the SLMP decoder; other datagrams are drained.
 * 
 * @note UDP is connectionless; "connected" state in this class simply means 
 *       that `begin()` has been called and remote endpoint is known.
 */
class ArduinoUdpTransport : public ITransport {
  public:
    /**
     * @brief Wrap an existing Arduino UDP object.
     * @param udp Reference to UDP object.
     * @param local_port Local port to bind to (0 = request an ephemeral port).
     */
    explicit ArduinoUdpTransport(::UDP& udp, uint16_t local_port = 0)
        : udp_(udp), local_port_(local_port) {}

    /** @brief Set a numeric remote IP address and begin listening on the local port. */
    bool connect(const char* host, uint16_t port) override {
        if (host == nullptr || port == 0U) {
            connected_ = false;
            return false;
        }

        if (!remote_ip_.fromString(host)) {
            connected_ = false;
            return false;
        }
        host_ = host;
        remote_port_ = port;
        connected_ = (udp_.begin(local_port_) == 1);
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
        while (packet_available_ == 0U) {
            const int packet_size = udp_.parsePacket();
            if (packet_size <= 0) return 0U;
            const size_t packet_bytes = static_cast<size_t>(packet_size);
            if (udp_.remoteIP() != remote_ip_ || udp_.remotePort() != remote_port_) {
                uint8_t discard[32];
                size_t remaining = packet_bytes;
                while (remaining > 0U) {
                    const size_t chunk = remaining < sizeof(discard) ? remaining : sizeof(discard);
                    const int discarded = udp_.read(discard, chunk);
                    if (discarded <= 0) return 0U;
                    remaining -= static_cast<size_t>(discarded);
                }
                continue;
            }
            packet_available_ = packet_bytes;
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
    IPAddress remote_ip_;
    uint16_t remote_port_ = 0;
    uint16_t local_port_ = 0;
    size_t packet_available_ = 0;
    bool connected_ = false;
};

#endif

}  // namespace slmp

#endif
