#include <assert.h>
#include <stdint.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>

#include "slmp_minimal.h"

// Note: This test requires SocketTransport from slmp_socket_integration.cpp
// We'll re-implement a minimal version here for a self-contained validation tool.

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
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

#define ASSERT_OK(expr) do { \
    slmp::Error err = (expr); \
    if (err != slmp::Error::Ok) { \
        fprintf(stderr, "ASSERT FAILED: %s -> error=%s, end_code=0x%04X\n", #expr, slmp::errorString(err), plc.lastEndCode()); \
        exit(1); \
    } \
} while(0)

void testSyncAsyncConsistency(slmp::SlmpClient& plc) {
    printf("Testing Sync/Async Consistency...\n");
    auto d100 = slmp::dev::D(slmp::dev::dec(100));
    uint16_t write_val[5] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
    uint16_t read_val_sync[5] = {0};
    uint16_t read_val_async[5] = {0};

    // 1. Sync Write
    ASSERT_OK(plc.writeWords(d100, write_val, 5));
    // 2. Sync Read
    ASSERT_OK(plc.readWords(d100, 5, read_val_sync, 5));
    // 3. Async Read
    ASSERT_OK(plc.beginReadWords(d100, 5, read_val_async, 5, getNow()));
    runAsync(plc);
    ASSERT_OK(plc.lastError());

    for(int i=0; i<5; i++) {
        assert(read_val_sync[i] == write_val[i]);
        assert(read_val_async[i] == write_val[i]);
    }
    printf("  -> OK\n");
}

void testLargeTransfer(slmp::SlmpClient& plc) {
    printf("Testing Large Transfer (90 words)...\n");
    auto d200 = slmp::dev::D(slmp::dev::dec(200));
    uint16_t data[90];
    for(int i=0; i<90; i++) data[i] = (uint16_t)i;
    uint16_t readback[90] = {0};

    ASSERT_OK(plc.beginWriteWords(d200, data, 90, getNow()));
    runAsync(plc);
    ASSERT_OK(plc.lastError());

    ASSERT_OK(plc.beginReadWords(d200, 90, readback, 90, getNow()));
    runAsync(plc);
    ASSERT_OK(plc.lastError());

    for(int i=0; i<90; i++) assert(readback[i] == data[i]);
    printf("  -> OK\n");
}

void testRandomAndBlock(slmp::SlmpClient& plc) {
    printf("Testing Random and Block Access (Async)...\n");
    
    // Random Write
    const slmp::DeviceAddress words[] = { slmp::dev::D(slmp::dev::dec(300)), slmp::dev::D(slmp::dev::dec(301)) };
    const uint16_t wvals[] = { 0xAAAA, 0xBBBB };
    const slmp::DeviceAddress dwords[] = { slmp::dev::D(slmp::dev::dec(310)) };
    const uint32_t dwvals[] = { 0x12345678 };
    
    ASSERT_OK(plc.beginWriteRandomWords(words, wvals, 2, dwords, dwvals, 1, getNow()));
    runAsync(plc);
    ASSERT_OK(plc.lastError());

    // Block Read
    const slmp::DeviceBlockRead blks[] = { slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2) };
    uint16_t blk_res[2];
    ASSERT_OK(plc.beginReadBlock(blks, 1, nullptr, 0, blk_res, 2, nullptr, 0, getNow()));
    runAsync(plc);
    ASSERT_OK(plc.lastError());
    
    assert(blk_res[0] == 0xAAAA);
    assert(blk_res[1] == 0xBBBB);
    printf("  -> OK\n");
}

void testStress(slmp::SlmpClient& plc) {
    printf("Testing Stress (100 sequential async requests)...\n");
    auto d0 = slmp::dev::D(slmp::dev::dec(0));
    for(int i=0; i<100; i++) {
        uint16_t write_val = (uint16_t)i;
        ASSERT_OK(plc.beginWriteWords(d0, &write_val, 1, getNow()));
        runAsync(plc);
        ASSERT_OK(plc.lastError());
        
        uint16_t readback = 0;
        ASSERT_OK(plc.beginReadWords(d0, 1, &readback, 1, getNow()));
        runAsync(plc);
        ASSERT_OK(plc.lastError());
        assert(readback == (uint16_t)i);
    }
    printf("  -> OK\n");
}

} // namespace

int main(int argc, char** argv) {
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t port = (argc > 2) ? (uint16_t)atoi(argv[2]) : 1025;

    printf("Starting GXSIM3 Thorough Validation on %s:%u\n", host, port);

    SocketTransport transport;
    uint8_t tx_buf[512], rx_buf[512];
    slmp::SlmpClient plc(transport, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
    plc.setTimeoutMs(2000);

    if (!plc.connect(host, port)) {
        fprintf(stderr, "Failed to connect to %s:%u. Make sure GXSIM3 is running and SLMP is enabled.\n", host, port);
        return 1;
    }

    slmp::TypeNameInfo info;
    ASSERT_OK(plc.readTypeName(info));
    printf("Connected to: %s (Model Code: 0x%04X)\n", info.model, info.model_code);

    testSyncAsyncConsistency(plc);
    testLargeTransfer(plc);
    testRandomAndBlock(plc);
    testStress(plc);

    printf("\nALL TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
