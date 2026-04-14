#include <assert.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "slmp_high_level.h"
#include "slmp_minimal.h"
#include "slmp_utility.h"
#include "generated_shared_spec.h"

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

std::vector<uint8_t> makeWordPayload(const std::vector<uint16_t>& words) {
    std::vector<uint8_t> data;
    data.reserve(words.size() * 2U);
    for (size_t i = 0; i < words.size(); ++i) {
        appendLe16(data, words[i]);
    }
    return data;
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
    bool read_supported;
    bool write_supported;
};

const DirectFunctionCase kDirectFunctionCases[] = {
    {"SM", slmp::DeviceCode::SM, 100, true, true, true},
    {"SD", slmp::DeviceCode::SD, 100, false, true, true},
    {"X", slmp::DeviceCode::X, 0x10, true, true, true},
    {"Y", slmp::DeviceCode::Y, 0x10, true, true, true},
    {"M", slmp::DeviceCode::M, 100, true, true, true},
    {"L", slmp::DeviceCode::L, 100, true, true, true},
    {"F", slmp::DeviceCode::F, 100, true, true, true},
    {"V", slmp::DeviceCode::V, 100, true, true, true},
    {"B", slmp::DeviceCode::B, 0x100, true, true, true},
    {"D", slmp::DeviceCode::D, 100, false, true, true},
    {"W", slmp::DeviceCode::W, 0x100, false, true, true},
    {"TS", slmp::DeviceCode::TS, 100, true, true, true},
    {"TC", slmp::DeviceCode::TC, 100, true, true, true},
    {"TN", slmp::DeviceCode::TN, 100, false, true, true},
    {"LTS", slmp::DeviceCode::LTS, 100, true, false, true},
    {"LTC", slmp::DeviceCode::LTC, 100, true, false, true},
    {"LTN", slmp::DeviceCode::LTN, 100, false, false, true},
    {"STS", slmp::DeviceCode::STS, 100, true, true, true},
    {"STC", slmp::DeviceCode::STC, 100, true, true, true},
    {"STN", slmp::DeviceCode::STN, 100, false, true, true},
    {"LSTS", slmp::DeviceCode::LSTS, 100, true, false, true},
    {"LSTC", slmp::DeviceCode::LSTC, 100, true, false, true},
    {"LSTN", slmp::DeviceCode::LSTN, 100, false, false, true},
    {"CS", slmp::DeviceCode::CS, 100, true, true, true},
    {"CC", slmp::DeviceCode::CC, 100, true, true, true},
    {"CN", slmp::DeviceCode::CN, 100, false, true, true},
    {"LCS", slmp::DeviceCode::LCS, 100, true, true, true},
    {"LCC", slmp::DeviceCode::LCC, 100, true, true, true},
    {"LCN", slmp::DeviceCode::LCN, 100, false, true, true},
    {"SB", slmp::DeviceCode::SB, 0x100, true, true, true},
    {"SW", slmp::DeviceCode::SW, 0x100, false, true, true},
    {"DX", slmp::DeviceCode::DX, 0x10, true, true, true},
    {"DY", slmp::DeviceCode::DY, 0x10, true, true, true},
    {"Z", slmp::DeviceCode::Z, 100, false, true, true},
    {"LZ", slmp::DeviceCode::LZ, 100, false, true, true},
    {"R", slmp::DeviceCode::R, 200, false, true, true},
    {"ZR", slmp::DeviceCode::ZR, 300, false, true, true},
    {"RD", slmp::DeviceCode::RD, 100, false, true, true},
    {"G", slmp::DeviceCode::G, 100, false, false, false},
    {"HG", slmp::DeviceCode::HG, 100, false, false, false},
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
    const size_t header_size = (request.size() >= 2U && request[0] == 0x50U && request[1] == 0x00U) ? 15U : 19U;
    assert(readLe16(request.data() + header_size - 4U) == command);
    assert(readLe16(request.data() + header_size - 2U) == subcommand);
    const size_t payload_offset = header_size;
    const bool legacy_spec = (subcommand == 0x0000U || subcommand == 0x0001U);
    assert(request.size() >= payload_offset + (legacy_spec ? 6U : 8U));
    if (legacy_spec) {
        assert(readLe24(request.data() + payload_offset) == device.number);
        assert(static_cast<uint16_t>(request[payload_offset + 3U]) == static_cast<uint16_t>(device.code));
        assert(readLe16(request.data() + payload_offset + 4U) == expected_points);
        return;
    }

    assert(readLe32(request.data() + payload_offset) == device.number);
    assert(readLe16(request.data() + payload_offset + 4U) == static_cast<uint16_t>(device.code));
    assert(readLe16(request.data() + payload_offset + 6U) == expected_points);
}

