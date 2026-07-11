#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define SLMP_ENABLE_UDP_TRANSPORT 0
#include "slmp_arduino_transport.h"

namespace {

class FakeClient final : public Client {
  public:
    int connect(const char*, uint16_t) override {
        connected_ = connect_result_ == 1;
        return connect_result_;
    }
    void stop() override {
        stopped_ = true;
        connected_ = false;
    }
    uint8_t connected() override { return connected_ ? 1U : 0U; }
    size_t write(const uint8_t*, size_t length) override { return length; }
    int read(uint8_t*, size_t) override { return 0; }
    int available() override { return 0; }

    int connect_result_ = 1;
    bool connected_ = false;
    bool stopped_ = false;
};

uint32_t g_idle_seconds = 0U;

bool acceptKeepAlive(Client&, uint32_t idle_seconds) {
    g_idle_seconds = idle_seconds;
    return true;
}

bool rejectKeepAlive(Client&, uint32_t idle_seconds) {
    g_idle_seconds = idle_seconds;
    return false;
}

}  // namespace

int main() {
    {
        FakeClient client;
        slmp::ArduinoClientTransport transport(client, acceptKeepAlive);
        assert(transport.connect("192.0.2.1", 1025U));
        assert(g_idle_seconds == 30U);
        assert(client.connected_);
        assert(!client.stopped_);
    }
    {
        FakeClient client;
        slmp::ArduinoClientTransport transport(client, rejectKeepAlive);
        assert(!transport.connect("192.0.2.1", 1025U));
        assert(g_idle_seconds == 30U);
        assert(!client.connected_);
        assert(client.stopped_);
    }
    {
        FakeClient client;
        slmp::ArduinoClientTransport transport(client, nullptr);
        assert(!transport.connect("192.0.2.1", 1025U));
        assert(!client.connected_);
        assert(client.stopped_);
    }
    return 0;
}
