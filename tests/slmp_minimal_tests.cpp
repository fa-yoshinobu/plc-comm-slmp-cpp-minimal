#include <assert.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "slmp_minimal.h"
#include "slmp_utility.h"
#include "python_golden_frames.h"

namespace {

uint16_t readLe16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe24(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16);
}

uint32_t readLe32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
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

std::vector<uint8_t> makeResponse3E(const std::vector<uint8_t>& request, uint16_t end_code, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    out.reserve(11U + data.size());
    out.push_back(0xD0);
    out.push_back(0x00);
    out.push_back(request[2]);
    out.push_back(request[3]);
    out.push_back(request[4]);
    out.push_back(request[5]);
    out.push_back(request[6]);
    appendLe16(out, static_cast<uint16_t>(2U + data.size()));
    appendLe16(out, end_code);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

void assertBytesEqual(const std::vector<uint8_t>& actual, const uint8_t* expected, size_t expected_size) {
    assert(actual.size() == expected_size);
    assert(std::memcmp(actual.data(), expected, expected_size) == 0);
}

void driveAsyncUntilIdle(slmp::SlmpClient& plc, uint32_t now_ms, int max_steps = 32) {
    for (int i = 0; i < max_steps && plc.isBusy(); ++i) {
        plc.update(now_ms);
    }
    assert(!plc.isBusy());
}

class MockTransport : public slmp::ITransport {
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
        write_history_.push_back(last_write_);
        return true;
    }

    bool readExact(uint8_t* data, size_t length, uint32_t) override {
        normalizeQueuedResponses();
        if (fail_next_read_) {
            fail_next_read_ = false;
            connected_ = false;
            return false;
        }
        if (queued_responses_.empty() || read_offset_ + length > queued_responses_.front().size()) {
            return false;
        }
        memcpy(data, queued_responses_.front().data() + read_offset_, length);
        read_offset_ += length;
        normalizeQueuedResponses();
        return true;
    }

    size_t write(const uint8_t* data, size_t length) override {
        if (fail_next_write_) {
            fail_next_write_ = false;
            connected_ = false;
            return 0;
        }
        last_write_.assign(data, data + length);
        write_history_.push_back(last_write_);
        return length;
    }

    size_t read(uint8_t* data, size_t length) override {
        normalizeQueuedResponses();
        if (fail_next_read_) {
            fail_next_read_ = false;
            connected_ = false;
            return 0;
        }
        if (queued_responses_.empty()) {
            return 0;
        }
        size_t avail = queued_responses_.front().size() - read_offset_;
        size_t to_read = (length < avail) ? length : avail;
        memcpy(data, queued_responses_.front().data() + read_offset_, to_read);
        read_offset_ += to_read;
        normalizeQueuedResponses();
        return to_read;
    }

    size_t available() override {
        normalizeQueuedResponses();
        if (queued_responses_.empty()) {
            return 0;
        }
        return queued_responses_.front().size() - read_offset_;
    }

    void queueConnectResult(bool result) {
        connect_results_.push_back(result);
    }

    void queueResponse(const std::vector<uint8_t>& response) {
        normalizeQueuedResponses();
        queued_responses_.push_back(response);
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

    const std::vector<std::vector<uint8_t>>& writeHistory() const {
        return write_history_;
    }

  private:
    void normalizeQueuedResponses() {
        while (!queued_responses_.empty() && read_offset_ >= queued_responses_.front().size()) {
            queued_responses_.pop_front();
            read_offset_ = 0;
        }
    }

    bool connected_ = true;
    std::deque<bool> connect_results_;
    std::vector<uint8_t> last_write_;
    std::vector<std::vector<uint8_t>> write_history_;
    std::deque<std::vector<uint8_t>> queued_responses_;
    size_t read_offset_ = 0;
    bool fail_next_write_ = false;
    bool fail_next_read_ = false;
};

struct DirectFunctionCase {
    const char* name;
    slmp::DeviceCode code;
    uint32_t number;
    bool bit_access;
    bool supported;
};

const DirectFunctionCase kDirectFunctionCases[] = {
    {"SM", slmp::DeviceCode::SM, 100, true, true},
    {"SD", slmp::DeviceCode::SD, 100, false, true},
    {"X", slmp::DeviceCode::X, 0x10, true, true},
    {"Y", slmp::DeviceCode::Y, 0x10, true, true},
    {"M", slmp::DeviceCode::M, 100, true, true},
    {"L", slmp::DeviceCode::L, 100, true, true},
    {"F", slmp::DeviceCode::F, 100, true, true},
    {"V", slmp::DeviceCode::V, 100, true, true},
    {"B", slmp::DeviceCode::B, 0x100, true, true},
    {"D", slmp::DeviceCode::D, 100, false, true},
    {"W", slmp::DeviceCode::W, 0x100, false, true},
    {"TS", slmp::DeviceCode::TS, 100, true, true},
    {"TC", slmp::DeviceCode::TC, 100, true, true},
    {"TN", slmp::DeviceCode::TN, 100, false, true},
    {"LTS", slmp::DeviceCode::LTS, 100, true, false},
    {"LTC", slmp::DeviceCode::LTC, 100, true, false},
    {"LTN", slmp::DeviceCode::LTN, 100, false, false},
    {"STS", slmp::DeviceCode::STS, 100, true, true},
    {"STC", slmp::DeviceCode::STC, 100, true, true},
    {"STN", slmp::DeviceCode::STN, 100, false, true},
    {"LSTS", slmp::DeviceCode::LSTS, 100, true, false},
    {"LSTC", slmp::DeviceCode::LSTC, 100, true, false},
    {"LSTN", slmp::DeviceCode::LSTN, 100, false, false},
    {"CS", slmp::DeviceCode::CS, 100, true, true},
    {"CC", slmp::DeviceCode::CC, 100, true, true},
    {"CN", slmp::DeviceCode::CN, 100, false, true},
    {"LCS", slmp::DeviceCode::LCS, 100, true, false},
    {"LCC", slmp::DeviceCode::LCC, 100, true, false},
    {"LCN", slmp::DeviceCode::LCN, 100, false, false},
    {"SB", slmp::DeviceCode::SB, 0x100, true, true},
    {"SW", slmp::DeviceCode::SW, 0x100, false, true},
    {"S", slmp::DeviceCode::S, 100, true, false},
    {"DX", slmp::DeviceCode::DX, 0x10, true, true},
    {"DY", slmp::DeviceCode::DY, 0x10, true, true},
    {"Z", slmp::DeviceCode::Z, 100, false, false},
    {"LZ", slmp::DeviceCode::LZ, 100, false, false},
    {"R", slmp::DeviceCode::R, 200, false, true},
    {"ZR", slmp::DeviceCode::ZR, 300, false, true},
    {"RD", slmp::DeviceCode::RD, 100, false, false},
    {"G", slmp::DeviceCode::G, 100, false, false},
    {"HG", slmp::DeviceCode::HG, 100, false, false},
};

std::vector<uint8_t> makeGenericRequest(uint16_t command, uint16_t subcommand) {
    return {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x10, 0x00,
        static_cast<uint8_t>(command & 0xFFU), static_cast<uint8_t>((command >> 8) & 0xFFU),
        static_cast<uint8_t>(subcommand & 0xFFU), static_cast<uint8_t>((subcommand >> 8) & 0xFFU)
    };
}

void assertDirectRequestHeader(
    const std::vector<uint8_t>& request,
    uint16_t command,
    uint16_t subcommand,
    const slmp::DeviceAddress& device,
    uint16_t expected_points = 1U
) {
    assert(readLe16(request.data() + 15) == command);
    assert(readLe16(request.data() + 17) == subcommand);
    assert(readLe24(request.data() + 19) == device.number);
    assert(readLe16(request.data() + 23) == static_cast<uint16_t>(device.code));
    assert(readLe16(request.data() + 25) == expected_points);
}

void testReadWordsAndFrames() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x34, 0x12, 0x78, 0x56}));

    uint16_t words[2] = {};
    assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::Ok);
    assert(words[0] == 0x1234U);
    assert(words[1] == 0x5678U);
    assert(plc.lastRequestFrameLength() == 27U);
    assert(plc.lastResponseFrameLength() == 19U);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0401U);
    assert(readLe16(transport.lastWrite().data() + 17) == 0x0002U);
    assert(plc.lastRequestFrame() == tx_buffer);
    assert(plc.lastResponseFrame() == rx_buffer);

    char hex[64] = {};
    assert(slmp::formatHexBytes(transport.lastWrite().data(), 4, hex, sizeof(hex)) == 11U);
    assert(std::string(hex) == "54 00 00 00");
}