const slmp::highlevel::DeviceRangeEntry* findDeviceRangeEntry(
    const slmp::highlevel::DeviceRangeCatalog& catalog,
    const char* device
) {
    for (size_t i = 0; i < catalog.entries.size(); ++i) {
        if (catalog.entries[i].device == device) return &catalog.entries[i];
    }
    return nullptr;
}

void testAdditionalDeviceHelpers() {
    const slmp::DeviceAddress z = slmp::dev::Z(slmp::dev::dec(10));
    assert(z.code == slmp::DeviceCode::Z);
    assert(z.number == 10U);

    const slmp::DeviceAddress lz = slmp::dev::LZ(slmp::dev::dec(11));
    assert(lz.code == slmp::DeviceCode::LZ);
    assert(lz.number == 11U);

    const slmp::DeviceAddress rd = slmp::dev::RD(slmp::dev::dec(12));
    assert(rd.code == slmp::DeviceCode::RD);
    assert(rd.number == 12U);

    const slmp::DeviceAddress ltn = slmp::dev::LTN(slmp::dev::dec(13));
    assert(ltn.code == slmp::DeviceCode::LTN);
    assert(ltn.number == 13U);

    const slmp::DeviceAddress lstn = slmp::dev::LSTN(slmp::dev::dec(14));
    assert(lstn.code == slmp::DeviceCode::LSTN);
    assert(lstn.number == 14U);

    const slmp::DeviceAddress lts = slmp::dev::LTS(slmp::dev::dec(15));
    assert(lts.code == slmp::DeviceCode::LTS);
    assert(lts.number == 15U);

    const slmp::DeviceAddress lsts = slmp::dev::LSTS(slmp::dev::dec(16));
    assert(lsts.code == slmp::DeviceCode::LSTS);
    assert(lsts.number == 16U);
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
                if (!test_case.read_supported) {
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
                if (!test_case.read_supported) {
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
                if (!test_case.write_supported) {
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
                if (!test_case.write_supported) {
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

void testUnsupportedLongFamilyCommandGuards() {
    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        bool bits[1] = {};
        assert(plc.readBits(slmp::dev::LTC(slmp::dev::dec(0)), 1, bits, 1) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        uint16_t words[2] = {};
        assert(plc.readWords(slmp::dev::LTN(slmp::dev::dec(0)), 2, words, 2) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {
            0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00
        }));
        uint16_t words[4] = {};
        assert(plc.readWords(slmp::dev::LTN(slmp::dev::dec(0)), 4, words, 4) == slmp::Error::Ok);
        assert(words[0] == 1U);
        assert(words[1] == 2U);
        assert(words[2] == 3U);
        assert(words[3] == 4U);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x0401U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        uint32_t dwords[1] = {};
        assert(plc.readDWords(slmp::dev::LTN(slmp::dev::dec(0)), 1, dwords, 1) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceAddress word_devices[] = {slmp::dev::LCS(slmp::dev::dec(10))};
        uint16_t word_values[1] = {};
        assert(plc.readRandom(word_devices, 1, word_values, 1, nullptr, 0, nullptr, 0) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceBlockRead bit_blocks[] = {
            slmp::dev::blockRead(slmp::dev::LCS(slmp::dev::dec(10)), 1),
        };
        uint16_t bit_values[1] = {};
        assert(plc.readBlock(nullptr, 0, bit_blocks, 1, nullptr, 0, bit_values, 1) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const uint16_t values[] = {1U};
        const slmp::DeviceBlockWrite bit_blocks[] = {
            slmp::dev::blockWrite(slmp::dev::LCC(slmp::dev::dec(10)), values, 1),
        };
        assert(plc.writeBlock(nullptr, 0, bit_blocks, 1) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const slmp::DeviceAddress word_devices[] = {slmp::dev::LCS(slmp::dev::dec(10))};
        assert(plc.registerMonitorDevices(word_devices, 1, nullptr, 0) == slmp::Error::UnsupportedDevice);
        assert(transport.lastWrite().empty());
    }
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
        "192.168.250.100",
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

void testSharedDeviceVectors() {
    for (const auto& vec : shared_spec::device_vectors::kCases) {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
        plc.setCompatibilityMode(vec.mode);

        const uint16_t subcommand = vec.bit_access
            ? (vec.mode == slmp::CompatibilityMode::Legacy ? 0x0001U : 0x0003U)
            : (vec.mode == slmp::CompatibilityMode::Legacy ? 0x0000U : 0x0002U);
        transport.queueResponse(makeResponse(makeGenericRequest(0x0401U, subcommand), 0x0000, vec.bit_access
            ? std::vector<uint8_t>{0x10}
            : std::vector<uint8_t>{0x34, 0x12}));

        const slmp::DeviceAddress device{vec.code, vec.number};
        if (vec.bit_access) {
            bool value = false;
            assert(plc.readOneBit(device, value) == slmp::Error::Ok);
        } else {
            uint16_t value = 0U;
            assert(plc.readOneWord(device, value) == slmp::Error::Ok);
        }

        assert(transport.lastWrite().size() >= 19U + vec.expected_size);
        assertBytesEqual(
            std::vector<uint8_t>(transport.lastWrite().begin() + 19, transport.lastWrite().begin() + 19 + vec.expected_size),
            vec.expected,
            vec.expected_size);
    }
}

void testSharedCppAddressVectors() {
    for (const auto& normalize_case : shared_spec::normalize_cases::kCases) {
        char normalized[32] = {};
        assert(slmp::highlevel::normalizeAddress(normalize_case.input, normalized, sizeof(normalized)) == slmp::Error::Ok);
        assert(std::string(normalized) == normalize_case.expected);
    }

    for (const auto& parse_case : shared_spec::cpp_parse_cases::kCases) {
        slmp::highlevel::AddressSpec spec{};
        const slmp::Error err = slmp::highlevel::parseAddressSpec(parse_case.input, spec);
        assert(err == parse_case.expected_error);
        if (!parse_case.has_value_expectation) {
            continue;
        }

        assert(spec.device.code == parse_case.code);
        assert(spec.device.number == parse_case.number);
        assert(spec.type == parse_case.value_type);
        assert(spec.explicit_type == parse_case.explicit_type);
        assert(spec.bit_index == parse_case.bit_index);
    }
}

void testSharedGoldenFrames() {
    for (const auto& frame : shared_spec::frame_vectors::kCases) {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(
            std::vector<uint8_t>(frame.request, frame.request + frame.request_size),
            0x0000,
            std::vector<uint8_t>(frame.response_data, frame.response_data + frame.response_data_size)
        ));

        if (std::strcmp(frame.operation, "read_type_name") == 0) {
            slmp::TypeNameInfo type_name = {};
            assert(plc.readTypeName(type_name) == slmp::Error::Ok);
        } else if (std::strcmp(frame.operation, "read_words") == 0) {
            uint16_t words[2] = {};
            assert(plc.readWords(slmp::dev::D(slmp::dev::dec(100)), 2, words, 2) == slmp::Error::Ok);
        } else if (std::strcmp(frame.operation, "write_bits") == 0) {
            assert(plc.writeOneBit(slmp::dev::M(slmp::dev::dec(101)), true) == slmp::Error::Ok);
        } else if (std::strcmp(frame.operation, "read_random") == 0) {
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
        } else if (std::strcmp(frame.operation, "write_random_bits") == 0) {
            const slmp::DeviceAddress random_bits[] = {
                slmp::dev::M(slmp::dev::dec(100)),
                slmp::dev::Y(slmp::dev::hex(0x20)),
            };
            const bool bit_values[] = {true, false};
            assert(plc.writeRandomBits(random_bits, bit_values, 2) == slmp::Error::Ok);
        } else if (std::strcmp(frame.operation, "read_block") == 0) {
            const slmp::DeviceBlockRead word_blocks[] = {
                slmp::dev::blockRead(slmp::dev::D(slmp::dev::dec(300)), 2),
            };
            const slmp::DeviceBlockRead bit_blocks[] = {
                slmp::dev::blockRead(slmp::dev::M(slmp::dev::dec(200)), 1),
            };
            uint16_t block_words[2] = {};
            uint16_t block_bits[1] = {};
            assert(plc.readBlock(word_blocks, 1, bit_blocks, 1, block_words, 2, block_bits, 1) == slmp::Error::Ok);
        } else if (std::strcmp(frame.operation, "remote_password_unlock") == 0) {
            assert(plc.remotePasswordUnlock("secret1") == slmp::Error::Ok);
        } else {
            assert(false);
        }

        assertBytesEqual(
            transport.lastWrite(),
            frame.request,
            frame.request_size
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

void testHighLevelParserAndTypedHelpers() {
    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("D100", spec) == slmp::Error::Ok);
        assert(spec.device.code == slmp::DeviceCode::D);
        assert(spec.device.number == 100U);
        assert(spec.type == slmp::highlevel::ValueType::U16);
        assert(spec.bit_index < 0);
    }

    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("M1000", spec) == slmp::Error::Ok);
        assert(spec.device.code == slmp::DeviceCode::M);
        assert(spec.device.number == 1000U);
        assert(spec.type == slmp::highlevel::ValueType::Bit);
    }

    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("D200:F", spec) == slmp::Error::Ok);
        assert(spec.device.code == slmp::DeviceCode::D);
        assert(spec.device.number == 200U);
        assert(spec.type == slmp::highlevel::ValueType::Float32);
        assert(spec.explicit_type);
    }

    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("D50.A", spec) == slmp::Error::Ok);
        assert(spec.device.code == slmp::DeviceCode::D);
        assert(spec.device.number == 50U);
        assert(spec.type == slmp::highlevel::ValueType::Bit);
        assert(spec.bit_index == 10);
    }

    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("M1000.0", spec) == slmp::Error::InvalidArgument);
        assert(slmp::highlevel::parseAddressSpec("D100:BIT", spec) == slmp::Error::InvalidArgument);
    }

    {
        struct ParseCase {
            const char* address;
            slmp::DeviceCode code;
            slmp::highlevel::ValueType type;
            bool explicit_type;
        };
        const ParseCase parse_cases[] = {
            {"Z100", slmp::DeviceCode::Z, slmp::highlevel::ValueType::U16, false},
            {"LZ100", slmp::DeviceCode::LZ, slmp::highlevel::ValueType::U32, false},
            {"RD100", slmp::DeviceCode::RD, slmp::highlevel::ValueType::U16, false},
            {"LTN100", slmp::DeviceCode::LTN, slmp::highlevel::ValueType::U32, false},
            {"LSTN100", slmp::DeviceCode::LSTN, slmp::highlevel::ValueType::U32, false},
            {"LCN100", slmp::DeviceCode::LCN, slmp::highlevel::ValueType::U32, false},
            {"LTS100", slmp::DeviceCode::LTS, slmp::highlevel::ValueType::Bit, false},
            {"LTC100", slmp::DeviceCode::LTC, slmp::highlevel::ValueType::Bit, false},
            {"LSTS100", slmp::DeviceCode::LSTS, slmp::highlevel::ValueType::Bit, false},
            {"LSTC100", slmp::DeviceCode::LSTC, slmp::highlevel::ValueType::Bit, false},
            {"LCS100", slmp::DeviceCode::LCS, slmp::highlevel::ValueType::Bit, false},
            {"LCC100", slmp::DeviceCode::LCC, slmp::highlevel::ValueType::Bit, false},
            {"RD100:D", slmp::DeviceCode::RD, slmp::highlevel::ValueType::U32, true},
        };

        for (const ParseCase& parse_case : parse_cases) {
            slmp::highlevel::AddressSpec spec{};
            assert(slmp::highlevel::parseAddressSpec(parse_case.address, spec) == slmp::Error::Ok);
            assert(spec.device.code == parse_case.code);
            assert(spec.device.number == 100U);
            assert(spec.type == parse_case.type);
            assert(spec.explicit_type == parse_case.explicit_type);
            assert(spec.bit_index < 0);
        }
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        const std::vector<uint8_t> request = {
            0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00,
            0x01, 0x04, 0x02, 0x00, 0xC8, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x02, 0x00
        };
        transport.queueResponse(makeResponse(request, 0x0000, {0x00, 0x00, 0xC0, 0x3F}));

        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "D200:F", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::Float32);
        uint32_t bits = 0;
        std::memcpy(&bits, &value.f32, sizeof(bits));
        assert(bits == 0x3FC00000UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0003), 0x0000, {0x10}));
        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "M100", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::Bit);
        assert(value.bit);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x34, 0x12}));
        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "Z100", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::U16);
        assert(value.u16 == 0x1234U);
        assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0002, {slmp::DeviceCode::Z, 100U});
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x02, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00}));
        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "LTS100", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::Bit);
        assert(value.bit);
        assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0002, {slmp::DeviceCode::LTN, 100U}, 4U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}));
        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "LCS100", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::Bit);
        assert(value.bit);
        assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0002, {slmp::DeviceCode::LCN, 100U}, 4U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x34, 0x12, 0x78, 0x56}));
        slmp::highlevel::Value value{};
        assert(slmp::highlevel::readTyped(plc, "RD100:D", value) == slmp::Error::Ok);
        assert(value.type == slmp::highlevel::ValueType::U32);
        assert(value.u32 == 0x56781234UL);
        assertDirectRequestHeader(transport.lastWrite(), 0x0401, 0x0002, {slmp::DeviceCode::RD, 100U}, 2U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0002), 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::float32Value(1.5f);
        assert(slmp::highlevel::writeTyped(plc, "D200:F", value) == slmp::Error::Ok);
        assert(readLe32(transport.lastWrite().data() + 27) == 0x3FC00000UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401, 0x0002), 0x0000, {0x00, 0x00}));
        std::vector<uint8_t> write_request = makeGenericRequest(0x1401, 0x0002);
        write_request[2] = 0x01;
        transport.queueResponse(makeResponse(write_request, 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::bitValue(true);
        assert(slmp::highlevel::writeTyped(plc, "D50.3", value) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 27) == 0x0008U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1401, 0x0003), 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::bitValue(true);
        assert(slmp::highlevel::writeTyped(plc, "LCC100", value) == slmp::Error::Ok);
        assertDirectRequestHeader(transport.lastWrite(), 0x1401, 0x0003, {slmp::DeviceCode::LCC, 100U});
        assert(transport.lastWrite()[27] == 0x10U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1402, 0x0002), 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::u32Value(0x12345678UL);
        assert(slmp::highlevel::writeTyped(plc, "LZ100", value) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1402U);
        assert(readLe16(transport.lastWrite().data() + 17) == 0x0002U);
        assert(transport.lastWrite()[19] == 0x00U);
        assert(transport.lastWrite()[20] == 0x01U);
        assert(readLe32(transport.lastWrite().data() + 21) == 100U);
        assert(readLe16(transport.lastWrite().data() + 25) == static_cast<uint16_t>(slmp::DeviceCode::LZ));
        assert(readLe32(transport.lastWrite().data() + 27) == 0x12345678UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1402, 0x0002), 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::u32Value(0x12345678UL);
        assert(slmp::highlevel::writeTyped(plc, "LTN100", value) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1402U);
        assert(readLe16(transport.lastWrite().data() + 17) == 0x0002U);
        assert(transport.lastWrite()[19] == 0x00U);
        assert(transport.lastWrite()[20] == 0x01U);
        assert(readLe32(transport.lastWrite().data() + 21) == 100U);
        assert(readLe16(transport.lastWrite().data() + 25) == static_cast<uint16_t>(slmp::DeviceCode::LTN));
        assert(readLe32(transport.lastWrite().data() + 27) == 0x12345678UL);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[128] = {};
        uint8_t rx_buffer[128] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        transport.queueResponse(makeResponse(makeGenericRequest(0x1402, 0x0003), 0x0000, {}));
        const slmp::highlevel::Value value = slmp::highlevel::Value::bitValue(true);
        assert(slmp::highlevel::writeTyped(plc, "LTC100", value) == slmp::Error::Ok);
        assert(readLe16(transport.lastWrite().data() + 15) == 0x1402U);
        assert(readLe16(transport.lastWrite().data() + 17) == 0x0003U);
        assert(transport.lastWrite()[19] == 0x01U);
        assert(readLe32(transport.lastWrite().data() + 20) == 100U);
        assert(readLe16(transport.lastWrite().data() + 24) == static_cast<uint16_t>(slmp::DeviceCode::LTC));
        assert(readLe16(transport.lastWrite().data() + 26) == 0x0001U);
    }
}

