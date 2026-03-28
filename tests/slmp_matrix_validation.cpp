#include <assert.h>
#include <stdint.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <iostream>

#include "slmp_minimal.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
struct SocketRuntimeInit {
    SocketRuntimeInit() { WSADATA data = {}; WSAStartup(MAKEWORD(2, 2), &data); }
    ~SocketRuntimeInit() { WSACleanup(); }
};
void closeSocket(SocketHandle handle) { if (handle != kInvalidSocket) closesocket(handle); }
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
void closeSocket(SocketHandle handle) { if (handle != kInvalidSocket) close(handle); }
#endif

class SocketTransport : public slmp::ITransport {
public:
    SocketTransport() = default;
    ~SocketTransport() override { close(); }
    bool connect(const char* host, uint16_t port) override {
        close();
#ifdef _WIN32
        static SocketRuntimeInit init;
#endif
        addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        char port_str[8]; sprintf(port_str, "%u", port);
        if (getaddrinfo(host, port_str, &hints, &res) != 0) return false;
        for (auto it = res; it; it = it->ai_next) {
            SocketHandle h = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (h == kInvalidSocket) continue;
            if (::connect(h, it->ai_addr, (int)it->ai_addrlen) == 0) {
                socket_ = h; connected_ = true;
#ifdef _WIN32
                u_long mode = 1; ioctlsocket(socket_, FIONBIO, &mode);
#else
                fcntl(socket_, F_SETFL, fcntl(socket_, F_GETFL, 0) | O_NONBLOCK);
#endif
                freeaddrinfo(res); return true;
            }
            closeSocket(h);
        }
        freeaddrinfo(res); return false;
    }
    void close() override { closeSocket(socket_); socket_ = kInvalidSocket; connected_ = false; }
    bool connected() const override { return connected_; }
    bool writeAll(const uint8_t* data, size_t length) override {
        size_t sent = 0;
        while (sent < length) {
            int r = ::send(socket_, (const char*)data + sent, (int)(length - sent), 0);
            if (r <= 0) {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) { std::this_thread::yield(); continue; }
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) { std::this_thread::yield(); continue; }
#endif
                close(); return false;
            }
            sent += r;
        }
        return true;
    }
    bool readExact(uint8_t* data, size_t length, uint32_t timeout_ms) override {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        size_t received = 0;
        while (received < length) {
            if (std::chrono::steady_clock::now() > deadline) return false;
            int r = ::recv(socket_, (char*)data + received, (int)(length - received), 0);
            if (r <= 0) {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) { std::this_thread::yield(); continue; }
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) { std::this_thread::yield(); continue; }
#endif
                close(); return false;
            }
            received += r;
        }
        return true;
    }
    size_t write(const uint8_t* data, size_t length) override {
        int r = ::send(socket_, (const char*)data, (int)length, 0);
        if (r < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
            close(); return 0;
        }
        return (size_t)r;
    }
    size_t read(uint8_t* data, size_t length) override {
        int r = ::recv(socket_, (char*)data, (int)length, 0);
        if (r < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 0;
#endif
            close(); return 0;
        }
        return (size_t)r;
    }
    size_t available() override {
        u_long arg = 0;
#ifdef _WIN32
        ioctlsocket(socket_, FIONREAD, &arg);
#else
        ioctl(socket_, FIONREAD, &arg);
#endif
        return (size_t)arg;
    }
private:
    SocketHandle socket_ = kInvalidSocket;
    bool connected_ = false;
};

uint32_t getNow() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void runAsync(slmp::SlmpClient& plc) {
    while (plc.isBusy()) {
        plc.update(getNow());
        std::this_thread::yield();
    }
}

#define ASSERT_OK_PLC(expr) do { \
    slmp::Error err = (expr); \
    if (err != slmp::Error::Ok) { \
        fprintf(stderr, "FAIL: %s | err=%s, end=0x%04X\n", #expr, slmp::errorString(err), plc.lastEndCode()); \
        exit(1); \
    } \
} while(0)

struct TestCase {
    const char* name;
    slmp::DeviceCode code;
    uint32_t addr;
    bool is_bit;
};

const TestCase kTestDevices[] = {
    {"D",  slmp::DeviceCode::D,  1000, false},
    {"W",  slmp::DeviceCode::W,  0x100, false},
    {"R",  slmp::DeviceCode::R,  500,  false},
    {"ZR", slmp::DeviceCode::ZR, 2000, false},
    {"M",  slmp::DeviceCode::M,  1000, true},
    {"X",  slmp::DeviceCode::X,  0x20,  true}, // Note: X/Y are hex but dev::hex() is just a helper
    {"Y",  slmp::DeviceCode::Y,  0x20,  true},
    {"B",  slmp::DeviceCode::B,  0x100, true},
    {"L",  slmp::DeviceCode::L,  100,  true},
    {"SB", slmp::DeviceCode::SB, 0x40,  true}
};

