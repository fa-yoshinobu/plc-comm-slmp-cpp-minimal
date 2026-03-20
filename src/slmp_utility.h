/**
 * @file slmp_utility.h
 * @brief High-level helper utilities for SLMP communication.
 * 
 * Includes components for automatic reconnection and state management.
 */

#ifndef SLMP_UTILITY_H
#define SLMP_UTILITY_H

#include <stdint.h>

#include "slmp_minimal.h"

namespace slmp {

/**
 * @struct ReconnectOptions
 * @brief Configuration for the ReconnectHelper.
 */
struct ReconnectOptions {
    uint32_t retry_interval_ms = 3000; ///< Minimum time between connection attempts.

    constexpr ReconnectOptions() = default;
    constexpr explicit ReconnectOptions(uint32_t retry_interval_ms_value)
        : retry_interval_ms(retry_interval_ms_value) {}
};

/**
 * @class ReconnectHelper
 * @brief Automates PLC connection maintenance.
 * 
 * Handles periodic retry logic when the connection is lost,
 * ensuring the application remains connected without blocking the main loop.
 */
class ReconnectHelper {
  public:
    /**
     * @brief Initialize helper with client and target.
     * @param client Reference to an initialized SlmpClient.
     * @param host Target IP address.
     * @param port Target Port.
     * @param options Reconnection configuration.
     */
    ReconnectHelper(
        SlmpClient& client,
        const char* host,
        uint16_t port,
        const ReconnectOptions& options = ReconnectOptions()
    )
        : client_(client),
          host_(host),
          port_(port),
          options_(options),
          last_attempt_ms_(0),
          has_attempt_(false),
          connected_edge_(false) {}

    /**
     * @brief Check and restore connection if needed.
     * @param now_ms Current system time in milliseconds.
     * @return true if currently connected (or just successfully reconnected).
     */
    bool ensureConnected(uint32_t now_ms) {
        connected_edge_ = false;
        if (client_.connected()) {
            return true;
        }
        if (host_ == nullptr || port_ == 0U) {
            return false;
        }
        if (has_attempt_) {
            const uint32_t elapsed = now_ms - last_attempt_ms_;
            if (elapsed < options_.retry_interval_ms) {
                return false;
            }
        }

        has_attempt_ = true;
        last_attempt_ms_ = now_ms;
        if (!client_.connect(host_, port_)) {
            return false;
        }

        connected_edge_ = true;
        return true;
    }

    /**
     * @brief Returns true if a NEW connection was established since the last call.
     * 
     * Useful for triggering one-time initialization (e.g., reading PLC type).
     */
    bool consumeConnectedEdge() {
        const bool value = connected_edge_;
        connected_edge_ = false;
        return value;
    }

    /**
     * @brief Manually trigger a reconnection attempt on the next update.
     */
    void forceReconnect(uint32_t now_ms) {
        client_.close();
        connected_edge_ = false;
        has_attempt_ = true;
        last_attempt_ms_ = now_ms - options_.retry_interval_ms;
    }

  private:
    SlmpClient& client_;
    const char* host_;
    uint16_t port_;
    ReconnectOptions options_;
    uint32_t last_attempt_ms_;
    bool has_attempt_;
    bool connected_edge_;
};

}  // namespace slmp

#endif
