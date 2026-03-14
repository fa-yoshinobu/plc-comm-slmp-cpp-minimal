#ifndef SLMP4E_UTILITY_H
#define SLMP4E_UTILITY_H

#include <stdint.h>

#include "slmp4e_minimal.h"

namespace slmp4e {

struct ReconnectOptions {
    uint32_t retry_interval_ms = 3000;

    constexpr ReconnectOptions() = default;
    constexpr explicit ReconnectOptions(uint32_t retry_interval_ms_value)
        : retry_interval_ms(retry_interval_ms_value) {}
};

class ReconnectHelper {
  public:
    ReconnectHelper(
        Slmp4eClient& client,
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

    bool consumeConnectedEdge() {
        const bool value = connected_edge_;
        connected_edge_ = false;
        return value;
    }

    void forceReconnect(uint32_t now_ms) {
        client_.close();
        connected_edge_ = false;
        has_attempt_ = true;
        last_attempt_ms_ = now_ms - options_.retry_interval_ms;
    }

  private:
    Slmp4eClient& client_;
    const char* host_;
    uint16_t port_;
    ReconnectOptions options_;
    uint32_t last_attempt_ms_;
    bool has_attempt_;
    bool connected_edge_;
};

}  // namespace slmp4e

#endif
