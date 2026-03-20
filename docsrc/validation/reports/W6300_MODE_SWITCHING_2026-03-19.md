# Validation Report: W6300 Serial Console Mode Switching

- **Date**: 2026-03-19
- **Target**: `examples/w6300_evb_pico2_serial_console`
- **Status**: BUILD VERIFIED

## 1. Change Summary

Added console-side switching for:

- transport: `tcp` / `udp`
- frame type: `3e` / `4e`

The console also uses the BOOTSEL button as a quick operator shortcut for the same mode toggles and LED status indication.

## 2. Validation Environment

- **Workspace**: `d:/PLC_COMM_PROJ/plc-comm-slmp-cpp-minimal`
- **Board target**: `wiznet_6300_evb_pico2`
- **Framework**: Arduino-Pico with `W6300lwIP`

## 3. Verification

Build command:

```bash
python -m platformio run -e wiznet_6300_evb_pico2
```

Result:

- **PASS**
- RAM: `13.9%` used, `72664` bytes of `524288`
- Flash: `8.0%` used, `168200` bytes of `2093056`

PC CLI check:

```bash
python -m py_compile scripts/w6300_console_cli.py
```

Result:

- **PASS**
- The PC CLI script is syntax-valid and ready to drive the serial console with `--auto` or `--auto-full`.

## 4. Notes

- `transport udp` is available in this build and routes through `ArduinoUdpTransport`.
- `frame 3e` and `frame 4e` switch the `slmp::SlmpClient` frame type without reflashing.
- No real-board PLC session was performed in this pass.
