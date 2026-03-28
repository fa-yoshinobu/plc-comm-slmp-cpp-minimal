#include <assert.h>
#include <stdint.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "slmp_minimal.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;

struct SocketRuntimeInit {
    SocketRuntimeInit() {
        WSADATA data = {};
        const int result = WSAStartup(MAKEWORD(2, 2), &data);
        assert(result == 0);
    }

    ~SocketRuntimeInit() {
        WSACleanup();
    }
};

void closeSocket(SocketHandle handle) {
    if (handle != kInvalidSocket) {
        closesocket(handle);
    }
}
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;

void closeSocket(SocketHandle handle) {
    if (handle != kInvalidSocket) {
        close(handle);
    }
}
#endif

uint16_t parsePort(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return 11025;
    }
    const unsigned long value = std::strtoul(text, nullptr, 10);
    assert(value > 0UL && value <= 65535UL);
    return static_cast<uint16_t>(value);
}

const char* getenvOrDefault(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? value : fallback;
}

void require(bool value) {
    assert(value);
}

class SocketTransport : public slmp::ITransport {
  public:
    SocketTransport() = default;

    ~SocketTransport() override {
        close();
    }

    bool connect(const char* host, uint16_t port) override {
        close();
        if (host == nullptr || host[0] == '\0' || port == 0U) {
            return false;
        }

#ifdef _WIN32
        static SocketRuntimeInit runtime_init;
#endif

        char port_text[8] = {};
        std::snprintf(port_text, sizeof(port_text), "%u", static_cast<unsigned>(port));

        addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        if (getaddrinfo(host, port_text, &hints, &result) != 0) {
            return false;
        }

        for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
            SocketHandle handle = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (handle == kInvalidSocket) {
                continue;
            }
            if (::connect(handle, it->ai_addr, static_cast<int>(it->ai_addrlen)) == 0) {
                socket_ = handle;
                connected_ = true;

#ifdef _WIN32
                u_long mode = 1;
                ioctlsocket(socket_, FIONBIO, &mode);
#else
                int flags = fcntl(socket_, F_GETFL, 0);
                fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
#endif

                freeaddrinfo(result);
                return true;
            }
            closeSocket(handle);
        }

        freeaddrinfo(result);
        return false;
    }

    void close() override {
        closeSocket(socket_);
        socket_ = kInvalidSocket;
        connected_ = false;
    }

    bool connected() const override {
        return connected_;
    }

    bool writeAll(const uint8_t* data, size_t length) override {
        if (!connected_ || socket_ == kInvalidSocket || data == nullptr) {
            return false;
        }

        size_t offset = 0;
        while (offset < length) {
            const size_t remaining = length - offset;
#ifdef _WIN32
            const int sent = ::send(socket_, reinterpret_cast<const char*>(data + offset), static_cast<int>(remaining), 0);
#else
            const ssize_t sent = ::send(socket_, data + offset, remaining, 0);
#endif
            if (sent <= 0) {
                close();
                return false;
            }
            offset += static_cast<size_t>(sent);
        }
        return true;
    }

    bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) override {
        if (!connected_ || socket_ == kInvalidSocket || data == nullptr) {
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        size_t offset = 0;
        while (offset < length) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return false;
            }
            const uint32_t wait_ms = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count()
            );
            if (!waitReadable(wait_ms)) {
                return false;
            }
#ifdef _WIN32
            const int received = ::recv(socket_, reinterpret_cast<char*>(data + offset), static_cast<int>(length - offset), 0);
#else
            const ssize_t received = ::recv(socket_, data + offset, length - offset, 0);
#endif
            if (received <= 0) {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
#endif
                close();
                return false;
            }
            offset += static_cast<size_t>(received);
        }
        return true;
    }

    size_t write(const uint8_t* data, size_t length) override {
        if (!connected_ || socket_ == kInvalidSocket || data == nullptr) {
            return 0;
        }
#ifdef _WIN32
        const int sent = ::send(socket_, reinterpret_cast<const char*>(data), static_cast<int>(length), 0);
#else
        const ssize_t sent = ::send(socket_, data, length, 0);
#endif
        if (sent <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
            close();
            return 0;
        }
        return static_cast<size_t>(sent);
    }

    size_t read(uint8_t* data, size_t length) override {
        if (!connected_ || socket_ == kInvalidSocket || data == nullptr) {
            return 0;
        }
#ifdef _WIN32
        const int received = ::recv(socket_, reinterpret_cast<char*>(data), static_cast<int>(length), 0);
#else
        const ssize_t received = ::recv(socket_, data, length, 0);
#endif
        if (received <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
            close();
            return 0;
        }
        return static_cast<size_t>(received);
    }

    size_t available() override {
        if (!connected_ || socket_ == kInvalidSocket) {
            return 0;
        }
        u_long arg = 0;
#ifdef _WIN32
        if (ioctlsocket(socket_, FIONREAD, &arg) == 0) {
            return static_cast<size_t>(arg);
        }
#else
        if (ioctl(socket_, FIONREAD, &arg) == 0) {
            return static_cast<size_t>(arg);
        }
#endif
        return 0;
    }

  private:
    bool waitReadable(uint32_t timeout_ms) const {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(socket_, &read_set);

        timeval timeout = {};
        timeout.tv_sec = static_cast<long>(timeout_ms / 1000U);
        timeout.tv_usec = static_cast<long>((timeout_ms % 1000U) * 1000U);

#ifdef _WIN32
        const int result = select(0, &read_set, nullptr, nullptr, &timeout);
#else
        const int result = select(socket_ + 1, &read_set, nullptr, nullptr, &timeout);
#endif
        return result > 0 && FD_ISSET(socket_, &read_set) != 0;
    }

    SocketHandle socket_ = kInvalidSocket;
    bool connected_ = false;
};

