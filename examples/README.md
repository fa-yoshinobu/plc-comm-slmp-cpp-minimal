# Example Index

Use this file as the quick entry point for example sketches.

| Use case | Folder | Board/transport | What it shows |
|---|---|---|---|
| Basic direct read | `esp32_read_words` | ESP32 + `WiFiClient` | `connect()`, `readTypeName()`, `readWords()`, frame dump |
| Random and block access | `esp32_random_block` | ESP32 + `WiFiClient` | `readRandom()`, `readBlock()`, optional write examples |
| Dynamic bit walk | `esp32_dynamic_bits` | ESP32 + `WiFiClient` | dynamic `M100..M500` writes with odd-address filtering |
| Reconnect + password | `esp32_password_read_loop` | ESP32 + `WiFiClient` | reconnect loop, password unlock, type-name read, periodic polling, `config.h` for deployment settings |
| RP2040 Ethernet basic read | `rp2040_w5500_read_words` | RP2040 + `EthernetClient` | same core API with W5500 transport |

Suggested order:

1. Start with `esp32_read_words`.
2. Move to `esp32_password_read_loop` if you need a more complete session pattern with a separate `config.h`.
3. Use `esp32_random_block` or `esp32_dynamic_bits` for specialized access patterns.
