#include <assert.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "slmp4e_minimal.h"
#include "slmp4e_utility.h"
#include "python_golden_frames.h"

namespace {

uint16_t readLe16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

void appendLe16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
}

void appendLe32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFFU));
}

std::vector<uint8_t> makeResponse(const std::vector<uint8_t>& request, uint16_t end_code, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(15U + data.size());
    out.push_back(0xD4);
    out.push_back(0x00);
    out.push_back(request[2]);
    out.push_back(request[3]);
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(request[6]);
    out.push_back(request[7]);
    out.push_back(request[8]);
    out.push_back(request[9]);
    out.push_back(request[10]);
    appendLe16(out, static_cast<uint16_t>(2U + data.size()));
    appendLe16(out, end_code);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

void assertBytesEqual(const std::vector<uint8_t>& actual, const uint8_t* expected, size_t expected_size) {
    assert(actual.size() == expected_size);
    assert(std::memcmp(actual.data(), expected, expected_size) == 0);
}

class MockTransport : public slmp4e::ITransport {
  public:
    bool connect(const char* host, uint16_t port) override {
        if (!connect_results_.empty()) {
            connected_ = connect_results_.front();
            connect_results_.pop_front();
            return connected_;
        }
        connected_ = (host != nullptr && port != 0U);
        return connected_;
    }

    void close() override {
        connected_ = false;
    }

    bool connected() const override {
        return connected_;
    }

    bool writeAll(const uint8_t* data, size_t length) override {
        if (fail_next_write_) {
            fail_next_write_ = false;
            connected_ = false;
            return false;
        }
        last_write_.assign(data, data + length);
        return true;
    }

    bool readExact(uint8_t* data, size_t length, uint32_t) override {
        if (fail_next_read_) {
            fail_next_read_ = false;
            connected_ = false;
            return false;
        }
        if (read_offset_ + length > queued_response_.size()) {
            return false;
        }
        memcpy(data, queued_response_.data() + read_offset_, length);
        read_offset_ += length;
        return true;
    }

    void queueConnectResult(bool result) {
        connect_results_.push_back(result);
    }

    void queueResponse(const std::vector<uint8_t>& response) {
        queued_response_ = response;
        read_offset_ = 0;
    }

    void setFailNextWrite(bool value = true) {
        fail_next_write_ = value;
    }

    void setFailNextRead(bool value = true) {
        fail_next_read_ = value;
    }

    const std::vector<uint8_t>& lastWrite() const {
        return last_write_;
    }

  private:
    bool connected_ = true;
    std::deque<bool> connect_results_;
    std::vector<uint8_t> last_write_;
    std::vector<uint8_t> queued_response_;
    size_t read_offset_ = 0;
    bool fail_next_write_ = false;
    bool fail_next_read_ = false;
};

void testReadWordsAndFrames() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x34, 0x12, 0x78, 0x56}));

    uint16_t words[2] = {};
    assert(plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, words, 2) == slmp4e::Error::Ok);
    assert(words[0] == 0x1234U);
    assert(words[1] == 0x5678U);
    assert(plc.lastRequestFrameLength() == 27U);
    assert(plc.lastResponseFrameLength() == 19U);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0401U);
    assert(readLe16(transport.lastWrite().data() + 17) == 0x0002U);
    assert(plc.lastRequestFrame() == tx_buffer);
    assert(plc.lastResponseFrame() == rx_buffer);

    char hex[64] = {};
    assert(slmp4e::formatHexBytes(transport.lastWrite().data(), 4, hex, sizeof(hex)) == 11U);
    assert(std::string(hex) == "54 00 00 00");
}

void testDWordAndOneShotHelpers() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0xC8, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x78, 0x56, 0x34, 0x12}));

    uint32_t dword = 0;
    assert(plc.readOneDWord(slmp4e::dev::D(slmp4e::dev::dec(200)), dword) == slmp4e::Error::Ok);
    assert(dword == 0x12345678UL);

    std::vector<uint8_t> second_request = provisional_request;
    second_request[2] = 0x01;
    second_request[15] = 0x01;
    second_request[16] = 0x14;
    second_request[17] = 0x03;
    second_request[18] = 0x00;
    transport.queueResponse(makeResponse(second_request, 0x0000, {}));
    assert(plc.writeOneBit(slmp4e::dev::M(slmp4e::dev::dec(101)), true) == slmp4e::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1401U);
    assert(readLe16(transport.lastWrite().data() + 17) == 0x0003U);
}

