#include <WiFi.h>

#include <slmp4e_arduino_transport.h>

namespace {

constexpr char kSsid[] = "YOUR_WIFI_SSID";
constexpr char kPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;
constexpr bool kEnableWrites = false;

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[256];
uint8_t rx_buffer[256];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

void printLastError(const char* label) {
    Serial.printf(
        "%s failed: %s end=0x%04X\n",
        label,
        slmp4e::errorString(plc.lastError()),
        plc.lastEndCode()
    );
}

}  // namespace

void connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(kSsid, kPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void setup() {
    Serial.begin(115200);
    connectWifi();

    if (!plc.connect(kPlcHost, kPlcPort)) {
        printLastError("connect");
        return;
    }

    const slmp4e::DeviceAddress random_words[] = {
        {slmp4e::DeviceCode::D, 100},
        {slmp4e::DeviceCode::D, 101},
    };
    const slmp4e::DeviceAddress random_dwords[] = {
        {slmp4e::DeviceCode::D, 200},
    };
    uint16_t random_word_values[2] = {};
    uint32_t random_dword_values[1] = {};

    if (plc.readRandom(
            random_words,
            2,
            random_word_values,
            2,
            random_dwords,
            1,
            random_dword_values,
            1
        ) == slmp4e::Error::Ok) {
        Serial.printf(
            "random D100=%u D101=%u D200=%lu\n",
            random_word_values[0],
            random_word_values[1],
            static_cast<unsigned long>(random_dword_values[0])
        );
    } else {
        printLastError("readRandom");
    }

    const slmp4e::DeviceBlockRead word_blocks[] = {
        {{slmp4e::DeviceCode::D, 300}, 2},
    };
    const slmp4e::DeviceBlockRead bit_blocks[] = {
        {{slmp4e::DeviceCode::M, 200}, 1},
    };
    uint16_t block_word_values[2] = {};
    uint16_t block_bit_values[1] = {};

    if (plc.readBlock(word_blocks, 1, bit_blocks, 1, block_word_values, 2, block_bit_values, 1) ==
        slmp4e::Error::Ok) {
        Serial.printf(
            "block D300=%u D301=%u M200packed=0x%04X\n",
            block_word_values[0],
            block_word_values[1],
            block_bit_values[0]
        );
    } else {
        printLastError("readBlock");
    }

    if (kEnableWrites) {
        const uint16_t write_random_words[] = {0x1111, 0x2222};
        const uint32_t write_random_dwords[] = {0x12345678UL};

        if (plc.writeRandomWords(
                random_words,
                write_random_words,
                2,
                random_dwords,
                write_random_dwords,
                1
            ) != slmp4e::Error::Ok) {
            printLastError("writeRandomWords");
        }

        const slmp4e::DeviceAddress random_bits[] = {
            slmp4e::dev::M(slmp4e::dev::dec(100)),
            slmp4e::dev::Y(slmp4e::dev::hex(0x20)),
        };
        const bool write_random_bits[] = {true, false};

        if (plc.writeRandomBits(random_bits, write_random_bits, 2) != slmp4e::Error::Ok) {
            printLastError("writeRandomBits");
        }

        const uint16_t write_block_words[] = {0x3333, 0x4444};
        const uint16_t write_block_bits[] = {0x0005};
        const slmp4e::DeviceBlockWrite write_word_blocks[] = {
            {{slmp4e::DeviceCode::D, 300}, write_block_words, 2},
        };
        const slmp4e::DeviceBlockWrite write_bit_blocks[] = {
            {{slmp4e::DeviceCode::M, 200}, write_block_bits, 1},
        };

        if (plc.writeBlock(write_word_blocks, 1, write_bit_blocks, 1) != slmp4e::Error::Ok) {
            printLastError("writeBlock");
        }
    }
}

void loop() {
    delay(1000);
}