void testSocketRoundTrip(const char* host, uint16_t port, const char* password) {
    SocketTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setTimeoutMs(1000);

    assert(plc.connect(host, port));
    assert(plc.lastError() == slmp::Error::Ok);

    slmp::TypeNameInfo type_name = {};
    assert(plc.readTypeName(type_name) == slmp::Error::Ok);
    assert(std::string(type_name.model) == "Q03UDVCPU");
    assert(type_name.has_model_code);
    assert(type_name.model_code == 0x1234U);

    uint16_t locked_value = 0;
    assert(plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), locked_value) == slmp::Error::PlcError);
    assert(plc.lastEndCode() == 0x4013U);

    assert(plc.remotePasswordUnlock(password) == slmp::Error::Ok);

    uint16_t words[2] = {};
    assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::Ok);
    assert(words[0] == 1234U);
    assert(words[1] == 5678U);

    uint32_t dword = 0;
    assert(plc.readOneDWord(slmp::dev::D(slmp::dev::dec(200)), dword) == slmp::Error::Ok);
    assert(dword == 0x12345678UL);

    const slmp::DeviceAddress random_words[] = {
        slmp::dev::D(slmp::dev::dec(100)),
        slmp::dev::D(slmp::dev::dec(101)),
    };
    const slmp::DeviceAddress random_dwords[] = {
        slmp::dev::D(slmp::dev::dec(200)),
    };
    uint16_t random_word_values[2] = {};
    uint32_t random_dword_values[1] = {};
    assert(plc.readRandom(
               random_words,
               2,
               random_word_values,
               2,
               random_dwords,
               1,
               random_dword_values,
               1
           ) == slmp::Error::Ok);
    assert(random_word_values[0] == 1234U);
    assert(random_word_values[1] == 5678U);
    assert(random_dword_values[0] == 0x12345678UL);

    const uint16_t direct_write_values[] = {0x1111U, 0x2222U};
    assert(plc.writeWords(slmp::dev::D(slmp::dev::dec(120)), direct_write_values, 2) == slmp::Error::Ok);
    uint16_t direct_readback[2] = {};
    assert(plc.readWords(slmp::dev::D(slmp::dev::dec(120)), 2, direct_readback, 2) == slmp::Error::Ok);
    assert(direct_readback[0] == 0x1111U);
    assert(direct_readback[1] == 0x2222U);

    assert(plc.writeOneBit(slmp::dev::M(slmp::dev::dec(101)), true) == slmp::Error::Ok);
    bool direct_bits[4] = {};
    assert(plc.readBits(slmp::dev::M(slmp::dev::dec(100)), 4, direct_bits, 4) == slmp::Error::Ok);
    assert(direct_bits[0]);
    assert(direct_bits[1]);
    assert(direct_bits[2]);
    assert(!direct_bits[3]);

    const slmp::DeviceAddress random_bit_devices[] = {
        slmp::dev::M(slmp::dev::dec(102)),
        slmp::dev::Y(slmp::dev::hex(0x20)),
    };
    const bool random_bit_values[] = {false, true};
    assert(plc.writeRandomBits(random_bit_devices, random_bit_values, 2) == slmp::Error::Ok);
    bool y20 = false;
    assert(plc.readOneBit(slmp::dev::Y(slmp::dev::hex(0x20)), y20) == slmp::Error::Ok);
    assert(y20);

    const slmp::DeviceBlockRead word_blocks[] = {
        slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2),
    };
    const slmp::DeviceBlockRead bit_blocks[] = {
        slmp::dev::blockRead(slmp::dev::M(slmp::dev::dec(200)), 1),
    };
    uint16_t block_word_values[2] = {};
    uint16_t block_bit_values[1] = {};
    assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_word_values, 2, block_bit_values, 1) == slmp::Error::Ok);
    assert(block_word_values[0] == 0x1234U);
    assert(block_word_values[1] == 0x5678U);
    assert(block_bit_values[0] == 0x0005U);

    const uint16_t block_write_values[] = {0xAAAAU, 0x5555U};
    const slmp::DeviceBlockWrite block_word_writes[] = {
        slmp::dev::blockWrite(slmp::dev::D(slmp::dev::dec(400)), block_write_values, 2),
    };
    const uint16_t block_bit_write_values[] = {0x0005U};
    const slmp::DeviceBlockWrite block_bit_writes[] = {
        slmp::dev::blockWrite(slmp::dev::M(slmp::dev::dec(240)), block_bit_write_values, 1),
    };
    assert(plc.writeBlock(block_word_writes, 1, block_bit_writes, 1) == slmp::Error::Ok);

    uint16_t block_word_readback[2] = {};
    assert(plc.readWords(slmp::dev::D(slmp::dev::dec(400)), 2, block_word_readback, 2) == slmp::Error::Ok);
    assert(block_word_readback[0] == 0xAAAAU);
    assert(block_word_readback[1] == 0x5555U);

    bool block_bits_readback[4] = {};
    assert(plc.readBits(slmp::dev::M(slmp::dev::dec(240)), 4, block_bits_readback, 4) == slmp::Error::Ok);
    assert(block_bits_readback[0]);
    assert(!block_bits_readback[1]);
    assert(block_bits_readback[2]);
    assert(!block_bits_readback[3]);

    assert(plc.remotePasswordLock(password) == slmp::Error::Ok);
    uint16_t after_lock = 0;
    assert(plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), after_lock) == slmp::Error::PlcError);
    assert(plc.lastEndCode() == 0x4013U);

    plc.close();
}

