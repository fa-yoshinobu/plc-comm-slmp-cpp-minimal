#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <cstring>
#include <vector>

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
    int parsePacket() override {
        if (incoming_offset_ < incoming_data_.size()) return static_cast<int>(incoming_data_.size() - incoming_offset_);
        return 0;
    }
    IPAddress remoteIP() override { return incoming_ip_; }
    uint16_t remotePort() override { return incoming_port_; }
    int available() override { return static_cast<int>(incoming_data_.size() - incoming_offset_); }
    int read(uint8_t* out, size_t length) override {
        const size_t count = std::min(length, incoming_data_.size() - incoming_offset_);
        if (count == 0U) return 0;
        std::memcpy(out, incoming_data_.data() + incoming_offset_, count);
        incoming_offset_ += count;
        return static_cast<int>(count);
    }

    void queuePacket(const char* ip, uint16_t port, std::initializer_list<uint8_t> data) {
        incoming_ip_.fromString(ip);
        incoming_port_ = port;
        incoming_data_.assign(data);
        incoming_offset_ = 0U;
    }

    static uint16_t next_ephemeral_port_;
    uint16_t requested_local_port_ = 0U;
    uint16_t assigned_local_port_ = 0U;
    uint16_t packet_port_ = 0U;
    std::string packet_host_;
    uint8_t begin_result_ = 1U;
    int begin_packet_result_ = 1;
    int end_packet_result_ = 1;
    bool stopped_ = false;
    IPAddress incoming_ip_;
    uint16_t incoming_port_ = 0U;
    std::vector<uint8_t> incoming_data_;
    size_t incoming_offset_ = 0U;
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
    {
        FakeUdp udp;
        udp.begin_result_ = 0U;
        slmp::ArduinoUdpTransport transport(udp);
        assert(!transport.connect("192.0.2.30", 1035U));
        const uint8_t frame[] = {0x50U, 0x00U};
        assert(!transport.writeAll(frame, sizeof(frame)));
    }
    {
        FakeUdp udp;
        slmp::ArduinoUdpTransport transport(udp);
        assert(transport.connect("192.0.2.40", 1035U));
        udp.queuePacket("192.0.2.41", 1035U, {0xD0U, 0x00U});
        assert(transport.available() == 0U);
        udp.queuePacket("192.0.2.40", 1036U, {0xD0U, 0x00U});
        assert(transport.available() == 0U);
        udp.queuePacket("192.0.2.40", 1035U, {0xD0U, 0x00U, 0x01U});
        assert(transport.available() == 3U);
    }
    return 0;
}
