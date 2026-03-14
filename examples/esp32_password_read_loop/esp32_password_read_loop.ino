#include <WiFi.h>

#include <slmp4e_arduino_transport.h>
#include <slmp4e_utility.h>

#include "config.h"

namespace {

WiFiClient tcp_client;
slmp4e::ArduinoClientTransport transport(tcp_client);

uint8_t tx_buffer[example_config::kTxBufferSize];
uint8_t rx_buffer[example_config::kRxBufferSize];
slmp4e::Slmp4eClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
slmp4e::ReconnectHelper reconnect(
    plc,
    example_config::kPlcHost,
    example_config::kPlcPort,
    slmp4e::ReconnectOptions{example_config::kReconnectIntervalMs}
);

uint32_t last_poll_ms = 0;

void printLastFrames() {
    char request_hex[192] = {};
    char response_hex[192] = {};
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

void printLastError(const char* label) {
    Serial.printf(
        "%s failed: %s end=0x%04X (%s)\n",
        label,
        slmp4e::errorString(plc.lastError()),
        plc.lastEndCode(),
        slmp4e::endCodeString(plc.lastEndCode())
    );
    printLastFrames();
}

bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(example_config::kWifiSsid, example_config::kWifiPassword);

    const uint32_t start_ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start_ms >= example_config::kWifiConnectTimeoutMs) {
            return false;
        }
        delay(250);
    }
    return true;
}

bool openPlcSession() {
    if (example_config::kPlcPassword[0] != '\0') {
        if (plc.remotePasswordUnlock(example_config::kPlcPassword) != slmp4e::Error::Ok) {
            printLastError("remotePasswordUnlock");
            return false;
        }
        Serial.println("password unlocked");
    }

    slmp4e::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp4e::Error::Ok) {
        printLastError("readTypeName");
        return false;
    }

    Serial.printf("model=%s code=0x%04X\n", type_name.model, type_name.model_code);
    return true;
}

}  // namespace

void setup() {
    Serial.begin(115200);
}

void loop() {
    if (!ensureWifi()) {
        delay(500);
        return;
    }

    if (!reconnect.ensureConnected(millis())) {
        delay(100);
        return;
    }

    if (reconnect.consumeConnectedEdge()) {
        Serial.println("plc connected");
        if (!openPlcSession()) {
            reconnect.forceReconnect(millis());
            delay(200);
            return;
        }
        last_poll_ms = 0;
    }

    const uint32_t now_ms = millis();
    if (now_ms - last_poll_ms < example_config::kPollIntervalMs) {
        delay(20);
        return;
    }
    last_poll_ms = now_ms;

    uint16_t words[example_config::kPollPoints] = {};
    if (plc.readWords(
            slmp4e::dev::D(slmp4e::dev::dec(example_config::kPollHeadDeviceNumber)),
            example_config::kPollPoints,
            words,
            example_config::kPollPoints
        ) != slmp4e::Error::Ok) {
        printLastError("readWords");
        reconnect.forceReconnect(now_ms);
        delay(200);
        return;
    }

    Serial.printf("D100=%u D101=%u\n", words[0], words[1]);
}
