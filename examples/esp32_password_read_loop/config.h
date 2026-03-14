#ifndef SLMP4E_EXAMPLE_ESP32_PASSWORD_READ_LOOP_CONFIG_H
#define SLMP4E_EXAMPLE_ESP32_PASSWORD_READ_LOOP_CONFIG_H

#include <stddef.h>
#include <stdint.h>

namespace example_config {

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

constexpr char kPlcHost[] = "192.168.250.101";
constexpr uint16_t kPlcPort = 1025;
constexpr char kPlcPassword[] = "123456";

constexpr uint32_t kReconnectIntervalMs = 3000;
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kWifiConnectTimeoutMs = 10000;

constexpr size_t kTxBufferSize = 128;
constexpr size_t kRxBufferSize = 128;

constexpr uint32_t kPollHeadDeviceNumber = 100;
constexpr uint16_t kPollPoints = 2;

}  // namespace example_config

#endif
