#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <vector>

#include "slmp_arduino_transport.h"
#include "slmp_high_level.h"
#include "slmp_minimal.h"

namespace {

#ifndef SLMP_WIFI_SSID
#define SLMP_WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef SLMP_WIFI_PASSWORD
#define SLMP_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#ifndef SLMP_PLC_HOST
#define SLMP_PLC_HOST "192.168.250.100"
#endif

#ifndef SLMP_PLC_PORT
#define SLMP_PLC_PORT 1035
#endif

constexpr char kWifiSsid[] = SLMP_WIFI_SSID;
constexpr char kWifiPassword[] = SLMP_WIFI_PASSWORD;
constexpr char kPlcHost[] = SLMP_PLC_HOST;
constexpr uint16_t kPlcPort = SLMP_PLC_PORT;
constexpr uint32_t kReadIntervalMs = 1000;
constexpr uint32_t kInitialBackoffMs = 1000;
constexpr uint32_t kMaxBackoffMs = 30000;
constexpr slmp::highlevel::PlcProfile kPlcProfile = slmp::highlevel::PlcProfile::IqR;

WiFiUDP g_udp;
slmp::ArduinoUdpTransport g_transport(g_udp);
uint8_t g_tx_buffer[160] = {};
uint8_t g_rx_buffer[160] = {};
slmp::SlmpClient g_plc(g_transport, slmp::PlcProfile::IqR, slmp::TargetAddress{0x00U, 0xFFU, slmp::module_io::OwnStation, 0x00U}, g_tx_buffer, sizeof(g_tx_buffer), g_rx_buffer, sizeof(g_rx_buffer));

slmp::highlevel::Poller g_poller;
bool g_planReady = false;
bool g_connectedOnce = false;
bool g_online = false;
bool g_pendingRecoveredLog = false;
uint32_t g_lastReadMs = 0;
uint32_t g_nextReconnectMs = 0;
uint32_t g_backoffMs = kInitialBackoffMs;

void logState(const char* state, const char* message) {
    Serial.printf("%lu [%s] %s\n", static_cast<unsigned long>(millis()), state, message);
}

bool retryable(slmp::Error error) {
    return error == slmp::Error::NotConnected
        || error == slmp::Error::TransportError
        || error == slmp::Error::ProtocolError;
}

void scheduleReconnect(const char* reason, slmp::Error error) {
    if (g_online) {
        Serial.printf("%lu [lost] %s err=%u\n", static_cast<unsigned long>(millis()), reason, static_cast<unsigned>(error));
    }
    g_plc.close();
    g_online = false;
    g_pendingRecoveredLog = false;
    g_nextReconnectMs = millis() + g_backoffMs;
    Serial.printf("%lu [reconnecting] retry in %lu ms\n", static_cast<unsigned long>(millis()), static_cast<unsigned long>(g_backoffMs));
    g_backoffMs = (g_backoffMs >= (kMaxBackoffMs / 2U)) ? kMaxBackoffMs : (g_backoffMs * 2U);
}

bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    if (g_online || g_plc.connected()) {
        scheduleReconnect("wifi disconnected", slmp::Error::TransportError);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiSsid, kWifiPassword);

    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - started) < 15000U) {
        delay(250);
    }

    return WiFi.status() == WL_CONNECTED;
}

bool compilePoller() {
    if (g_planReady) {
        return true;
    }

    const std::vector<std::string> addresses = {
        "SM400:BIT",
        "D100:U",
        "D200:F",
        "D50.3",
    };
    const auto compileErr = g_poller.compile(addresses, kPlcProfile);
    if (compileErr != slmp::Error::Ok) {
        Serial.printf("poller compile failed: %u\n", static_cast<unsigned>(compileErr));
        return false;
    }
    g_planReady = true;
    return true;
}

bool ensurePlc() {
    if (g_plc.connected()) {
        return true;
    }

    const uint32_t now = millis();
    if (now < g_nextReconnectMs) {
        return false;
    }

    logState("reconnecting", "opening SLMP UDP endpoint");
    g_plc.setPlcProfile(kPlcProfile);
    if (!g_plc.connect(kPlcHost, kPlcPort)) {
        scheduleReconnect("connect failed", g_plc.lastError());
        return false;
    }

    if (!compilePoller()) {
        scheduleReconnect("poller compile failed", slmp::Error::InvalidArgument);
        return false;
    }

    if (g_connectedOnce) {
        g_pendingRecoveredLog = true;
        logState("reconnecting", "transport opened; waiting for read");
    } else {
        logState("connected", "D100:U");
        g_connectedOnce = true;
        g_online = true;
    }
    g_backoffMs = kInitialBackoffMs;
    g_nextReconnectMs = 0;
    return true;
}

void readHighLevelValues() {
    slmp::highlevel::Value d100;
    const auto typedErr = slmp::highlevel::readTyped(g_plc, "D100:U", d100);
    if (typedErr != slmp::Error::Ok) {
        if (retryable(typedErr)) {
            scheduleReconnect("readTyped failed", typedErr);
        } else {
            Serial.printf("%lu [read] readTyped failed err=%u\n", static_cast<unsigned long>(millis()), static_cast<unsigned>(typedErr));
        }
        return;
    }

    slmp::highlevel::Snapshot snapshot;
    const auto pollErr = g_poller.readOnce(g_plc, snapshot);
    if (pollErr != slmp::Error::Ok || snapshot.size() < 4U) {
        if (retryable(pollErr)) {
            scheduleReconnect("poller read failed", pollErr);
        } else {
            Serial.printf("%lu [read] poller read failed err=%u\n", static_cast<unsigned long>(millis()), static_cast<unsigned>(pollErr));
        }
        return;
    }

    if (g_pendingRecoveredLog) {
        logState("recovered", "D100:U");
        g_pendingRecoveredLog = false;
        g_online = true;
    }

    Serial.printf(
        "%lu [read] D100:U=%u SM400=%u D200:F=%.3f D50.3=%u\n",
        static_cast<unsigned long>(millis()),
        static_cast<unsigned>(d100.u16),
        snapshot[0].value.bit ? 1U : 0U,
        static_cast<double>(snapshot[2].value.f32),
        snapshot[3].value.bit ? 1U : 0U);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-DevKitC SLMP UDP polling reconnect sample");
}

void loop() {
    if (!ensureWifi()) {
        delay(250);
        return;
    }

    if (!ensurePlc()) {
        delay(50);
        return;
    }

    const uint32_t now = millis();
    if ((now - g_lastReadMs) >= kReadIntervalMs) {
        g_lastReadMs = now;
        readHighLevelValues();
    }

    delay(25);
}
