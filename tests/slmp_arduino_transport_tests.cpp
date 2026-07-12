#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define SLMP_ENABLE_UDP_TRANSPORT 1
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

class FakeUdp final : public UDP {
  public:
    uint8_t begin(uint16_t port) override {
        requested_local_port_ = port;
        assigned_local_port_ = port == 0U ? next_ephemeral_port_++ : port;
        return begin_result_;
    }
    void stop() override { stopped_ = true; }
    int beginPacket(const char* host, uint16_t port) override {
        packet_host_ = host == nullptr ? "" : host;
        packet_port_ = port;
        return begin_packet_result_;
    }
    int endPacket() override { return end_packet_result_; }
    size_t write(const uint8_t*, size_t length) override { return length; }
    int parsePacket() override { return 0; }
    int available() override { return 0; }
    int read(uint8_t*, size_t) override { return 0; }

    static uint16_t next_ephemeral_port_;
    uint16_t requested_local_port_ = 0U;
    uint16_t assigned_local_port_ = 0U;
    uint16_t packet_port_ = 0U;
    std::string packet_host_;
    uint8_t begin_result_ = 1U;
    int begin_packet_result_ = 1;
    int end_packet_result_ = 1;
    bool stopped_ = false;
};

uint16_t FakeUdp::next_ephemeral_port_ = 49152U;

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
    {
        FakeUdp first_udp;
        FakeUdp second_udp;
        slmp::ArduinoUdpTransport first(first_udp);
        slmp::ArduinoUdpTransport second(second_udp);

        assert(first.connect("192.0.2.10", 1035U));
        assert(second.connect("192.0.2.11", 1035U));
        assert(first_udp.requested_local_port_ == 0U);
        assert(second_udp.requested_local_port_ == 0U);
        assert(first_udp.assigned_local_port_ != second_udp.assigned_local_port_);
        assert(first_udp.assigned_local_port_ != 1035U);
        assert(second_udp.assigned_local_port_ != 1035U);

        const uint8_t frame[] = {0x50U, 0x00U};
        assert(first.writeAll(frame, sizeof(frame)));
        assert(first_udp.packet_host_ == "192.0.2.10");
        assert(first_udp.packet_port_ == 1035U);
    }
    {
        FakeUdp udp;
        slmp::ArduinoUdpTransport transport(udp, 20000U);
        assert(transport.connect("192.0.2.20", 1035U));
        assert(udp.requested_local_port_ == 20000U);
        assert(udp.assigned_local_port_ == 20000U);
    }
    return 0;
}
