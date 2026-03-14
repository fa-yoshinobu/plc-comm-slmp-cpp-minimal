#include <Arduino.h>
#include <WiFi.h>

#include <slmp4e_arduino_transport.h>
#include <slmp4e_utility.h>

namespace {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;
constexpr char kPlcPassword[] = "123456";

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[128];
uint8_t rx_buffer[128];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
slmp4e::ReconnectHelper reconnect(
    plc,
    kPlcHost,
    kPlcPort,
    slmp4e::ReconnectOptions{3000}
);

}  // namespace

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);

    (void)reconnect.ensureConnected(millis());
    if (reconnect.consumeConnectedEdge()) {
        (void)plc.remotePasswordUnlock(kPlcPassword);
        slmp4e::TypeNameInfo info = {};
        (void)plc.readTypeName(info);
    }

    uint16_t words[2] = {};
    (void)plc.readWords(slmp4e::dev::D(slmp4e::dev::dec(100)), 2, words, 2);
}

void loop() {
    delay(1000);
}