void testAllDirectDeviceFamilies() {
    for (const DirectFunctionCase& test_case : kDirectFunctionCases) {
        const slmp::DeviceAddress device = {test_case.code, test_case.number};

        {
            MockTransport transport;
            uint8_t tx_buffer[128] = {};
            uint8_t rx_buffer[128] = {};
            slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

            if (test_case.bit_access) {
                transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0003), 0x0000, {0x10}));
                bool value = false;
                const slmp::Error error = plc.readOneBit(device, value);
                if (!test_case.supported) {
                    assert(error == slmp::Error::UnsupportedDevice);
                    assert(transport.lastWrite().empty());
                } else {
                    assert(error == slmp::Error::Ok);
                    assert(value);
                    assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0003, device);
                }
            } else {
                transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x34, 0x12}));
                uint16_t value = 0;
                const slmp::Error error = plc.readOneWord(device, value);
                if (!test_case.supported) {
                    assert(error == slmp::Error::UnsupportedDevice);
                    assert(transport.lastWrite().empty());
                } else {
                    assert(error == slmp::Error::Ok);
                    assert(value == 0x1234U);
                    assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0002, device);
                }
            }
        }

        {
            MockTransport transport;
            uint8_t tx_buffer[128] = {};
            uint8_t rx_buffer[128] = {};
            slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

            if (test_case.bit_access) {
                transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0003), 0x0000, {}));
                const slmp::Error error = plc.writeOneBit(device, true);
                if (!test_case.supported) {
                    assert(error == slmp::Error::UnsupportedDevice);
                    assert(transport.lastWrite().empty());
                } else {
                    assert(error == slmp::Error::Ok);
                    assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0003, device);
                    assert(transport.lastWrite()[27] == 0x10U);
                }
            } else {
                transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0002), 0x0000, {}));
                const slmp::Error error = plc.writeOneWord(device, 0x1234U);
                if (!test_case.supported) {
                    assert(error == slmp::Error::UnsupportedDevice);
                    assert(transport.lastWrite().empty());
                } else {
                    assert(error == slmp::Error::Ok);
                    assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0002, device);
                    assert(readLe16(transport.lastWrite().data() + 27) == 0x1234U);
                }
            }
        }
    }
}

