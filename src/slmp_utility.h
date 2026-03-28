/**
 * @file slmp_utility.h
 * @brief High-level helper utilities for SLMP communication.
 * 
 * Includes components for automatic reconnection, state management, and 
 * simplified application patterns.
 */

#ifndef SLMP_UTILITY_H
#define SLMP_UTILITY_H

#include <stdint.h>

#include "slmp_minimal.h"

namespace slmp {

/**
 * @struct ReconnectOptions
 * @ingroup SLMP_Utils
 * @brief Configuration for the @ref ReconnectHelper.
 */
struct ReconnectOptions {
    /** 
     * @brief Minimum time between connection attempts. 
     * Prevents hammering the network or PLC when the connection is physically down.
     */
    uint32_t retry_interval_ms = 3000;

    constexpr ReconnectOptions() = default;
    constexpr explicit ReconnectOptions(uint32_t retry_interval_ms_value)
        : retry_interval_ms(retry_interval_ms_value) {}
};

/**
 * @class ReconnectHelper
 * @ingroup SLMP_Utils
 * @brief Automates PLC connection maintenance.
 * 
 * Handles periodic retry logic when the connection is lost,
 * ensuring the application remains connected without blocking the main loop.
 * 
 * ### Example Usage
 * @code
 * slmp::ReconnectHelper recon(plc, "192.168.250.100", 1025);
 * 
 * void loop() {
 *     if (recon.ensureConnected(millis())) {
 *         if (recon.consumeConnectedEdge()) {
 *             // Do one-time setup on connect
 *         }
 *         // Normal polling
 *     }
 * }
 * @endcode
 */
class ReconnectHelper {
  public:
    /**
     * @brief Initialize helper with client and target endpoint.
     * @param client Reference to an initialized @ref SlmpClient.
     * @param host Target IP address or hostname.
     * @param port Target Port (usually 1025 or 1280).
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
     * 
     * If already connected, returns true immediately.
     * If disconnected, attempts to connect only if @ref ReconnectOptions::retry_interval_ms 
     * has elapsed since the last attempt.
     * 
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
     * @brief Detect a rising edge of the connection state.
     * 
     * Returns true only ONCE when a new connection is established.
     * Useful for triggering initialization logic like reading PLC type or setting up monitors.
     * 
     * @return true if a NEW connection was established since the last call to this method.
     */
    bool consumeConnectedEdge() {
        const bool value = connected_edge_;
        connected_edge_ = false;
        return value;
    }

    /**
     * @brief Manually trigger a reconnection attempt on the next call to @ref ensureConnected.
     * 
     * Closes the current connection and resets the retry timer to allow 
     * an immediate attempt if desired.
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
