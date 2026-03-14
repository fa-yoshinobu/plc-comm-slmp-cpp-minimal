#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#include <slmp4e_arduino_transport.h>

namespace {

byte kMacAddress[6] = {0x02, 0x4E, 0x53, 0x4C, 0x4D, 0x50};
IPAddress kLocalIp(192, 168, 250, 50);
IPAddress kDns(192, 168, 250, 1);
IPAddress kGateway(192, 168, 250, 1);
IPAddress kSubnet(255, 255, 255, 0);

constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;
constexpr uint8_t kEthernetCsPin = 17;

EthernetClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[96];
uint8_t rx_buffer[96];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

}  // namespace

void setup() {
    Serial.begin(115200);

    Ethernet.init(kEthernetCsPin);
    Ethernet.begin(kMacAddress, kLocalIp, kDns, kGateway, kSubnet);

    slmp4e::TypeNameInfo info = {};
    (void)plc.connect(kPlcHost, kPlcPort);
    (void)plc.readTypeName(info);
}

void loop() {
    delay(1000);
}