void testDWordAndOneShotHelpers() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0xC8, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x78, 0x56, 0x34, 0x12}));

    uint32_t dword = 0;
    assert(plc.readOneDWord(slmp::dev::D(slmp::dev::dec(200)), dword) == slmp::Error::Ok);
    assert(dword == 0x12345678UL);

    std::vector<uint8_t> second_request = provisional_request;
    second_request[2] = 0x01;
    second_request[15] = 0x01;
    second_request[16] = 0x14;
    second_request[17] = 0x03;
    second_request[18] = 0x00;
    transport.queueResponse(makeResponse(second_request, 0x0000, {}));
    assert(plc.writeOneBit(slmp::dev::M(slmp::dev::dec(101)), true) == slmp::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1401U);
    assert(readLe16(transport.lastWrite().data() + 17) == 0x0003U);
}

void testWriteDWordsAndRandomWords() {
    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0002), 0x0000, {}));
        const slmp::DeviceAddress device = slmp::dev::D(slmp::dev::dec(200));
        assert(plc.writeOneDWord(device, 0x12345678UL) == slmp::Error::Ok);
        assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0002, device, 2U);
        assert(readLe32(transport.lastWrite().data() + 27) == 0x12345678UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0002), 0x0000, {}));
        const uint32_t values[] = {0x89ABCDEFUL, 0x01234567UL};
        const slmp::DeviceAddress device = slmp::dev::D(slmp::dev::dec(300));
        assert(plc.writeDWords(device, values, 2) == slmp::Error::Ok);
        assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0002, device, 4U);
        assert(readLe32(transport.lastWrite().data() + 27) == values[0]);
        assert(readLe32(transport.lastWrite().data() + 31) == values[1]);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1402, 0x0002), 0x0000, {}));
        const slmp::DeviceAddress word_devices[] = {
            slmp::dev::D(slmp::dev::dec(120)),
        };
        const uint16_t word_values[] = {0x1111U};
        const slmp::DeviceAddress dword_devices[] = {
            slmp::dev::ZR(slmp::dev::dec(300)),
        };
        const uint32_t dword_values[] = {0x12345678UL};
        assert(plc.writeRandomWords(word_devices, word_values, 1, dword_devices, dword_values, 1) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1402U);
        assert(readLe16(transport.lastWrite().data() + 17) == 0x0002U);
        assert(transport.lastWrite()[19] == 1U);
        assert(transport.lastWrite()[20] == 1U);
        assert(readLe24(transport.lastWrite().data() + 21) == 120U);
        assert(readLe16(transport.lastWrite().data() + 25) == static_cast<uint16_t>(slmp::DeviceCode::D));
        assert(readLe16(transport.lastWrite().data() + 27) == word_values[0]);
        assert(readLe24(transport.lastWrite().data() + 29) == 300U);
        assert(readLe16(transport.lastWrite().data() + 33) == static_cast<uint16_t>(slmp::DeviceCode::ZR));
        assert(readLe32(transport.lastWrite().data() + 35) == dword_values[0]);
    }
}

void testFloat32Helpers() {
    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> provisional_request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x02, 0x00, 0xC8, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
        };
        transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x00, 0x00, 0xC0, 0x3F}));

        float value = 0.0f;
        assert(plc.readOneFloat32(slmp::dev::D(slmp::dev::dec(200)), value) == slmp::Error::Ok);
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        assert(bits == 0x3FC00000UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0002), 0x0000, {}));
        const slmp::DeviceAddress device = slmp::dev::D(slmp::dev::dec(200));
        assert(plc.writeOneFloat32(device, 1.5f) == slmp::Error::Ok);
        assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0002, device, 2U);
        assert(readLe32(transport.lastWrite().data() + 27) == 0x3FC00000UL);
    }
}

