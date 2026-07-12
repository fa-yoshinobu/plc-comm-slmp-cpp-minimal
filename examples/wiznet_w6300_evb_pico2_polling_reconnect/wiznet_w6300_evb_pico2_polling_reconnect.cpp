#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <W6300lwIP.h>

#include "slmp_arduino_transport.h"
#include "slmp_high_level.h"
#include "slmp_minimal.h"

namespace {

#ifndef SLMP_PLC_HOST
#define SLMP_PLC_HOST "192.168.250.100"
#endif

#ifndef SLMP_PLC_PORT
#define SLMP_PLC_PORT 1035
#endif

#ifndef SLMP_USE_DHCP
#define SLMP_USE_DHCP 0
#endif

#ifndef SLMP_LOCAL_IP
#define SLMP_LOCAL_IP "192.168.250.201"
#endif

#ifndef SLMP_GATEWAY_IP
#define SLMP_GATEWAY_IP "192.168.250.1"
#endif

#ifndef SLMP_SUBNET_MASK
#define SLMP_SUBNET_MASK "255.255.255.0"
#endif

#ifndef SLMP_DNS_IP
#define SLMP_DNS_IP "192.168.250.1"
#endif

#ifndef W6300_PIN_MISO
#define W6300_PIN_MISO 0
#endif

#ifndef W6300_PIN_CS
#define W6300_PIN_CS 1
#endif

#ifndef W6300_PIN_SCK
#define W6300_PIN_SCK 2
#endif

#ifndef W6300_PIN_MOSI
#define W6300_PIN_MOSI 3
#endif

constexpr char kPlcHost[] = SLMP_PLC_HOST;
constexpr uint16_t kPlcPort = SLMP_PLC_PORT;
constexpr uint32_t kReadIntervalMs = 1000;
constexpr uint32_t kInitialBackoffMs = 1000;
constexpr uint32_t kMaxBackoffMs = 30000;
constexpr slmp::highlevel::PlcProfile kPlcProfile = slmp::highlevel::PlcProfile::IqR;

Wiznet6300lwIP g_eth(W6300_PIN_CS);
WiFiUDP g_udp;
slmp::ArduinoUdpTransport g_transport(g_udp);
uint8_t g_tx_buffer[160] = {};
uint8_t g_rx_buffer[160] = {};
slmp::SlmpClient g_plc(g_transport, slmp::PlcProfile::IqR, slmp::TargetAddress{0x00U, 0xFFU, slmp::module_io::OwnStation, 0x00U}, g_tx_buffer, sizeof(g_tx_buffer), g_rx_buffer, sizeof(g_rx_buffer));

bool g_ethernetStarted = false;
bool g_connectedOnce = false;
bool g_online = false;
bool g_pendingRecoveredLog = false;
uint32_t g_lastReadMs = 0;
uint32_t g_nextReconnectMs = 0;
uint32_t g_backoffMs = kInitialBackoffMs;
uint32_t g_lastNetworkLogMs = 0;

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

bool parseIp(const char* text, IPAddress& out) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    return out.fromString(text);
}

bool configureStaticIp() {
#if SLMP_USE_DHCP
    return true;
#else
    IPAddress local;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
    if (!parseIp(SLMP_LOCAL_IP, local) ||
        !parseIp(SLMP_GATEWAY_IP, gateway) ||
        !parseIp(SLMP_SUBNET_MASK, subnet) ||
        !parseIp(SLMP_DNS_IP, dns)) {
        logState("fatal", "invalid static IPv4 build flag");
        return false;
    }
    return g_eth.config(local, gateway, subnet, dns);
#endif
}

bool startEthernet() {
    if (g_ethernetStarted) {
        return true;
    }

    SPI.setRX(W6300_PIN_MISO);
    SPI.setCS(W6300_PIN_CS);
    SPI.setSCK(W6300_PIN_SCK);
    SPI.setTX(W6300_PIN_MOSI);

    if (!configureStaticIp()) {
        return false;
    }
    if (!g_eth.begin()) {
        logState("fatal", "W6300 Ethernet init failed");
        return false;
    }
    g_eth.setDefault(true);
    g_ethernetStarted = true;
    return true;
}

bool ensureEthernet() {
    if (!startEthernet()) {
        delay(500);
        return false;
    }

    if (g_eth.linkStatus() == LinkOFF) {
        if (g_online) {
            scheduleReconnect("ethernet link down", slmp::Error::TransportError);
        } else if ((millis() - g_lastNetworkLogMs) >= 2000U) {
            g_lastNetworkLogMs = millis();
            logState("reconnecting", "waiting for Ethernet link");
        }
        return false;
    }

    if (!g_eth.connected()) {
        if ((millis() - g_lastNetworkLogMs) >= 2000U) {
            g_lastNetworkLogMs = millis();
            logState("reconnecting", "waiting for IPv4 address");
        }
        return false;
    }

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

void readD100() {
    slmp::highlevel::Value d100;
    const auto err = slmp::highlevel::readTyped(g_plc, "D100:U", d100);
    if (err != slmp::Error::Ok) {
        if (retryable(err)) {
            scheduleReconnect("readTyped failed", err);
        } else {
            Serial.printf("%lu [read] readTyped failed err=%u\n", static_cast<unsigned long>(millis()), static_cast<unsigned>(err));
        }
        return;
    }

    if (g_pendingRecoveredLog) {
        logState("recovered", "D100:U");
        g_pendingRecoveredLog = false;
        g_online = true;
    }

    Serial.printf("%lu [read] D100:U=%u\n", static_cast<unsigned long>(millis()), static_cast<unsigned>(d100.u16));
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("W6300-EVB-Pico2 SLMP UDP polling reconnect sample");
}

void loop() {
    if (!ensureEthernet()) {
        delay(100);
        return;
    }

    if (!ensurePlc()) {
        delay(50);
        return;
    }

    const uint32_t now = millis();
    if ((now - g_lastReadMs) >= kReadIntervalMs) {
        g_lastReadMs = now;
        readD100();
    }

    delay(25);
}
