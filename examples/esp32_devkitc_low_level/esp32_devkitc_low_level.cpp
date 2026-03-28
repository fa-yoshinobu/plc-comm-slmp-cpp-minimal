#include <Arduino.h>
#include <WiFi.h>

#include "slmp_arduino_transport.h"
#include "slmp_minimal.h"

namespace {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kPlcPort = 1025;
constexpr uint32_t kReadIntervalMs = 1000;

WiFiClient g_tcp;
slmp::ArduinoClientTransport g_transport(g_tcp);
uint8_t g_tx_buffer[128] = {};
uint8_t g_rx_buffer[128] = {};
slmp::SlmpClient g_plc(g_transport, g_tx_buffer, sizeof(g_tx_buffer), g_rx_buffer, sizeof(g_rx_buffer));

uint32_t g_lastReadMs = 0;

bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);

    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started) < 15000U) {
        delay(250);
    }

    return WiFi.status() == WL_CONNECTED;
}

bool ensurePlc() {
    if (g_plc.connected()) {
        return true;
    }

    g_plc.setFrameType(slmp::FrameType::Frame4E);
    g_plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);
    return g_plc.connect(kPlcHost, kPlcPort);
}

void readDirectDevices() {
    uint16_t d100 = 0;
    bool m1000 = false;

    const auto wordErr = g_plc.readOneWord(slmp::dev::D(slmp::dev::dec(100)), d100);
    const auto bitErr = g_plc.readOneBit(slmp::dev::M(slmp::dev::dec(1000)), m1000);

    if (wordErr == slmp::Error::Ok && bitErr == slmp::Error::Ok) {
        Serial.printf("low-level D100=%u M1000=%u\n", static_cast<unsigned>(d100), m1000 ? 1U : 0U);
        return;
    }

    Serial.printf(
        "low-level read failed: word=%u bit=%u\n",
        static_cast<unsigned>(wordErr),
        static_cast<unsigned>(bitErr));
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-DevKitC low-level SLMP sample");
}

void loop() {
    if (!ensureWifi()) {
        Serial.println("Wi-Fi not connected");
        delay(1000);
        return;
    }

    if (!ensurePlc()) {
        Serial.println("PLC connection failed");
        delay(1000);
        return;
    }

    const uint32_t now = millis();
    if ((now - g_lastReadMs) >= kReadIntervalMs) {
        g_lastReadMs = now;
        readDirectDevices();
    }

    delay(50);
}