void testRandomAndBlock() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    std::vector<uint8_t> random_payload = {0x11, 0x11, 0x22, 0x22};
    appendLe32(random_payload, 0x12345678UL);
    const std::vector<uint8_t> serial0_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x14, 0x00, 0x10, 0x00,
        0x03, 0x04, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(serial0_request, 0x0000, random_payload));

    const slmp::DeviceAddress random_words[] = {
        slmp::dev::D(slmp::dev::dec(100)),
        slmp::dev::D(slmp::dev::dec(101)),
    };
    const slmp::DeviceAddress random_dwords[] = {
        slmp::dev::D(slmp::dev::dec(200)),
    };
    uint16_t word_values[2] = {};
    uint32_t dword_values[1] = {};
    assert(plc.readRandom(random_words, 2, word_values, 2, random_dwords, 1, dword_values, 1) == slmp::Error::Ok);
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

    const slmp::DeviceBlockRead word_blocks[] = {
        slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2),
    };
    const slmp::DeviceBlockRead bit_blocks[] = {
        slmp::dev::blockRead(slmp::dev::M(slmp::dev::dec(200)), 1),
    };
    uint16_t block_words[2] = {};
    uint16_t block_bits[1] = {};
    assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_words, 2, block_bits, 1) == slmp::Error::Ok);
    assert(block_words[0] == 0x1234U);
    assert(block_words[1] == 0x5678U);
    assert(block_bits[0] == 0x0005U);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0406U);
}

void testTargetAndMonitoringTimerHeaders() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    slmp::TargetAddress target = {};
    target.network = 0x12;
    target.station = 0x34;
    target.module_io = 0x0567;
    target.multidrop = 0x89;
    plc.setTarget(target);
    plc.setMonitoringTimer(0x4321U);
    plc.setTimeoutMs(987U);

    assert(plc.target().network == target.network);
    assert(plc.target().station == target.station);
    assert(plc.target().module_io == target.module_io);
    assert(plc.target().multidrop == target.multidrop);
    assert(plc.monitoringTimer() == 0x4321U);
    assert(plc.timeoutMs() == 987U);

    transport.queueResponse(makeResponse(makeGenericRequest(0x0101, 0x0000), 0x0000, {
        'Q', '0', '3', 'U', 'D', 'V', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x34, 0x12
    }));

    slmp::TypeNameInfo type_name = {};
    assert(plc.readTypeName(type_name) == slmp::Error::Ok);
    assert(transport.lastWrite()[6] == target.network);
    assert(transport.lastWrite()[7] == target.station);
    assert(readLe16(transport.lastWrite().data() + 8) == target.module_io);
    assert(transport.lastWrite()[10] == target.multidrop);
    assert(readLe16(transport.lastWrite().data() + 13) == 0x4321U);
}

void testProfileRecommendation() {
    {
        slmp::TypeNameInfo info = {};
        std::memcpy(info.model, "Q26UDEHCPU", 11);
        info.model[11] = '\0';
        info.model_code = 0x026CU;
        info.has_model_code = true;
        const slmp::ProfileRecommendation recommendation = slmp::recommendProfile(info);
        assert(recommendation.confident);
        assert(recommendation.profile_class == slmp::ProfileClass::LegacyQL);
        assert(recommendation.frame_type == slmp::FrameType::Frame3E);
        assert(recommendation.compatibility_mode == slmp::CompatibilityMode::Legacy);
        assert(std::string(slmp::profileClassString(recommendation.profile_class)) == "legacy_ql");
    }

    {
        const slmp::ProfileRecommendation recommendation = slmp::recommendProfile("R08CPU", 0x4801U, true);
        assert(recommendation.confident);
        assert(recommendation.profile_class == slmp::ProfileClass::ModernIQR);
        assert(recommendation.frame_type == slmp::FrameType::Frame4E);
        assert(recommendation.compatibility_mode == slmp::CompatibilityMode::iQR);
    }

    {
        const slmp::ProfileRecommendation recommendation = slmp::recommendProfile("L26CPU-BT", 0x0542U, true);
        assert(recommendation.confident);
        assert(recommendation.profile_class == slmp::ProfileClass::LegacyQL);
    }

    {
        const slmp::ProfileRecommendation recommendation = slmp::recommendProfile("FX5UCPU", 0x0000U, false);
        assert(recommendation.confident);
        assert(recommendation.profile_class == slmp::ProfileClass::ModernIQR);
    }

    {
        const slmp::ProfileRecommendation recommendation = slmp::recommendProfile("UnknownTarget", 0x0000U, false);
        assert(!recommendation.confident);
        assert(recommendation.profile_class == slmp::ProfileClass::Unknown);
        assert(recommendation.frame_type == slmp::FrameType::Frame4E);
        assert(recommendation.compatibility_mode == slmp::CompatibilityMode::iQR);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
        plc.setFrameType(slmp::FrameType::Frame4E);
        plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);
        slmp::applyProfileRecommendation(
            plc,
            slmp::recommendProfile("Q06UDVCPU", 0x0000U, false)
        );
        assert(plc.frameType() == slmp::FrameType::Frame3E);
        assert(plc.compatibilityMode() == slmp::CompatibilityMode::Legacy);
    }
}

void testPlcErrorAndStrings() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x01, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x4031, {}));

    uint16_t value = 0;
    assert(plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), value) == slmp::Error::PlcError);
    assert(plc.lastEndCode() == 0x4031U);
    assert(std::string(slmp::errorString(plc.lastError())) == "plc_error");
    assert(std::string(slmp::endCodeString(plc.lastEndCode())) == "range_or_allocation_mismatch");
    assert(std::string(slmp::endCodeString(0x414AU)) == "target_or_write_path_rejected");
    assert(std::string(slmp::endCodeString(0xC056U)) == "request_format_or_combination_rejected");
    assert(std::string(slmp::endCodeString(0xC201U)) == "password_lock_or_authentication_required");
    assert(std::string(slmp::endCodeString(0xC810U)) == "invalid_password");
    assert(std::string(slmp::endCodeString(0xDEADU)) == "unknown_plc_end_code");
}