void runMatrix(slmp::SlmpClient& plc, slmp::FrameType frame) {
    plc.setFrameType(frame);
    const char* frame_str = (frame == slmp::FrameType::Frame4E) ? "4E" : "3E";
    printf("--- Testing Frame Type: %s ---\n", frame_str);

    for (const auto& tc : kTestDevices) {
        printf("Device %s: ", tc.name);
        slmp::DeviceAddress dev = {tc.code, tc.addr};

        // 1. Sync Pattern
        if (tc.is_bit) {
            bool val = (tc.addr % 2 == 0);
            ASSERT_OK_PLC(plc.writeOneBit(dev, val));
            bool readback = !val;
            ASSERT_OK_PLC(plc.readOneBit(dev, readback));
            if (readback != val) { printf("Sync Bit Mismatch! "); exit(1); }
        } else {
            uint16_t val = (uint16_t)(tc.addr & 0xFFFF);
            ASSERT_OK_PLC(plc.writeOneWord(dev, val));
            uint16_t readback = 0;
            ASSERT_OK_PLC(plc.readOneWord(dev, readback));
            if (readback != val) { printf("Sync Word Mismatch! "); exit(1); }
        }
        printf("Sync OK, ");

        // 2. Async Pattern
        if (tc.is_bit) {
            bool val = (tc.addr % 2 != 0);
            ASSERT_OK_PLC(plc.beginWriteBits(dev, &val, 1, getNow()));
            runAsync(plc); ASSERT_OK_PLC(plc.lastError());
            bool readback = !val;
            ASSERT_OK_PLC(plc.beginReadBits(dev, 1, &readback, 1, getNow()));
            runAsync(plc); ASSERT_OK_PLC(plc.lastError());
            if (readback != val) { printf("Async Bit Mismatch! "); exit(1); }
        } else {
            uint16_t val = (uint16_t)(~tc.addr & 0xFFFF);
            ASSERT_OK_PLC(plc.beginWriteWords(dev, &val, 1, getNow()));
            runAsync(plc); ASSERT_OK_PLC(plc.lastError());
            uint16_t readback = 0;
            ASSERT_OK_PLC(plc.beginReadWords(dev, 1, &readback, 1, getNow()));
            runAsync(plc); ASSERT_OK_PLC(plc.lastError());
            if (readback != val) { printf("Async Word Mismatch! "); exit(1); }
        }
        printf("Async OK\n");
    }

    // 3. Random/Block Mixed Test (Async)
    printf("Random/Block Mixed Async: ");
    const slmp::DeviceAddress rw[] = { {slmp::DeviceCode::D, 500}, {slmp::DeviceCode::D, 501} };
    const uint16_t rv[] = { 0x1234, 0x5678 };
    ASSERT_OK_PLC(plc.beginWriteRandomWords(rw, rv, 2, nullptr, nullptr, 0, getNow()));
    runAsync(plc); ASSERT_OK_PLC(plc.lastError());

    const slmp::DeviceBlockRead blks[] = { slmp::dev::blockRead({slmp::DeviceCode::D, 500}, 2) };
    uint16_t br[2];
    ASSERT_OK_PLC(plc.beginReadBlock(blks, 1, nullptr, 0, br, 2, nullptr, 0, getNow()));
    runAsync(plc); ASSERT_OK_PLC(plc.lastError());
    assert(br[0] == 0x1234 && br[1] == 0x5678);
    printf("OK\n");
}

} // namespace

int main(int argc, char** argv) {
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (argc > 2) ? (uint16_t)atoi(argv[2]) : 5511;

    printf("Starting SLMP 3E/4E Sync/Async Full Matrix Validation on %s:%u\n", host, port);

    SocketTransport transport;
    uint8_t tx[1024], rx[1024];
    slmp::SlmpClient plc(transport, tx, sizeof(tx), rx, sizeof(rx));
    plc.setTimeoutMs(2000);

    if (!plc.connect(host, port)) {
        fprintf(stderr, "Connection failed.\n");
        return 1;
    }

    runMatrix(plc, slmp::FrameType::Frame4E);
    printf("\n");
    runMatrix(plc, slmp::FrameType::Frame3E);

    printf("\n==========================================\n");
    printf("ALL PATTERNS PASSED (3E/4E, Sync/Async, All Devices)\n");
    printf("==========================================\n");
    return 0;
}
