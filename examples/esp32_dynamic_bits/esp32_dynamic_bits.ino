#include <WiFi.h>

#include <slmp4e_arduino_transport.h>

namespace {

constexpr char kSsid[] = "YOUR_WIFI_SSID";
constexpr char kPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;

constexpr uint32_t kFirstDevice = 100;
constexpr uint32_t kLastDevice = 500;
constexpr bool kOddOnly = true;
constexpr uint32_t kWriteIntervalMs = 100;

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[96];
uint8_t rx_buffer[96];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

uint32_t next_device = kFirstDevice;
uint32_t last_write_ms = 0;

void printLastError(const char* label) {
    Serial.printf(
        "%s failed: %s end=0x%04X\n",
        label,
        slmp4e::errorString(plc.lastError()),
        plc.lastEndCode()
    );
}

void advanceDevice() {
    ++next_device;
    if (next_device > kLastDevice) {
        next_device = kFirstDevice;
    }
}

bool shouldWrite(uint32_t device_no) {
    if (!kOddOnly) {
        return true;
    }
    return (device_no & 1U) == 1U;
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
    }
}

void loop() {
    uint32_t now = millis();
    if (now - last_write_ms < kWriteIntervalMs) {
        delay(1);
        return;
    }
    last_write_ms = now;

    if (!plc.connected()) {
        if (!plc.connect(kPlcHost, kPlcPort)) {
            printLastError("reconnect");
            delay(1000);
            return;
        }
    }

    if (!shouldWrite(next_device)) {
        advanceDevice();
        return;
    }

    const bool on = true;
    const slmp4e::DeviceAddress device = slmp4e::dev::M(slmp4e::dev::dec(next_device));
    if (plc.writeBits(device, &on, 1) == slmp4e::Error::Ok) {
        Serial.printf("M%lu=ON\n", static_cast<unsigned long>(next_device));
    } else {
        printLastError("writeBits");
    }

    advanceDevice();
}