void testHighLevelAddressFormatting() {
    {
        char normalized[32] = {};
        assert(slmp::highlevel::normalizeAddress(" d200:f ", normalized, sizeof(normalized)) == slmp::Error::Ok);
        assert(std::string(normalized) == "D200:F");
    }

    {
        char normalized[32] = {};
        assert(slmp::highlevel::normalizeAddress(" x1a ", normalized, sizeof(normalized)) == slmp::Error::Ok);
        assert(std::string(normalized) == "X1A");
    }

    {
        char normalized[32] = {};
        assert(slmp::highlevel::normalizeAddress("d50.a", normalized, sizeof(normalized)) == slmp::Error::Ok);
        assert(std::string(normalized) == "D50.A");
    }

    {
        slmp::highlevel::AddressSpec spec{};
        assert(slmp::highlevel::parseAddressSpec("RD100:D", spec) == slmp::Error::Ok);
        char formatted[32] = {};
        assert(slmp::highlevel::formatAddressSpec(spec, formatted, sizeof(formatted)) == slmp::Error::Ok);
        assert(std::string(formatted) == "RD100:D");
    }

    {
        slmp::highlevel::AddressSpec spec{};
        spec.device = slmp::dev::M(slmp::dev::dec(1000));
        spec.type = slmp::highlevel::ValueType::Bit;
        spec.explicit_type = true;
        char formatted[32] = {};
        assert(slmp::highlevel::formatAddressSpec(spec, formatted, sizeof(formatted)) == slmp::Error::Ok);
        assert(std::string(formatted) == "M1000:BIT");
    }

    {
        slmp::highlevel::AddressSpec spec{};
        spec.device = slmp::dev::D(slmp::dev::dec(50));
        spec.type = slmp::highlevel::ValueType::Bit;
        spec.bit_index = 10;
        char formatted[32] = {};
        assert(slmp::highlevel::formatAddressSpec(spec, formatted, sizeof(formatted)) == slmp::Error::Ok);
        assert(std::string(formatted) == "D50.A");
    }

    {
        char normalized[6] = {};
        assert(slmp::highlevel::normalizeAddress("D200:F", normalized, sizeof(normalized)) == slmp::Error::BufferTooSmall);
    }

    {
        slmp::highlevel::AddressSpec invalid{};
        invalid.device = slmp::dev::M(slmp::dev::dec(100));
        invalid.type = slmp::highlevel::ValueType::Bit;
        invalid.bit_index = 1;
        char formatted[32] = {};
        assert(slmp::highlevel::formatAddressSpec(invalid, formatted, sizeof(formatted)) == slmp::Error::InvalidArgument);
    }
}

