#include <SPI.h>
#include <Ethernet.h>

#include <slmp4e_arduino_transport.h>

namespace {

byte kMacAddress[6] = {0x02, 0x4E, 0x53, 0x4C, 0x4D, 0x50};
constexpr IPAddress kLocalIp(192, 168, 250, 50);
constexpr IPAddress kDns(192, 168, 250, 1);
constexpr IPAddress kGateway(192, 168, 250, 1);
constexpr IPAddress kSubnet(255, 255, 255, 0);
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

// Adjust this pin to match the RP2040 board and W5500 wiring.
constexpr uint8_t kEthernetCsPin = 17;

EthernetClient tcp_client;
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
    Serial.print("last request: ");
    Serial.println(request_hex);
    Serial.print("last response: ");
    Serial.println(response_hex);
}

void printLastPlcError(const char* label) {
    Serial.print(label);
    Serial.print(" failed: ");
    Serial.print(slmp4e::errorString(plc.lastError()));
    Serial.print(" end=0x");
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(" (");
    Serial.print(slmp4e::endCodeString(plc.lastEndCode()));
    Serial.println(")");
    printLastFrames();
}

}  // namespace

void bringUpEthernet() {
    Ethernet.init(kEthernetCsPin);
    Ethernet.begin(
        kMacAddress,
        kLocalIp,
        kDns,
        kGateway,
        kSubnet
    );
    delay(1000);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    bringUpEthernet();
    Serial.print("local ip=");
    Serial.println(Ethernet.localIP());

    if (!plc.connect(kPlcHost, kPlcPort)) {
        Serial.print("connect failed: ");
        Serial.println(slmp4e::errorString(plc.lastError()));
        return;
    }

    slmp4e::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) == slmp4e::Error::Ok) {
        Serial.print("model=");
        Serial.print(type_name.model);
        if (type_name.has_model_code) {
            Serial.print(" code=0x");
            Serial.println(type_name.model_code, HEX);
        } else {
            Serial.println();
        }
    } else {
        printLastPlcError("readTypeName");
    }

    uint16_t words[2] = {};
    if (plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, words, 2) == slmp4e::Error::Ok) {
        Serial.print("D100=");
        Serial.print(words[0]);
        Serial.print(" D101=");
        Serial.println(words[1]);
    } else {
        printLastPlcError("readWords");
    }
}

void loop() {
    delay(1000);
}