void testPasswordAndWriteBlock() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> unlock_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0F, 0x00, 0x10, 0x00,
        0x30, 0x16, 0x00, 0x00
    };
    transport.queueResponse(makeResponse(unlock_request, 0x0000, {}));
    assert(plc.remotePasswordUnlock("secret1") == slmp::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1630U);
    assert(readLe16(transport.lastWrite().data() + 19) == 7U);
    assert(std::memcmp(transport.lastWrite().data() + 21, "secret1", 7) == 0);

    std::vector<uint8_t> block_request = unlock_request;
    block_request[2] = 0x01;
    block_request[15] = 0x06;
    block_request[16] = 0x14;
    transport.queueResponse(makeResponse(block_request, 0x0000, {}));

    const uint16_t block_values[] = {0x1234, 0x5678};
    const slmp::DeviceBlockWrite word_blocks[] = {
        slmp::dev::blockWrite(slmp::dev::D(slmp::dev::dec(400)), block_values, 2),
    };
    assert(plc.writeBlock(word_blocks, 1, nullptr, 0) == slmp::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1406U);
}

void testRemoteControl() {
    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1001, 0x0000), 0x0000, {}));
        assert(plc.remoteRun(false, 2U) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1001U);
        assert(readLe16(transport.lastWrite().data() + 19) == 0x0001U);
        assert(readLe16(transport.lastWrite().data() + 21) == 0x0002U);
        assert(plc.remoteRun(false, 3U) == slmp::Error::InvalidArgument);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1002, 0x0000), 0x0000, {}));
        assert(plc.remoteStop() == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1002U);
        assert(readLe16(transport.lastWrite().data() + 19) == 0x0001U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1003, 0x0000), 0x0000, {}));
        assert(plc.remotePause(true) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1003U);
        assert(readLe16(transport.lastWrite().data() + 19) == 0x0003U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1005, 0x0000), 0x0000, {}));
        assert(plc.remoteLatchClear() == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1005U);
        assert(readLe16(transport.lastWrite().data() + 19) == 0x0001U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        assert(plc.remoteReset() == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1006U);
        assert(readLe16(transport.lastWrite().data() + 17) == 0x0000U);
        assert(plc.remoteReset(0x0002U, true) == slmp::Error::InvalidArgument);
    }
}

void testSelfTestAndClearError() {
    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0619, 0x0000), 0x0000, {0x05, 0x00, 'A', 'B', 'C', 'D', 'E'}));
        const uint8_t input[] = {'A', 'B', 'C', 'D', 'E'};
        uint8_t output[8] = {};
        size_t output_length = 0;
        assert(plc.selfTestLoopback(input, sizeof(input), output, sizeof(output), output_length) == slmp::Error::Ok);
        assert(output_length == 5U);
        assert(std::memcmp(output, input, sizeof(input)) == 0);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x0619U);
        assert(readLe16(transport.lastWrite().data() + 19) == 0x0005U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1617, 0x0000), 0x0000, {}));
        assert(plc.clearError() == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1617U);
    }
}

void testWriteBlockOptions() {
    const uint16_t word_values[] = {0x1234U, 0x5678U};
    const uint16_t bit_values[] = {0x0005U};
    const slmp::DeviceBlockWrite word_blocks[] = {
        slmp::dev::blockWrite(slmp::dev::D(slmp::dev::dec(400)), word_values, 2),
    };
    const slmp::DeviceBlockWrite bit_blocks[] = {
        slmp::dev::blockWrite(slmp::dev::M(slmp::dev::dec(240)), bit_values, 1),
    };

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> response_request = makeGenericRequest(0x1406, 0x0002);
        response_request[2] = 0x00;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));
        response_request[2] = 0x01;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));

        slmp::BlockWriteOptions options = {};
        options.split_mixed_blocks = true;
        assert(plc.writeBlock(word_blocks, 1, bit_blocks, 1, options) == slmp::Error::Ok);
        assert(transport.writeHistory().size() == 2U);
        assert(readLe16(transport.writeHistory()[0].data() + 15) == 0x1406U);
        assert(transport.writeHistory()[0][19] == 1U);
        assert(transport.writeHistory()[0][20] == 0U);
        assert(transport.writeHistory()[1][19] == 0U);
        assert(transport.writeHistory()[1][20] == 1U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> response_request = makeGenericRequest(0x1406, 0x0002);
        response_request[2] = 0x00;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0xC05B, {}));
        response_request[2] = 0x01;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));
        response_request[2] = 0x02;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));

        slmp::BlockWriteOptions options = {};
        options.retry_mixed_on_error = true;
        assert(plc.writeBlock(word_blocks, 1, bit_blocks, 1, options) == slmp::Error::Ok);
        assert(plc.lastEndCode() == 0x0000U);
        assert(transport.writeHistory().size() == 3U);
        assert(transport.writeHistory()[0][19] == 1U);
        assert(transport.writeHistory()[0][20] == 1U);
        assert(transport.writeHistory()[1][19] == 1U);
        assert(transport.writeHistory()[1][20] == 0U);
        assert(transport.writeHistory()[2][19] == 0U);
        assert(transport.writeHistory()[2][20] == 1U);
    }
}