void testHighLevelNamedReadAndPoller() {
    MockTransport transport;
    uint8_t tx_buffer[256] = {};
    uint8_t rx_buffer[256] = {};
    slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    std::vector<uint8_t> random_payload = {0x34, 0x12, 0xFE, 0xFF, 0x08, 0x00};
    appendLe32(random_payload, 0x3FC00000UL);
    transport.queueResponse(makeResponse(makeGenericRequest(0x0403, 0x0002), 0x0000, random_payload));
    std::vector<uint8_t> bit_request = makeGenericRequest(0x0401, 0x0003);
    bit_request[2] = 0x01;
    transport.queueResponse(makeResponse(bit_request, 0x0000, {0x10}));

    const std::vector<std::string> addresses = {
        "D100",
        "D101:S",
        "D200:F",
        "D50.3",
        "M100",
    };

    slmp::highlevel::Snapshot snapshot;
    assert(slmp::highlevel::readNamed(plc, addresses, snapshot) == slmp::Error::Ok);
    assert(snapshot.size() == addresses.size());
    assert(snapshot[0].address == "D100");
    assert(snapshot[0].value.u16 == 0x1234U);
    assert(snapshot[1].value.s16 == static_cast<int16_t>(0xFFFEU));
    uint32_t float_bits = 0;
    std::memcpy(&float_bits, &snapshot[2].value.f32, sizeof(float_bits));
    assert(float_bits == 0x3FC00000UL);
    assert(snapshot[3].value.bit);
    assert(snapshot[4].value.bit);
    assert(readLe16(transport.lastWrite().data() + 15) == 0x0401U);

    slmp::highlevel::Poller poller;
    assert(poller.compile(addresses) == slmp::Error::Ok);

    random_payload = {0x78, 0x56, 0xFD, 0xFF, 0x00, 0x00};
    appendLe32(random_payload, 0x40000000UL);
    std::vector<uint8_t> random_request_second = makeGenericRequest(0x0403, 0x0002);
    random_request_second[2] = 0x02;
    transport.queueResponse(makeResponse(random_request_second, 0x0000, random_payload));
    std::vector<uint8_t> bit_request_second = makeGenericRequest(0x0401, 0x0003);
    bit_request_second[2] = 0x03;
    transport.queueResponse(makeResponse(bit_request_second, 0x0000, {0x00}));

    snapshot.clear();
    assert(poller.readOnce(plc, snapshot) == slmp::Error::Ok);
    assert(snapshot.size() == addresses.size());
    assert(snapshot[0].value.u16 == 0x5678U);
    assert(snapshot[1].value.s16 == static_cast<int16_t>(0xFFFDU));
    std::memcpy(&float_bits, &snapshot[2].value.f32, sizeof(float_bits));
    assert(float_bits == 0x40000000UL);
    assert(!snapshot[3].value.bit);
    assert(!snapshot[4].value.bit);
}

