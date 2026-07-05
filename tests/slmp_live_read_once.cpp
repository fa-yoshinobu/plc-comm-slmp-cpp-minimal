#include <stdint.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "slmp_error_codes.h"
#include "slmp_minimal.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
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
        WSAStartup(MAKEWORD(2, 2), &data);
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

class SocketTransport : public slmp::ITransport {
  public:
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
                const int flags = fcntl(socket_, F_GETFL, 0);
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
#ifdef _WIN32
            const int sent = ::send(
                socket_,
                reinterpret_cast<const char*>(data + offset),
                static_cast<int>(length - offset),
                0
            );
#else
            const ssize_t sent = ::send(socket_, data + offset, length - offset, 0);
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
            const int received = ::recv(
                socket_,
                reinterpret_cast<char*>(data + offset),
                static_cast<int>(length - offset),
                0
            );
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

struct ParsedDevice {
    slmp::DeviceAddress address;
    const char* code;
    uint32_t number;
};

bool parsePort(const char* text, uint16_t& out) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    char* end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value == 0UL || value > 65535UL) {
        return false;
    }
    out = static_cast<uint16_t>(value);
    return true;
}

bool parseDecimalDevice(const char* text, ParsedDevice& out) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    const char* digits = nullptr;
    slmp::DeviceCode code = slmp::DeviceCode::D;
    if (std::strncmp(text, "D", 1) == 0) {
        digits = text + 1;
        code = slmp::DeviceCode::D;
        out.code = "D";
    } else if (std::strncmp(text, "SD", 2) == 0) {
        digits = text + 2;
        code = slmp::DeviceCode::SD;
        out.code = "SD";
    } else if (std::strncmp(text, "SM", 2) == 0) {
        digits = text + 2;
        code = slmp::DeviceCode::SM;
        out.code = "SM";
    } else {
        return false;
    }

    char* end = nullptr;
    const unsigned long number = std::strtoul(digits, &end, 10);
    if (end == digits || *end != '\0' || number > 0xFFFFFFFFUL) {
        return false;
    }
    out.number = static_cast<uint32_t>(number);
    out.address = {code, out.number};
    return true;
}

bool parseProfile(const char* text, slmp::PlcProfile& out) {
    if (std::strcmp(text, "melsec:iq-r") == 0) {
        out = slmp::PlcProfile::IqR;
        return true;
    }
    if (std::strcmp(text, "melsec:iq-r:rj71en71") == 0) {
        out = slmp::PlcProfile::IqRRj71En71;
        return true;
    }
    if (std::strcmp(text, "melsec:lcpu") == 0) {
        out = slmp::PlcProfile::LCpu;
        return true;
    }
    if (std::strcmp(text, "melsec:qcpu:qj71e71-100") == 0) {
        out = slmp::PlcProfile::QCpuQj71E71100;
        return true;
    }
    if (std::strcmp(text, "melsec:lcpu:lj71e71-100") == 0) {
        out = slmp::PlcProfile::LCpuLj71E71100;
        return true;
    }
    if (std::strcmp(text, "melsec:qnu:qj71e71-100") == 0) {
        out = slmp::PlcProfile::QnUQj71E71100;
        return true;
    }
    if (std::strcmp(text, "melsec:qnudv:qj71e71-100") == 0) {
        out = slmp::PlcProfile::QnUDVQj71E71100;
        return true;
    }
    return false;
}

const char* frameName(slmp::FrameType frame) {
    return frame == slmp::FrameType::Frame4E ? "4E" : "3E";
}

const char* compatibilityName(slmp::CompatibilityMode mode) {
    return mode == slmp::CompatibilityMode::Legacy ? "Q/L" : "iQ-R";
}

const char* errorName(slmp::Error error) {
    switch (error) {
        case slmp::Error::Ok: return "Ok";
        case slmp::Error::InvalidArgument: return "InvalidArgument";
        case slmp::Error::TransportError: return "TransportError";
        case slmp::Error::ProtocolError: return "ProtocolError";
        case slmp::Error::PlcError: return "PlcError";
        case slmp::Error::Busy: return "Busy";
        case slmp::Error::BufferTooSmall: return "BufferTooSmall";
        case slmp::Error::UnsupportedDevice: return "UnsupportedDevice";
        case slmp::Error::ProfileFeatureBlocked: return "ProfileFeatureBlocked";
        default: return "Unknown";
    }
}

void printUsage() {
    std::fprintf(
        stderr,
        "usage: slmp_live_read_once <host> <port> <profile> <device>\n"
        "profiles: melsec:iq-r | melsec:iq-r:rj71en71 | melsec:lcpu | melsec:qcpu:qj71e71-100 | melsec:lcpu:lj71e71-100 | "
        "melsec:qnu:qj71e71-100 | melsec:qnudv:qj71e71-100\n"
        "devices: decimal D, SD, or SM address such as D1000\n"
    );
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        printUsage();
        return 2;
    }

    const char* host = argv[1];
    uint16_t port = 0;
    if (!parsePort(argv[2], port)) {
        std::fprintf(stderr, "invalid port: %s\n", argv[2]);
        return 2;
    }

    slmp::PlcProfile profile = slmp::PlcProfile::Unspecified;
    if (!parseProfile(argv[3], profile)) {
        std::fprintf(stderr, "unsupported profile: %s\n", argv[3]);
        return 2;
    }

    ParsedDevice device = {};
    if (!parseDecimalDevice(argv[4], device)) {
        std::fprintf(stderr, "unsupported read device: %s\n", argv[4]);
        return 2;
    }

    SocketTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    plc.setPlcProfile(profile);
    plc.setTimeoutMs(3000);

    if (!plc.connect(host, port)) {
        std::fprintf(stderr, "{\"status\":\"error\",\"stage\":\"connect\",\"error\":\"%s\"}\n", errorName(plc.lastError()));
        return 1;
    }

    uint16_t value = 0;
    const slmp::Error error = plc.readOneWord(device.address, value);
    plc.close();
    if (error != slmp::Error::Ok) {
        if (error == slmp::Error::PlcError) {
            std::fprintf(
                stderr,
                "{\"status\":\"error\",\"stage\":\"read\",\"error\":\"%s\",\"end_code\":\"%s\"}\n",
                errorName(error),
                slmp::endCodeString(plc.lastEndCode())
            );
        } else {
            std::fprintf(stderr, "{\"status\":\"error\",\"stage\":\"read\",\"error\":\"%s\"}\n", errorName(error));
        }
        return 1;
    }

    std::printf(
        "{\"status\":\"success\",\"profile\":\"%s\",\"frame\":\"%s\",\"compat\":\"%s\","
        "\"device\":\"%s%lu\",\"values\":[%u]}\n",
        argv[3],
        frameName(plc.frameType()),
        compatibilityName(plc.compatibilityMode()),
        device.code,
        static_cast<unsigned long>(device.number),
        static_cast<unsigned>(value)
    );
    return 0;
}
