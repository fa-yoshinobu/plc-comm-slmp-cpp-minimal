#include <WiFi.h>

#include <slmp4e_arduino_transport.h>

namespace {

constexpr char kSsid[] = "YOUR_WIFI_SSID";
constexpr char kPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[96];
uint8_t rx_buffer[96];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

void printLastFrames() {
    char request_hex[160] = {};
    char response_hex[160] = {};
    slmp4e::formatHexBytes(
        plc.lastRequestFrame(),
        plc.lastRequestFrameLength(),
        request_hex,
        sizeof(request_hex)
    );
    slmp4e::formatHexBytes(
        plc.lastResponseFrame(),
        plc.lastResponseFrameLength(),
        response_hex,
        sizeof(response_hex)
    );
    Serial.printf("last request: %s\n", request_hex);
    Serial.printf("last response: %s\n", response_hex);
}

void printLastPlcError(const char* label) {
    Serial.printf(
        "%s failed: %s end=0x%04X (%s)\n",
        label,
        slmp4e::errorString(plc.lastError()),
        plc.lastEndCode(),
        slmp4e::endCodeString(plc.lastEndCode())
    );
    printLastFrames();
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
        Serial.printf("connect failed: %s\n", slmp4e::errorString(plc.lastError()));
        return;
    }

    slmp4e::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) == slmp4e::Error::Ok) {
        Serial.printf("model=%s code=0x%04X\n", type_name.model, type_name.model_code);
    } else {
        printLastPlcError("readTypeName");
    }

    uint16_t words[2] = {};
    slmp4e::DeviceAddress d100 = slmp4e::dev::D(slmp4e::dev::dec(100));
    if (plc.readWords(d100, 2, words, 2) == slmp4e::Error::Ok) {
        Serial.printf("D100=%u D101=%u\n", words[0], words[1]);
    } else {
        printLastPlcError("readWords");
    }
}

void loop() {
    delay(1000);
}