void testAsyncWriteBlockOptions() {
    const uint16_t word_values[] = {0x1234U, 0x5678U};
    const uint16_t bit_values[] = {0x0005U};
    const slmp::DeviceBlockWrite word_blocks[] = {
        slmp::dev::blockWrite(slmp::dev::D(slmp::dev::dec(400)), word_values, 2),
    };
    const slmp::DeviceBlockWrite bit_blocks[] = {
        slmp::dev::blockWrite(slmp::dev::M(slmp::dev::dec(240)), bit_values, 1),
    };
    const uint32_t now = 1000U;

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> response_request = makeGenericRequest(0x1406, 0x0002);
        response_request[2] = 0x00;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));
        response_request[2] = 0x01;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));

        slmp::BlockWriteOptions options = {};
        options.split_mixed_blocks = true;
        assert(plc.beginWriteBlock(word_blocks, 1, bit_blocks, 1, options, now) == slmp::Error::Ok);
        driveAsyncUntilIdle(plc, now);
        assert(plc.lastError() == slmp::Error::Ok);
        assert(transport.writeHistory().size() == 2U);
        assert(transport.writeHistory()[0][19] == 1U);
        assert(transport.writeHistory()[0][20] == 0U);
        assert(transport.writeHistory()[1][19] == 0U);
        assert(transport.writeHistory()[1][20] == 1U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> response_request = makeGenericRequest(0x1406, 0x0002);
        response_request[2] = 0x00;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0xC05B, {}));
        response_request[2] = 0x01;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));
        response_request[2] = 0x02;
        response_request[3] = 0x00;
        transport.queueResponse(makeResponse(response_request, 0x0000, {}));

        slmp::BlockWriteOptions options = {};
        options.retry_mixed_on_error = true;
        assert(plc.beginWriteBlock(word_blocks, 1, bit_blocks, 1, options, now) == slmp::Error::Ok);
        driveAsyncUntilIdle(plc, now);
        assert(plc.lastError() == slmp::Error::Ok);
        assert(plc.lastEndCode() == 0x0000U);
        assert(transport.writeHistory().size() == 3U);
        assert(transport.writeHistory()[0][19] == 1U);
        assert(transport.writeHistory()[0][20] == 1U);
        assert(transport.writeHistory()[1][19] == 1U);
        assert(transport.writeHistory()[1][20] == 0U);
        assert(transport.writeHistory()[2][19] == 0U);
        assert(transport.writeHistory()[2][20] == 1U);
    }
}

void testAsyncRemoteControl() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    const uint32_t now = 1000U;

    transport.queueResponse(makeResponse(makeGenericRequest(0x1001, 0x0000), 0x0000, {}));
    assert(plc.beginRemoteRun(true, 1U, now) == slmp::Error::Ok);
    driveAsyncUntilIdle(plc, now);
    assert(plc.lastError() == slmp::Error::Ok);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x1001U);
    assert(readLe16(transport.lastWrite().data() + 19) == 0x0003U);
    assert(readLe16(transport.lastWrite().data() + 21) == 0x0001U);

    MockTransport reset_transport;
    uint8_t reset_tx_buffer[256] = {};
    uint8_t reset_rx_buffer[256] = {};
    slmp::SlmpClient reset_plc(reset_transport, reset_tx_buffer, sizeof(reset_tx_buffer), reset_rx_buffer, sizeof(reset_rx_buffer));
    assert(reset_plc.beginRemoteReset(0x0000U, false, now) == slmp::Error::Ok);
    driveAsyncUntilIdle(reset_plc, now);
    assert(reset_plc.lastError() == slmp::Error::Ok);
    assert(readLe16(reset_transport.lastWrite().data() + 15) == 0x1006U);
    assert(readLe16(reset_transport.lastWrite().data() + 17) == 0x0000U);
}

void testAsyncSelfTestAndClearError() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
    const uint32_t now = 1000U;

    transport.queueResponse(makeResponse(makeGenericRequest(0x0619, 0x0000), 0x0000, {0x05, 0x00, 'A', 'B', 'C', 'D', 'E'}));
    const uint8_t input[] = {'A', 'B', 'C', 'D', 'E'};
    uint8_t output[8] = {};
    size_t output_length = 0;
    assert(plc.beginSelfTestLoopback(input, sizeof(input), output, sizeof(output), &output_length, now) == slmp::Error::Ok);
    driveAsyncUntilIdle(plc, now);
    assert(plc.lastError() == slmp::Error::Ok);
    assert(output_length == 5U);
    assert(std::memcmp(output, input, sizeof(input)) == 0);

    MockTransport clear_transport;
    uint8_t clear_tx_buffer[256] = {};
    uint8_t clear_rx_buffer[256] = {};
    slmp::SlmpClient clear_plc(clear_transport, clear_tx_buffer, sizeof(clear_tx_buffer), clear_rx_buffer, sizeof(clear_rx_buffer));
    clear_transport.queueResponse(makeResponse(makeGenericRequest(0x1617, 0x0000), 0x0000, {}));
    assert(clear_plc.beginClearError(now) == slmp::Error::Ok);
    driveAsyncUntilIdle(clear_plc, now);
    assert(clear_plc.lastError() == slmp::Error::Ok);
    assert(readLe16(clear_transport.lastWrite().data() + 15) == 0x1617U);
}

