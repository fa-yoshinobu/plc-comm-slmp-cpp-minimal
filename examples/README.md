
[![Documentation](https://img.shields.io/badge/docs-GitHub_Pages-blue.svg)](https://fa-yoshinobu.github.io/plc-comm-slmp-cpp-minimal/)

Use this file as the quick "which sketch should I start from?" guide.

For install steps and the overall library overview, go back to `README.md` in the repository root.

The interactive console sketches were moved to the companion repository:

- <https://github.com/fa-yoshinobu/plc-comm-slmp-cpp-minimal-console-app>

## Quick Picks

- Want the smallest compile-checked high-level facade sample: start with `high_level_snapshot`.
- Want the smallest direct firmware sample: start with `esp32_devkitc_low_level`.
- Want the smallest high-level firmware sample: start with `esp32_devkitc_high_level`.

| Use case | Folder | Board/transport | What it shows |
|---|---|---|---|
| ESP32 low-level size baseline | `esp32_devkitc_low_level` | ESP32-DevKitC + `WiFiClient` | smallest direct core example using `SlmpClient`, fixed buffers, and `DeviceAddress` helpers |
| ESP32 high-level size sample | `esp32_devkitc_high_level` | ESP32-DevKitC + `WiFiClient` | same board and transport, but using explicit `setFrameType`, `setCompatibilityMode`, `connect`, plus `readTyped` and `Poller` so the binary size delta is easy to compare |
| High-level snapshot sample | `high_level_snapshot` | host-side compile smoke | `readTyped`, `readNamed`, `writeNamed`, and `Poller` usage in one compile-checked sample |

Suggested order:

1. Start with `esp32_devkitc_low_level` and `esp32_devkitc_high_level` if you want a clean size comparison on the same board.
2. Start with `high_level_snapshot` if you want the host-side high-level API surface first.
3. Use the companion console-app repository only if you need interactive bring-up tools.

## ESP32 Size Comparison

Build commands:

```bash
pio run -e esp32-devkitc-low-level
pio run -e esp32-devkitc-high-level
```

Then compare the generated binaries:

- `.pio/build/esp32-devkitc-low-level/firmware.bin`
- `.pio/build/esp32-devkitc-high-level/firmware.bin`

The low-level sample intentionally avoids `slmp_high_level.cpp`.
The high-level sample intentionally includes it and uses explicit frame/mode selection,
`connect`, `readTyped`, and `Poller`, so the size difference is easy to observe.
Large contiguous reads stay behind explicit chunked helpers instead of appearing as hidden fallback behavior.

Current reference build numbers:

- low-level sample: Flash `749717` bytes, RAM `45064` bytes
- high-level sample: Flash `772181` bytes, RAM `45184` bytes
- delta: Flash `+22464` bytes, RAM `+120` bytes

Full measurement notes are stored in `docsrc/validation/reports/ESP32_DEVKITC_SIZE_COMPARISON.md`.

## What To Learn From Each Sample

### `high_level_snapshot`

Use this first if you want the user-facing helper surface.

It demonstrates:

- `readTyped`
- `writeTyped`
- `readNamed`
- `writeNamed`
- `Poller`
- `parseAddressSpec`

It is intentionally small and compile-checked so you can copy it into a host or firmware project with minimal cleanup.

### `esp32_devkitc_low_level`

Use this when you want the smallest direct Arduino example on ESP32.

It demonstrates:

- `WiFiClient` transport setup
- fixed TX/RX buffers
- `SlmpClient`
- `setFrameType`
- `setCompatibilityMode`
- `readOneWord`
- `readOneBit`

### `esp32_devkitc_high_level`

Use this when you want the ESP32 equivalent of the user-facing Python and .NET helper style.

It demonstrates:

- explicit `setFrameType`
- explicit `setCompatibilityMode`
- explicit `connect`
- `readTyped`
- `Poller`
- string addresses such as `D100`, `D200:F`, and `D50.3`
- the binary size cost of enabling the optional high-level layer

## Related Docs

- `README.md`: install, quick start, board-by-board overview, memory notes
- `docsrc/user/TROUBLESHOOTING.md`: network, buffer, and frame-dump troubleshooting



