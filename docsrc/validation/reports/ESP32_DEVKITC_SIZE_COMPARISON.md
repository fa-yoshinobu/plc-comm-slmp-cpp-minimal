# ESP32-DevKitC Size Comparison

- Date: 2026-03-27
- Board: `esp32dev` / ESP32-DevKitC
- Goal: compare the binary-size cost of enabling the optional high-level facade on the same transport, board, and compiler settings

## Build Commands

```bash
pio run -e esp32-devkitc-low-level
pio run -e esp32-devkitc-high-level
```

## Sample Roles

- `examples/esp32_devkitc_low_level`: direct fixed-buffer `SlmpClient` usage only
- `examples/esp32_devkitc_high_level`: same board and transport, but with `slmp_high_level.cpp`, explicit frame/mode selection, `connect`, `readTyped`, and `Poller`

## Measured Output

| Sample | Flash bytes | RAM bytes |
| --- | ---: | ---: |
| `esp32-devkitc-low-level` | 749717 | 45064 |
| `esp32-devkitc-high-level` | 772181 | 45184 |

## Delta

- Flash: `+22464` bytes
- RAM: `+120` bytes

## Interpretation

- The low-level sample is the size baseline for firmware that wants the fixed-buffer core only.
- The high-level sample shows the incremental cost of string address parsing, typed helpers, mixed snapshots, and the polling facade.
- This delta is small enough for many ESP32-class application builds, but the low-level-only path remains available for tighter image budgets.
