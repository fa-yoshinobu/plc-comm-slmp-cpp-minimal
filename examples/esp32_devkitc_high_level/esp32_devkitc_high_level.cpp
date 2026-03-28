#include <Arduino.h>
#include <WiFi.h>

#include <vector>

#include "slmp_arduino_transport.h"
#include "slmp_high_level.h"
#include "slmp_minimal.h"

namespace {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kPlcHost[] = "192.168.250.100";
constexpr uint16_t kPlcPort = 1025;
constexpr uint32_t kReadIntervalMs = 1000;
constexpr slmp::FrameType kFrameType = slmp::FrameType::Frame4E;
constexpr slmp::CompatibilityMode kCompatibilityMode = slmp::CompatibilityMode::iQR;

WiFiClient g_tcp;
slmp::ArduinoClientTransport g_transport(g_tcp);
uint8_t g_tx_buffer[160] = {};
uint8_t g_rx_buffer[160] = {};
slmp::SlmpClient g_plc(g_transport, g_tx_buffer, sizeof(g_tx_buffer), g_rx_buffer, sizeof(g_rx_buffer));

slmp::highlevel::Poller g_poller;
bool g_planReady = false;
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

    g_plc.setFrameType(kFrameType);
    g_plc.setCompatibilityMode(kCompatibilityMode);

    if (!g_plc.connect(kPlcHost, kPlcPort)) {
        Serial.printf("connect failed: %u\n", static_cast<unsigned>(g_plc.lastError()));
        return false;
    }

    slmp::TypeNameInfo info = {};
    const bool type_name_ok = (g_plc.readTypeName(info) == slmp::Error::Ok);

    if (!g_planReady) {
        const std::vector<std::string> addresses = {
            "SM400",
            "D100",
            "D200:F",
            "D50.3",
        };
        const auto compileErr = g_poller.compile(addresses);
        if (compileErr != slmp::Error::Ok) {
            Serial.printf("Poller compile failed: %u\n", static_cast<unsigned>(compileErr));
            return false;
        }
        g_planReady = true;
    }

    Serial.printf(
        "connected frame=%u compat=%u model=%s\n",
        static_cast<unsigned>(kFrameType),
        static_cast<unsigned>(kCompatibilityMode),
        type_name_ok ? info.model : "unknown");
    return true;
}

void readHighLevelValues() {
    slmp::highlevel::Value d100;
    const auto typedErr = slmp::highlevel::readTyped(g_plc, "D100", d100);
    if (typedErr != slmp::Error::Ok) {
        Serial.printf("readTyped failed: %u\n", static_cast<unsigned>(typedErr));
        return;
    }

    slmp::highlevel::Snapshot snapshot;
    const auto pollErr = g_poller.readOnce(g_plc, snapshot);
    if (pollErr != slmp::Error::Ok || snapshot.size() < 4U) {
        Serial.printf("Poller read failed: %u\n", static_cast<unsigned>(pollErr));
        return;
    }

    Serial.printf(
        "high-level D100=%u SM400=%u D200:F=%.3f D50.3=%u\n",
        static_cast<unsigned>(d100.u16),
        snapshot[0].value.bit ? 1U : 0U,
        static_cast<double>(snapshot[2].value.f32),
        snapshot[3].value.bit ? 1U : 0U);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-DevKitC high-level SLMP sample");
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
        readHighLevelValues();
    }

    delay(50);
}