void testHighLevelDeviceRangeCatalog() {
    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
        plc.setCompatibilityMode(slmp::CompatibilityMode::Legacy);

        std::vector<uint16_t> registers(46U, 0U);
        registers[0] = 1024U;   // SD260
        registers[2] = 1024U;   // SD262
        registers[4] = 7680U;   // SD264
        registers[6] = 256U;    // SD266
        registers[8] = 512U;    // SD268
        registers[10] = 128U;   // SD270
        registers[14] = 7680U;  // SD274
        registers[20] = 8000U;  // SD280
        registers[22] = 512U;   // SD282
        registers[24] = 512U;   // SD284
        registers[28] = 512U;   // SD288
        registers[30] = 16U;    // SD290
        registers[32] = 256U;   // SD292
        registers[38] = 64U;    // SD298
        registers[40] = 20U;    // SD300
        registers[42] = 2U;     // SD302
        registers[44] = 0x8000U; // SD304
        registers[45] = 0x0000U; // SD305

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401U, 0x0000U), 0x0000U, makeWordPayload(registers)));

        slmp::highlevel::DeviceRangeCatalog catalog;
        assert(slmp::highlevel::readDeviceRangeCatalogForFamily(
            plc,
            slmp::highlevel::DeviceRangeFamily::IqF,
            catalog) == slmp::Error::Ok);

        assertDirectRequestHeader(
            transport.lastWrite(),
            0x0401U,
            0x0000U,
            slmp::dev::SD(slmp::dev::dec(260)),
            46U);
        assert(catalog.family == slmp::highlevel::DeviceRangeFamily::IqF);
        assert(catalog.model == "IQ-F");

        const slmp::highlevel::DeviceRangeEntry* x = findDeviceRangeEntry(catalog, "X");
        assert(x != nullptr);
        assert(x->supported);
        assert(x->has_point_count);
        assert(x->point_count == 1024U);
        assert(x->notation == slmp::highlevel::DeviceRangeNotation::Base8);
        assert(x->address_range == "X0000-X1777");

        const slmp::highlevel::DeviceRangeEntry* y = findDeviceRangeEntry(catalog, "Y");
        assert(y != nullptr);
        assert(y->supported);
        assert(y->has_point_count);
        assert(y->point_count == 1024U);
        assert(y->notation == slmp::highlevel::DeviceRangeNotation::Base8);
        assert(y->address_range == "Y0000-Y1777");

        const slmp::highlevel::DeviceRangeEntry* r = findDeviceRangeEntry(catalog, "R");
        assert(r != nullptr);
        assert(r->supported);
        assert(r->has_point_count);
        assert(r->point_count == 32768U);
        assert(r->has_upper_bound);
        assert(r->upper_bound == 32767U);
        assert(r->address_range == "R0-R32767");

        const slmp::highlevel::DeviceRangeEntry* v = findDeviceRangeEntry(catalog, "V");
        assert(v != nullptr);
        assert(!v->supported);
        assert(!v->has_point_count);
        assert(!v->has_upper_bound);
        assert(v->address_range.empty());
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[256] = {};
        uint8_t rx_buffer[256] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
        plc.setCompatibilityMode(slmp::CompatibilityMode::Legacy);

        std::vector<uint16_t> registers(26U, 0U);
        registers[0] = 8192U;   // SD286 low
        registers[1] = 0U;      // SD287 high
        registers[2] = 8192U;   // SD288 low
        registers[3] = 0U;      // SD289 high
        registers[4] = 8192U;   // SD290
        registers[5] = 8192U;   // SD291
        registers[7] = 8192U;   // SD293
        registers[9] = 2048U;   // SD295
        registers[10] = 2048U;  // SD296
        registers[11] = 2048U;  // SD297
        registers[12] = 8192U;  // SD298
        registers[13] = 2048U;  // SD299
        registers[14] = 16U;    // SD300
        registers[15] = 1024U;  // SD301
        registers[18] = 2048U;  // SD304
        registers[19] = 20U;    // SD305
        registers[22] = 12288U; // SD308 low
        registers[23] = 0U;     // SD309 high
        registers[24] = 8192U;  // SD310 low
        registers[25] = 0U;     // SD311 high

        transport.queueResponse(makeResponse(makeGenericRequest(0x0401U, 0x0000U), 0x0000U, makeWordPayload(registers)));

        slmp::highlevel::DeviceRangeCatalog catalog;
        assert(slmp::highlevel::readDeviceRangeCatalogForFamily(
            plc,
            slmp::highlevel::DeviceRangeFamily::QnU,
            catalog) == slmp::Error::Ok);

        assertDirectRequestHeader(
            transport.lastWrite(),
            0x0401U,
            0x0000U,
            slmp::dev::SD(slmp::dev::dec(286)),
            26U);
        assert(catalog.family == slmp::highlevel::DeviceRangeFamily::QnU);
        assert(catalog.model == "QnU");

        const slmp::highlevel::DeviceRangeEntry* sts = findDeviceRangeEntry(catalog, "STS");
        assert(sts != nullptr);
        assert(sts->supported);
        assert(sts->has_point_count);
        assert(sts->point_count == 16U);
        assert(sts->address_range == "STS0-STS15");

        const slmp::highlevel::DeviceRangeEntry* stc = findDeviceRangeEntry(catalog, "STC");
        assert(stc != nullptr);
        assert(stc->supported);
        assert(stc->has_point_count);
        assert(stc->point_count == 16U);
        assert(stc->address_range == "STC0-STC15");

        const slmp::highlevel::DeviceRangeEntry* stn = findDeviceRangeEntry(catalog, "STN");
        assert(stn != nullptr);
        assert(stn->supported);
        assert(stn->has_point_count);
        assert(stn->point_count == 16U);
        assert(stn->address_range == "STN0-STN15");

        const slmp::highlevel::DeviceRangeEntry* cs = findDeviceRangeEntry(catalog, "CS");
        assert(cs != nullptr);
        assert(cs->supported);
        assert(cs->has_point_count);
        assert(cs->point_count == 1024U);
        assert(cs->address_range == "CS0-CS1023");

        const slmp::highlevel::DeviceRangeEntry* z = findDeviceRangeEntry(catalog, "Z");
        assert(z != nullptr);
        assert(z->supported);
        assert(z->has_point_count);
        assert(z->point_count == 20U);
        assert(z->address_range == "Z0-Z19");

        const slmp::highlevel::DeviceRangeEntry* r = findDeviceRangeEntry(catalog, "R");
        assert(r != nullptr);
        assert(r->supported);
        assert(r->has_point_count);
        assert(r->point_count == 0U);
        assert(!r->has_upper_bound);
        assert(r->address_range.empty());
    }
}

