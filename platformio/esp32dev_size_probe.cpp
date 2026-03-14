#include <Arduino.h>
#include <WiFi.h>

#include <slmp4e_arduino_transport.h>

namespace {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[256];
uint8_t rx_buffer[256];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

}  // namespace

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);

    (void)plc.connect(kPlcHost, kPlcPort);

    slmp4e::TypeNameInfo type_name = {};
    (void)plc.readTypeName(type_name);

#if defined(SLMP4E_SIZE_USE_DIRECT) || defined(SLMP4E_SIZE_USE_ALL)
    uint16_t read_words[2] = {};
    const uint16_t write_words[2] = {0x1111, 0x2222};
    bool read_bits[2] = {};
    const bool write_bits[2] = {true, false};
    uint32_t read_dwords[2] = {};
    const uint32_t write_dwords[2] = {0x12345678UL, 0x89ABCDEFUL};
    uint16_t one_word = 0;
    bool one_bit = false;
    uint32_t one_dword = 0;

    (void)plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, read_words, 2);
    (void)plc.writeWords(slmp4e::dev::D(slmp4e::dev::dec(100)), write_words, 2);
    (void)plc.readBits(slmp4e::dev::M(slmp4e::dev::dec(100)), 2, read_bits, 2);
    (void)plc.writeBits(slmp4e::dev::M(slmp4e::dev::dec(100)), write_bits, 2);
    (void)plc.readDWords(slmp4e::dev::D(slmp4e::dev::dec(200)), 2, read_dwords, 2);
    (void)plc.writeDWords(slmp4e::dev::D(slmp4e::dev::dec(200)), write_dwords, 2);
    (void)plc.readOneWord(slmp4e::dev::D(slmp4e::dev::dec(300)), one_word);
    (void)plc.writeOneWord(slmp4e::dev::D(slmp4e::dev::dec(300)), 0x5555);
    (void)plc.readOneBit(slmp4e::dev::M(slmp4e::dev::dec(300)), one_bit);
    (void)plc.writeOneBit(slmp4e::dev::M(slmp4e::dev::dec(300)), true);
    (void)plc.readOneDWord(slmp4e::dev::D(slmp4e::dev::dec(400)), one_dword);
    (void)plc.writeOneDWord(slmp4e::dev::D(slmp4e::dev::dec(400)), 0xCAFEBABEUL);
#endif

#if defined(SLMP4E_SIZE_USE_PASSWORD) || defined(SLMP4E_SIZE_USE_ALL)
    (void)plc.remotePasswordUnlock("abcdef");
    (void)plc.remotePasswordLock("abcdef");
#endif

#if defined(SLMP4E_SIZE_USE_ENDCODE) || defined(SLMP4E_SIZE_USE_ALL)
    volatile uint16_t end_code = 0x4031;
    const char* end_code_text = slmp4e::endCodeString(end_code);
    if (end_code_text[0] == '\0') {
        Serial.println(end_code_text);
    }
#endif

#if defined(SLMP4E_SIZE_USE_DEBUG) || defined(SLMP4E_SIZE_USE_ALL)
    char request_hex[160] = {};
    char response_hex[160] = {};
    (void)slmp4e::formatHexBytes(
        plc.lastRequestFrame(),
        plc.lastRequestFrameLength(),
        request_hex,
        sizeof(request_hex)
    );
    (void)slmp4e::formatHexBytes(
        plc.lastResponseFrame(),
        plc.lastResponseFrameLength(),
        response_hex,
        sizeof(response_hex)
    );
    if (request_hex[0] == '\0' && response_hex[0] == '\0') {
        Serial.println(request_hex);
    }
#endif

#if defined(SLMP4E_SIZE_USE_RANDOM) || defined(SLMP4E_SIZE_USE_ALL)
    const slmp4e::DeviceAddress random_words[] = {
        slmp4e::dev::D(slmp4e::dev::dec(100)),
        slmp4e::dev::D(slmp4e::dev::dec(101)),
    };
    const slmp4e::DeviceAddress random_dwords[] = {
        slmp4e::dev::D(slmp4e::dev::dec(200)),
    };
    uint16_t random_word_values[2] = {};
    uint32_t random_dword_values[1] = {};
    const uint16_t random_write_words[] = {0x1111, 0x2222};
    const uint32_t random_write_dwords[] = {0x12345678UL};
    const slmp4e::DeviceAddress random_bits[] = {
        slmp4e::dev::M(slmp4e::dev::dec(100)),
        slmp4e::dev::Y(slmp4e::dev::hex(0x20)),
    };
    const bool random_bit_values[] = {true, false};

    (void)plc.readRandom(random_words, 2, random_word_values, 2, random_dwords, 1, random_dword_values, 1);
    (void)plc.writeRandomWords(random_words, random_write_words, 2, random_dwords, random_write_dwords, 1);
    (void)plc.writeRandomBits(random_bits, random_bit_values, 2);
#endif

#if defined(SLMP4E_SIZE_USE_BLOCK) || defined(SLMP4E_SIZE_USE_ALL)
    const slmp4e::DeviceBlockRead read_word_blocks[] = {
        slmp4e::dev::blockRead(slmp4e::dev::D(slmp4e::dev::dec(300)), 2),
    };
    const slmp4e::DeviceBlockRead read_bit_blocks[] = {
        slmp4e::dev::blockRead(slmp4e::dev::M(slmp4e::dev::dec(200)), 1),
    };
    uint16_t block_word_values[2] = {};
    uint16_t block_bit_values[1] = {};
    const uint16_t write_block_words[] = {0x3333, 0x4444};
    const uint16_t write_block_bits[] = {0x0005};
    const slmp4e::DeviceBlockWrite write_word_blocks[] = {
        slmp4e::dev::blockWrite(slmp4e::dev::D(slmp4e::dev::dec(300)), write_block_words, 2),
    };
    const slmp4e::DeviceBlockWrite write_bit_blocks[] = {
        slmp4e::dev::blockWrite(slmp4e::dev::M(slmp4e::dev::dec(200)), write_block_bits, 1),
    };

    (void)plc.readBlock(read_word_blocks, 1, read_bit_blocks, 1, block_word_values, 2, block_bit_values, 1);
    (void)plc.writeBlock(write_word_blocks, 1, write_bit_blocks, 1);
#endif
}

void loop() {
    delay(1000);
}