void testRandomAndBlock() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    std::vector<uint8_t> random_payload = {0x11, 0x11, 0x22, 0x22};
    appendLe32(random_payload, 0x12345678UL);
    const std::vector<uint8_t> serial0_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x14, 0x00, 0x10, 0x00,
        0x03, 0x04, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(serial0_request, 0x0000, random_payload));

    const slmp4e::DeviceAddress random_words[] = {
        slmp4e::dev::D(slmp4e::dev::dec(100)),
        slmp4e::dev::D(slmp4e::dev::dec(101)),
    };
    const slmp4e::DeviceAddress random_dwords[] = {
        slmp4e::dev::D(slmp4e::dev::dec(200)),
    };
    uint16_t word_values[2] = {};
    uint32_t dword_values[1] = {};
    assert(plc.readRandom(random_words, 2, word_values, 2, random_dwords, 1, dword_values, 1) == slmp4e::Error::Ok);
    assert(word_values[0] == 0x1111U);
    assert(word_values[1] == 0x2222U);
    assert(dword_values[0] == 0x12345678UL);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0403U);

    std::vector<uint8_t> block_payload = {0x34, 0x12, 0x78, 0x56, 0x05, 0x00};
    std::vector<uint8_t> serial1_request = serial0_request;
    serial1_request[2] = 0x01;
    serial1_request[15] = 0x06;
    serial1_request[16] = 0x04;
    transport.queueResponse(makeResponse(serial1_request, 0x0000, block_payload));

    const slmp4e::DeviceBlockRead word_blocks[] = {
        slmp4e::dev::blockRead(slmp4e::dev::D(slmp4e::dev::dec(300)), 2),
    };
    const slmp4e::DeviceBlockRead bit_blocks[] = {
        slmp4e::dev::blockRead(slmp4e::dev::M(slmp4e::dev::dec(200)), 1),
    };
    uint16_t block_words[2] = {};
    uint16_t block_bits[1] = {};
    assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_words, 2, block_bits, 1) == slmp4e::Error::Ok);
    assert(block_words[0] == 0x1234U);
    assert(block_words[1] == 0x5678U);
    assert(block_bits[0] == 0x0005U);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0406U);
}

void testPlcErrorAndStrings() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x01, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x4031, {}));

    uint16_t value = 0;
    assert(plc.readOneWord(slmp4e::dev::D(slmp4e::dev::dec(100)), value) == slmp4e::Error::PlcError);
    assert(plc.lastEndCode() == 0x4031U);
    assert(std::string(slmp4e::errorString(plc.lastError())) == "plc_error");
    assert(std::string(slmp4e::endCodeString(plc.lastEndCode())) == "range_or_allocation_mismatch");
    assert(std::string(slmp4e::endCodeString(0xDEADU)) == "unknown_plc_end_code");
}

void testPasswordAndWriteBlock() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> unlock_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0F, 0x00, 0x10, 0x00,
        0x30, 0x16, 0x00, 0x00
    };
    transport.queueResponse(makeResponse(unlock_request, 0x0000, {}));
    assert(plc.remotePasswordUnlock("secret1") == slmp4e::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1630U);
    assert(readLe16(transport.lastWrite().data() + 19) == 7U);
    assert(std::memcmp(transport.lastWrite().data() + 21, "secret1", 7) == 0);

    std::vector<uint8_t> block_request = unlock_request;
    block_request[2] = 0x01;
    block_request[15] = 0x06;
    block_request[16] = 0x14;
    transport.queueResponse(makeResponse(block_request, 0x0000, {}));

    const uint16_t block_values[] = {0x1234, 0x5678};
    const slmp4e::DeviceBlockWrite word_blocks[] = {
        slmp4e::dev::blockWrite(slmp4e::dev::D(slmp4e::dev::dec(400)), block_values, 2),
    };
    assert(plc.writeBlock(word_blocks, 1, nullptr, 0) == slmp4e::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1406U);
}

void testTransportFailuresAndReconnectHelper() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    transport.close();
    transport.queueConnectResult(false);
    transport.queueConnectResult(true);

    slmp4e::ReconnectHelper reconnect(
        plc,
        "192.168.250.101",
        1025,
        slmp4e::ReconnectOptions{1000}
    );

    assert(!reconnect.ensureConnected(0));
    assert(!reconnect.consumeConnectedEdge());
    assert(!reconnect.ensureConnected(500));
    assert(reconnect.ensureConnected(1000));
    assert(reconnect.consumeConnectedEdge());
    assert(!reconnect.consumeConnectedEdge());

    transport.setFailNextRead();
    slmp4e::TypeNameInfo type_name = {};
    assert(plc.readTypeName(type_name) == slmp4e::Error::TransportError);
    assert(plc.lastError() == slmp4e::Error::TransportError);

    transport.queueConnectResult(true);
    reconnect.forceReconnect(1200);
    assert(reconnect.ensureConnected(1200));
    assert(reconnect.consumeConnectedEdge());
}