void testValidationAndBoundaryFailures() {
    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        assert(plc.writeRandomWords(nullptr, nullptr, 0, nullptr, nullptr, 0) == slmp::Error::InvalidArgument);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[20] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceAddress word_devices[] = {slmp::dev::D(slmp::dev::dec(120))};
        const uint16_t word_values[] = {0x1111U};
        const slmp::DeviceAddress dword_devices[] = {slmp::dev::D(slmp::dev::dec(200))};
        const uint32_t dword_values[] = {0x12345678UL};
        assert(plc.writeRandomWords(word_devices, word_values, 1, dword_devices, dword_values, 1) == slmp::Error::BufferTooSmall);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        uint32_t dwords[1] = {};
        assert(plc.readDWords(slmp::dev::D(slmp::dev::dec(200)), 0, dwords, 1) == slmp::Error::InvalidArgument);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceBlockRead word_blocks[] = {
            slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2),
        };
        uint16_t too_small_word_values[1] = {};
        assert(plc.readBlock(word_blocks, 1, nullptr, 0, too_small_word_values, 1, nullptr, 0) == slmp::Error::InvalidArgument);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceBlockWrite invalid_word_blocks[] = {
            {{slmp::DeviceCode::D, 400}, nullptr, 2},
        };
        assert(plc.writeBlock(invalid_word_blocks, 1, nullptr, 0) == slmp::Error::InvalidArgument);
    }
}

void testTransportFailuresAndReconnectHelper() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    transport.close();
    transport.queueConnectResult(false);
    transport.queueConnectResult(true);

    slmp::ReconnectHelper reconnect(
        plc,
        "192.168.250.101",
        1025,
        slmp::ReconnectOptions{1000}
    );

    assert(!reconnect.ensureConnected(0));
    assert(!reconnect.consumeConnectedEdge());
    assert(!reconnect.ensureConnected(500));
    assert(reconnect.ensureConnected(1000));
    assert(reconnect.consumeConnectedEdge());
    assert(!reconnect.consumeConnectedEdge());

    transport.setFailNextRead();
    slmp::TypeNameInfo type_name = {};
    assert(plc.readTypeName(type_name) == slmp::Error::TransportError);
    assert(plc.lastError() == slmp::Error::TransportError);

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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(std::vector<uint8_t>(python_golden::kReadTypeNameRequest, python_golden::kReadTypeNameRequest + python_golden::size(python_golden::kReadTypeNameRequest)), 0x0000, {
            'Q', '0', '3', 'U', 'D', 'V', 'C', 'P', 'U', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x34, 0x12
        }));
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadWordsD1002Request,
                python_golden::kReadWordsD1002Request + python_golden::size(python_golden::kReadWordsD1002Request)
            ),
            0x0000,
            {0x34, 0x12, 0x78, 0x56}
        ));
        uint16_t words[2] = {};
        assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kWriteBitsM101TrueRequest,
                python_golden::kWriteBitsM101TrueRequest + python_golden::size(python_golden::kWriteBitsM101TrueRequest)
            ),
            0x0000,
            {}
        ));
        assert(plc.writeOneBit(slmp::dev::M(slmp::dev::dec(101)), true) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadRandomRequest,
                python_golden::kReadRandomRequest + python_golden::size(python_golden::kReadRandomRequest)
            ),
            0x0000,
            {0x11, 0x11, 0x22, 0x22, 0x78, 0x56, 0x34, 0x12}
        ));
        const slmp::DeviceAddress random_words[] = {
            slmp::dev::D(slmp::dev::dec(100)),
            slmp::dev::D(slmp::dev::dec(101)),
        };
        const slmp::DeviceAddress random_dwords[] = {
            slmp::dev::D(slmp::dev::dec(200)),
        };
        uint16_t word_values[2] = {};
        uint32_t dword_values[1] = {};
        assert(plc.readRandom(random_words, 2, word_values, 2, random_dwords, 1, dword_values, 1) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kWriteRandomBitsRequest,
                python_golden::kWriteRandomBitsRequest + python_golden::size(python_golden::kWriteRandomBitsRequest)
            ),
            0x0000,
            {}
        ));
        const slmp::DeviceAddress random_bits[] = {
            slmp::dev::M(slmp::dev::dec(100)),
            slmp::dev::Y(slmp::dev::hex(0x20)),
        };
        const bool bit_values[] = {true, false};
        assert(plc.writeRandomBits(random_bits, bit_values, 2) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kReadBlockRequest,
                python_golden::kReadBlockRequest + python_golden::size(python_golden::kReadBlockRequest)
            ),
            0x0000,
            {0x34, 0x12, 0x78, 0x56, 0x05, 0x00}
        ));
        const slmp::DeviceBlockRead word_blocks[] = {
            slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2),
        };
        const slmp::DeviceBlockRead bit_blocks[] = {
            slmp::dev::blockRead(slmp::dev::M(slmp::dev::dec(200)), 1),
        };
        uint16_t block_words[2] = {};
        uint16_t block_bits[1] = {};
        assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_words, 2, block_bits, 1) == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(
                python_golden::kRemoteUnlockRequest,
                python_golden::kRemoteUnlockRequest + python_golden::size(python_golden::kRemoteUnlockRequest)
            ),
            0x0000,
            {}
        ));
        assert(plc.remotePasswordUnlock("secret1") == slmp::Error::Ok);
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
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00
        });
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00
        });
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x00
        });
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[14] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse({
            0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x14, 0x00
        });
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::BufferTooSmall);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.setFailNextWrite();
        slmp::TypeNameInfo type_name = {};
        assert(plc.readTypeName(type_name) == slmp::Error::TransportError);
    }
}