void testInjectedPlcError(const char* host, uint16_t port) {
    SocketTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setTimeoutMs(1000);

    require(plc.connect(host, port));

    slmp::TypeNameInfo type_name = {};
    require(plc.readTypeName(type_name) == slmp::Error::Ok);

    uint16_t word = 0;
    require(plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), word) == slmp::Error::PlcError);
    require(plc.lastEndCode() == 0xC051U);
    require(std::string(slmp::endCodeString(plc.lastEndCode())) == "word_count_or_unit_rule_violation");
    plc.close();
}

void testDisconnectDuringResponse(const char* host, uint16_t port) {
    SocketTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setTimeoutMs(1000);

    require(plc.connect(host, port));

    slmp::TypeNameInfo type_name = {};
    require(plc.readTypeName(type_name) == slmp::Error::Ok);

    uint16_t word = 0;
    require(plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), word) == slmp::Error::TransportError);
    require(plc.lastError() == slmp::Error::TransportError);
    plc.close();
}

void testDelayedTimeout(const char* host, uint16_t port) {
    SocketTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setTimeoutMs(100);

    require(plc.connect(host, port));

    slmp::TypeNameInfo type_name = {};
    require(plc.readTypeName(type_name) == slmp::Error::TransportError);
    require(plc.lastError() == slmp::Error::TransportError);
    plc.close();
}

void testMalformedResponse(const char* host, uint16_t port) {
    SocketTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setTimeoutMs(1000);

    require(plc.connect(host, port));

    slmp::TypeNameInfo type_name = {};
    require(plc.readTypeName(type_name) == slmp::Error::ProtocolError);
    require(plc.lastError() == slmp::Error::ProtocolError);
    plc.close();
}

}  // namespace

int main() {
    const char* host = getenvOrDefault("SLMP_TEST_HOST", "127.0.0.1");
    const uint16_t port = parsePort(std::getenv("SLMP_TEST_PORT"));
    const char* password = getenvOrDefault("SLMP_TEST_PASSWORD", "123456");
    const char* scenario = getenvOrDefault("SLMP_TEST_SCENARIO", "normal");

    if (std::strcmp(scenario, "normal") == 0) {
        testSocketRoundTrip(host, port, password);
    } else if (std::strcmp(scenario, "plc_error") == 0) {
        testInjectedPlcError(host, port);
    } else if (std::strcmp(scenario, "disconnect") == 0) {
        testDisconnectDuringResponse(host, port);
    } else if (std::strcmp(scenario, "delay") == 0) {
        testDelayedTimeout(host, port);
    } else if (std::strcmp(scenario, "malformed") == 0) {
        testMalformedResponse(host, port);
    } else {
        std::fprintf(stderr, "unknown scenario: %s\n", scenario);
        return 2;
    }

    std::printf("slmp_socket_integration: %s ok\n", scenario);
    return 0;
}