void testPythonCompatibilityGoldenFrames() {
    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(std::vector<uint8_t>(python_golden::kReadTypeNameRequest, python_golden::kReadTypeNameRequest + python_golden::size(python_golden::kReadTypeNameRequest)), 0x0000, {
            'Q', '0', '3', 'U', 'D', 'V', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x34, 0x12
        }));
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kReadTypeNameRequest,
            python_golden::size(python_golden::kReadTypeNameRequest)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadWordsD1002Request,
                python_golden::kReadWordsD1002Request + python_golden::size(python_golden::kReadWordsD1002Request)
            ),
            0x0000,
            {0x34, 0x12, 0x78, 0x56}
        ));
        uint16_t words[2] = {};
        assert(plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, words, 2) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kReadWordsD1002Request,
            python_golden::size(python_golden::kReadWordsD1002Request)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kWriteBitsM101TrueRequest,
                python_golden::kWriteBitsM101TrueRequest + python_golden::size(python_golden::kWriteBitsM101TrueRequest)
            ),
            0x0000,
            {}
        ));
        assert(plc.writeOneBit(slmp4e::dev::M(slmp4e::dev::dec(101)), true) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kWriteBitsM101TrueRequest,
            python_golden::size(python_golden::kWriteBitsM101TrueRequest)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadRandomRequest,
                python_golden::kReadRandomRequest + python_golden::size(python_golden::kReadRandomRequest)
            ),
            0x0000,
            {0x11, 0x11, 0x22, 0x22, 0x78, 0x56, 0x34, 0x12}
        ));
        const slmp4e::DeviceAddress random_words[] = {
            slmp4e::dev::D(slmp4e::dev::dec(100)),
            slmp4e::dev::D(slmp4e::dev::dec(101)),
        };
        const slmp4e::DeviceAddress random_dwords[] = {
            slmp4e::dev::D(slmp4e::dev::dec(200)),
        };
        uint16_t word_values[2] = {};
        uint32_t dword_values[1] = {};
        assert(plc.readRandom(random_words, 2, word_values, 2, random_dwords, 1, dword_values, 1) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kReadRandomRequest,
            python_golden::size(python_golden::kReadRandomRequest)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kWriteRandomBitsRequest,
                python_golden::kWriteRandomBitsRequest + python_golden::size(python_golden::kWriteRandomBitsRequest)
            ),
            0x0000,
            {}
        ));
        const slmp4e::DeviceAddress random_bits[] = {
            slmp4e::dev::M(slmp4e::dev::dec(100)),
            slmp4e::dev::Y(slmp4e::dev::hex(0x20)),
        };
        const bool bit_values[] = {true, false};
        assert(plc.writeRandomBits(random_bits, bit_values, 2) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kWriteRandomBitsRequest,
            python_golden::size(python_golden::kWriteRandomBitsRequest)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadBlockRequest,
                python_golden::kReadBlockRequest + python_golden::size(python_golden::kReadBlockRequest)
            ),
            0x0000,
            {0x34, 0x12, 0x78, 0x56, 0x05, 0x00}
        ));
        const slmp4e::DeviceBlockRead word_blocks[] = {
            slmp4e::dev::blockRead(slmp4e::dev::D(slmp4e::dev::dec(300)), 2),
        };
        const slmp4e::DeviceBlockRead bit_blocks[] = {
            slmp4e::dev::blockRead(slmp4e::dev::M(slmp4e::dev::dec(200)), 1),
        };
        uint16_t block_words[2] = {};
        uint16_t block_bits[1] = {};
        assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_words, 2, block_bits, 1) == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kReadBlockRequest,
            python_golden::size(python_golden::kReadBlockRequest)
        );
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kRemoteUnlockRequest,
                python_golden::kRemoteUnlockRequest + python_golden::size(python_golden::kRemoteUnlockRequest)
            ),
            0x0000,
            {}
        ));
        assert(plc.remotePasswordUnlock("secret1") == slmp4e::Error::Ok);
        assertBytesEqual(
            transport.lastWrite(),
            python_golden::kRemoteUnlockRequest,
            python_golden::size(python_golden::kRemoteUnlockRequest)
        );
    }
}

void testProtocolFailures() {
    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00
        });
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00
        });
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x00
        });
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[14] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x14, 0x00
        });
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::BufferTooSmall);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.setFailNextWrite();
        slmp4e::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp4e::Error::TransportError);
    }
}

void testPayloadValidationFailures() {
    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x02, 0x00
        };
        transport.queueResponse(makeResponse(request, 0x0000, {0x34, 0x12}));
        uint16_t words[2] = {};
        assert(plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, words, 2) == slmp4e::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x03, 0x00
        };
        transport.queueResponse(makeResponse(request, 0x0000, {}));
        bool value = false;
        assert(plc.readOneBit(slmp4e::dev::M(slmp4e::dev::dec(100)), value) == slmp4e::Error::ProtocolError);
    }
}

}  // namespace

int main() {
    testReadWordsAndFrames();
    testDWordAndOneShotHelpers();
    testRandomAndBlock();
    testPlcErrorAndStrings();
    testPasswordAndWriteBlock();
    testTransportFailuresAndReconnectHelper();
    testPythonCompatibilityGoldenFrames();
    testProtocolFailures();
    testPayloadValidationFailures();
    std::puts("slmp4e_minimal_tests: ok");
    return 0;
}