void testCpuOperationState() {
    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> request = makeGenericRequest(0x0401U, 0x0002U);
        transport.queueResponse(makeResponse(request, 0x0000U, {0xA2, 0x00}));

        slmp::CpuOperationState state{};
        assert(plc.readCpuOperationState(state) == slmp::Error::Ok);
        assertDirectRequestHeader(transport.lastWrite(), 0x0401U, 0x0002U, slmp::dev::SD(slmp::dev::dec(203)));
        assert(state.status == slmp::CpuOperationStatus::Stop);
        assert(state.raw_status_word == 0x00A2U);
        assert(state.raw_code == 0x02U);
    }

    {
        MockTransport transport;
        uint8_t tx_buffer[64] = {};
        uint8_t rx_buffer[64] = {};
        slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

        std::vector<uint8_t> request = makeGenericRequest(0x0401U, 0x0002U);
        transport.queueResponse(makeResponse(request, 0x0000U, {0xF5, 0x00}));

        slmp::CpuOperationState state{};
        assert(plc.readCpuOperationState(state) == slmp::Error::Ok);
        assert(state.status == slmp::CpuOperationStatus::Unknown);
        assert(state.raw_status_word == 0x00F5U);
        assert(state.raw_code == 0x05U);
    }
}

}  // namespace

