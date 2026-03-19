# Example Index

Use this file as the quick "which sketch should I start from?" guide.

For install steps and the overall library overview, go back to [../README.md](../README.md).

## Quick Picks

- Want the primary Wi-Fi interactive debug console: start with `atom_matrix_serial_console`.
- Want the primary Ethernet interactive debug console on WIZnet RP2350 hardware: start with `w6300_evb_pico2_serial_console`.

| Use case | Folder | Board/transport | What it shows |
|---|---|---|---|
| Atom Matrix interactive console | `atom_matrix_serial_console` | M5Stack Atom Matrix (`ESP32-PICO-D4`) + `WiFiClient` | shared Wi-Fi serial console plus Atom-specific `demo` mode for button-driven `D0` increment and 5x5 `M0..M24` LED mirroring |
| W6300-EVB-Pico2 interactive console | `w6300_evb_pico2_serial_console` | RP2350 + onboard W6300 + `WiFiClient` or `WiFiUDP` via `W6300lwIP` | primary Ethernet validation console with direct, random, block, `funcheck`, `endurance`, `reconnect`, `txlimit`, password, and frame-dump checks plus serial `transport` / `frame` switching, BOOTSEL shortcuts, and `txlimit sweep` upper-bound probing (normal and fast binary-search variants) |

Suggested order:

1. Start with `atom_matrix_serial_console` for Wi-Fi-side validation.
2. Use `w6300_evb_pico2_serial_console` for Ethernet-side validation on RP2350 + W6300 hardware.
3. In the W6300 console, use `transport tcp|udp` and `frame 3e|4e` to switch protocol modes without reflashing.

## Related Docs

- [../README.md](../README.md): install, quick start, board-by-board overview, memory notes
- [../docs/user/TROUBLESHOOTING.md](../docs/user/TROUBLESHOOTING.md): network, buffer, and frame-dump troubleshooting
