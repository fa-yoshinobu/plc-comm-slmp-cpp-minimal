#include <Arduino.h>
#include <WiFi.h>

#include <slmp4e_arduino_transport.h>

namespace {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

constexpr uint32_t kFirstDevice = 100;
constexpr uint32_t kLastDevice = 500;

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[96];
uint8_t rx_buffer[96];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

uint32_t next_device = kFirstDevice;

void advanceDevice() {
    ++next_device;
    if (next_device > kLastDevice) {
        next_device = kFirstDevice;
    }
}

bool shouldWrite(uint32_t device_no) {
    return (device_no & 1U) == 1U;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);
    (void)plc.connect(kPlcHost, kPlcPort);

    for (size_t i = 0; i < 5; ++i) {
        if (!shouldWrite(next_device)) {
            advanceDevice();
            continue;
        }

        const bool on = true;
        const slmp4e::DeviceAddress device = slmp4e::dev::M(slmp4e::dev::dec(next_device));
        (void)plc.writeBits(device, &on, 1);
        advanceDevice();
    }
}

void loop() {
    delay(1000);
}