int main() {
    testReadWordsAndFrames();
    testAdditionalDeviceHelpers();
    testAsyncApi();
    testAsyncFloat32Api();
    testFrame3E();
    testAllDirectDeviceFamilies();
    testDWordAndOneShotHelpers();
    testFloat32Helpers();
    testWriteDWordsAndRandomWords();
    testRandomAndBlock();
    testUnsupportedLongFamilyCommandGuards();
    testTargetAndMonitoringTimerHeaders();
    testPlcErrorAndStrings();
    testPasswordAndWriteBlock();
    testRemoteControl();
    testSelfTestAndClearError();
    testWriteBlockOptions();
    testAsyncWriteBlockOptions();
    testValidationAndBoundaryFailures();
    testTransportFailuresAndReconnectHelper();
    testSharedDeviceVectors();
    testSharedCppAddressVectors();
    testSharedGoldenFrames();
    testProtocolFailures();
    testPayloadValidationFailures();
    testAsyncRemoteControl();
    testAsyncSelfTestAndClearError();
    testHighLevelParserAndTypedHelpers();
    testHighLevelAddressFormatting();
    testHighLevelNamedReadAndPoller();
    testHighLevelDeviceRangeCatalog();
    testCpuOperationState();
    std::puts("slmp_minimal_tests: ok");
    return 0;
}
