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
    size_t write(const uint8_t* data, size_t length) override {
        outgoing_data_.assign(data, data + length);
        return length;
    }
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

    void queuePacket(const char* ip, uint16_t port, const std::vector<uint8_t>& data) {
        incoming_ip_.fromString(ip);
        incoming_port_ = port;
        incoming_data_ = data;
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
    std::vector<uint8_t> outgoing_data_;
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

void appendLe16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
}

std::vector<uint8_t> makeResponseWithPayload(
    const std::vector<uint8_t>& request,
    const std::vector<uint8_t>& payload
) {
    std::vector<uint8_t> response;
    const bool frame4e = request.size() >= 2U && request[0] == 0x54U && request[1] == 0x00U;
    if (frame4e) {
        response = {0xD4U, 0x00U, request[2], request[3], 0x00U, 0x00U,
                    request[6], request[7], request[8], request[9], request[10]};
    } else {
        assert(request.size() >= 7U && request[0] == 0x50U && request[1] == 0x00U);
        response = {0xD0U, 0x00U, request[2], request[3], request[4], request[5], request[6]};
    }
    appendLe16(response, static_cast<uint16_t>(2U + payload.size()));
    appendLe16(response, 0U);
    response.insert(response.end(), payload.begin(), payload.end());
    return response;
}

std::vector<uint8_t> makeResponse(const std::vector<uint8_t>& request, uint16_t word_value) {
    return makeResponseWithPayload(
        request,
        {
            static_cast<uint8_t>(word_value & 0xFFU),
            static_cast<uint8_t>((word_value >> 8) & 0xFFU),
        }
    );
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
    for (const slmp::PlcProfile profile : {slmp::PlcProfile::IqR, slmp::PlcProfile::IqF}) {
        FakeUdp udp;
        slmp::ArduinoUdpTransport transport(udp);
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        const slmp::TargetAddress target{0x12U, 0x34U, 0x5678U, 0x9AU};
        slmp::SlmpClient plc(
            transport,
            profile,
            target,
            tx_buffer,
            sizeof(tx_buffer),
            rx_buffer,
            sizeof(rx_buffer)
        );
        assert(plc.connect("192.0.2.50", 1035U));

        uint16_t value = 0U;
        const uint32_t started_at = 1000U;
        assert(plc.beginReadWords(
                   slmp::dev::D(profile, slmp::dev::dec(100)),
                   1U,
                   &value,
                   1U,
                   started_at) == slmp::Error::Ok);
        plc.update(started_at);
        assert(!udp.outgoing_data_.empty());

        const std::vector<uint8_t> matching = makeResponse(udp.outgoing_data_, 0x1234U);
        std::vector<uint8_t> oversized_foreign = makeResponseWithPayload(
            udp.outgoing_data_,
            std::vector<uint8_t>(256U, 0xA5U)
        );
        std::vector<uint8_t> foreign_route = matching;
        const size_t station_offset = profile == slmp::PlcProfile::IqR ? 7U : 3U;
        oversized_foreign[station_offset] ^= 0x01U;
        foreign_route[station_offset] ^= 0x01U;
        assert(oversized_foreign.size() > sizeof(rx_buffer));
        udp.queuePacket("192.0.2.50", 1035U, oversized_foreign);
        for (size_t step = 0U; step < 8U; ++step) plc.update(started_at);
        assert(plc.isBusy());

        udp.queuePacket("192.0.2.50", 1035U, foreign_route);
        plc.update(started_at);
        plc.update(started_at);
        assert(plc.isBusy());

        if (profile == slmp::PlcProfile::IqR) {
            std::vector<uint8_t> wrong_serial = matching;
            wrong_serial[2] ^= 0x01U;
            udp.queuePacket("192.0.2.50", 1035U, wrong_serial);
            plc.update(started_at);
            plc.update(started_at);
            assert(plc.isBusy());
        }

        udp.queuePacket("192.0.2.50", 1035U, matching);
        plc.update(started_at);
        plc.update(started_at);
        assert(!plc.isBusy());
        assert(plc.lastError() == slmp::Error::Ok);
        assert(plc.connected());
        assert(value == 0x1234U);
        const uint64_t expected_frames = profile == slmp::PlcProfile::IqR ? 3U : 2U;
        assert(plc.trafficStats().rx_bytes ==
               oversized_foreign.size() + (matching.size() * expected_frames));

        const uint64_t rx_before_truncated = plc.trafficStats().rx_bytes;
        value = 0x7777U;
        assert(plc.beginReadWords(
                   slmp::dev::D(profile, slmp::dev::dec(101)),
                   1U,
                   &value,
                   1U,
                   started_at + 1U) == slmp::Error::Ok);
        plc.update(started_at + 1U);
        std::vector<uint8_t> truncated_foreign = makeResponse(udp.outgoing_data_, 0xBEEFU);
        truncated_foreign[station_offset] ^= 0x01U;
        truncated_foreign.resize(truncated_foreign.size() - 2U);
        udp.queuePacket("192.0.2.50", 1035U, truncated_foreign);
        plc.update(started_at + 1U);
        assert(!plc.isBusy());
        assert(plc.lastError() == slmp::Error::ProtocolError);
        assert(!plc.connected());
        assert(value == 0x7777U);
        assert(plc.trafficStats().rx_bytes == rx_before_truncated);

        assert(plc.connect("192.0.2.50", 1035U));
        assert(plc.beginReadWords(
                   slmp::dev::D(profile, slmp::dev::dec(102)),
                   1U,
                   &value,
                   1U,
                   started_at + 2U) == slmp::Error::Ok);
        plc.update(started_at + 2U);
        const std::vector<uint8_t> clean_after_truncated = makeResponse(udp.outgoing_data_, 0xCAFEU);
        udp.queuePacket("192.0.2.50", 1035U, clean_after_truncated);
        plc.update(started_at + 2U);
        plc.update(started_at + 2U);
        assert(!plc.isBusy());
        assert(plc.lastError() == slmp::Error::Ok);
        assert(plc.connected());
        assert(value == 0xCAFEU);
        assert(plc.trafficStats().rx_bytes == rx_before_truncated + clean_after_truncated.size());

        const uint64_t rx_before_short_prefix = plc.trafficStats().rx_bytes;
        value = 0x6666U;
        assert(plc.beginReadWords(
                   slmp::dev::D(profile, slmp::dev::dec(103)),
                   1U,
                   &value,
                   1U,
                   started_at + 3U) == slmp::Error::Ok);
        plc.update(started_at + 3U);
        std::vector<uint8_t> short_foreign_prefix = makeResponse(udp.outgoing_data_, 0xABCDU);
        short_foreign_prefix[station_offset] ^= 0x01U;
        const size_t response_prefix_size = profile == slmp::PlcProfile::IqR ? 13U : 9U;
        short_foreign_prefix.resize(response_prefix_size - 1U);
        udp.queuePacket("192.0.2.50", 1035U, short_foreign_prefix);
        plc.update(started_at + 3U);
        assert(!plc.isBusy());
        assert(plc.lastError() == slmp::Error::ProtocolError);
        assert(!plc.connected());
        assert(value == 0x6666U);
        assert(plc.trafficStats().rx_bytes == rx_before_short_prefix);
    }
    return 0;
}