void testPayloadValidationFailures() {
    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x02, 0x00
        };
        transport.queueResponse(makeResponse(request, 0x0000, {0x34, 0x12}));
        uint16_t words[2] = {};
        assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::ProtocolError);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x03, 0x00
        };
        transport.queueResponse(makeResponse(request, 0x0000, {}));
        bool value = false;
        assert(plc.readOneBit(slmp::dev::M(slmp::dev::dec(100)), value) == slmp::Error::ProtocolError);
    }
}

void testAsyncApi() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x34, 0x12, 0x78, 0x56}));

    uint16_t words[2] = {};
    uint32_t now = 1000;
    assert(plc.beginReadWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2, now) == slmp::Error::Ok);
    assert(plc.isBusy());

    // Progress state machine
    plc.update(now); // Sends
    assert(plc.isBusy());
    plc.update(now); // Receives prefix
    assert(plc.isBusy());
    plc.update(now); // Receives body
    assert(!plc.isBusy());

    assert(plc.lastError() == slmp::Error::Ok);
    assert(words[0] == 0x1234U);
    assert(words[1] == 0x5678U);
}

void testAsyncFloat32Api() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    const std::vector<uint8_t> provisional_request = {
        0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
        0x01, 0x04, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse(provisional_request, 0x0000, {0x00, 0x00, 0xC0, 0x3F}));

    float value = 0.0f;
    uint32_t now = 1000;
    assert(plc.beginReadFloat32s(slmp::dev::D(slmp::dev::dec(100)), 1, &value, 1, now) == slmp::Error::Ok);
    assert(plc.isBusy());
    plc.update(now);
    plc.update(now);
    plc.update(now);
    assert(!plc.isBusy());

    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    assert(bits == 0x3FC00000UL);
}

void testFrame3E() {
    MockTransport transport;
    uint8_t tx_buffer[128] = {};
    uint8_t rx_buffer[128] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    plc.setFrameType(slmp::FrameType::Frame3E);
    assert(plc.frameType() == slmp::FrameType::Frame3E);

    const std::vector<uint8_t> expected_request = {
        0x50, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00, 0x01, 0x04, 0x02, 0x00,
        0x64, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
    };
    transport.queueResponse(makeResponse3E(expected_request, 0x0000, {0x34, 0x12, 0x78, 0x56}));

    uint16_t words[2] = {};
    assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::Ok);
    assert(words[0] == 0x1234U);
    assert(words[1] == 0x5678U);
    assert(plc.lastRequestFrameLength() == 23U);
    assert(plc.lastResponseFrameLength() == 15U);
    assert(transport.lastWrite().size() == 23U);
    assert(transport.lastWrite()[0] == 0x50);
    assert(transport.lastWrite()[1] == 0x00);
    assert(transport.lastWrite()[2] == 0x00); // Network
    assert(transport.lastWrite()[3] == 0xFF); // Station
}

}  // namespace

int main() {
    testReadWordsAndFrames();
    testAsyncApi();
    testAsyncFloat32Api();
    testFrame3E();
    testAllDirectDeviceFamilies();
    testDWordAndOneShotHelpers();
    testFloat32Helpers();
    testWriteDWordsAndRandomWords();
    testRandomAndBlock();
    testTargetAndMonitoringTimerHeaders();
    testProfileRecommendation();
    testPlcErrorAndStrings();
    testPasswordAndWriteBlock();
    testRemoteControl();
    testSelfTestAndClearError();
    testWriteBlockOptions();
    testAsyncWriteBlockOptions();
    testValidationAndBoundaryFailures();
    testTransportFailuresAndReconnectHelper();
    testPythonCompatibilityGoldenFrames();
    testProtocolFailures();
    testPayloadValidationFailures();
    testAsyncRemoteControl();
    testAsyncSelfTestAndClearError();
    std::puts("slmp_minimal_tests: ok");
    return 0;
}
