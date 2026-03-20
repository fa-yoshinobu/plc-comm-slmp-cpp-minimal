# W6300 txlimit Sweep Validation

## Scope

Validate the new upper-bound probing path for the W6300-EVB-Pico2 serial console.

## Change Summary

- Added `txlimit sweep [all|words|block]` to the W6300 console.
- The sweep iterates payload sizes upward until the first `BufferTooSmall` boundary is reached.
- Added the same sweep command to the PC-side serial CLI auto sequence.
- Added `txlimit sweep binary [all|words|block]` for binary-search-based boundary discovery, and invoked it via `txlimit sweep binary all` in the CLI auto sequence.

## Verification

- `python -m py_compile scripts/w6300_console_cli.py`
- `python .\scripts\w6300_console_cli.py --help`
- `python -m platformio run -e wiznet_6300_evb_pico2`

## Observed Build Result

- Build completed successfully for `wiznet_6300_evb_pico2`.
- Reported memory usage after build:
  - RAM: 13.9%
  - Flash: 8.1%

## Operational Notes

- `txlimit probe` still performs the exact-fit and one-over checks.
- `txlimit sweep all` is the new recommended command when the goal is to discover the highest transferable payload on the current `tx_buffer` configuration.
- Use `txlimit sweep binary all` when you want the same boundary but with fewer attempts (binary search instead of linear ramp).
- The sweep is a transport-side boundary test. It does not require changing the PLC program state beyond temporary test writes, and the console clears the tested word range after successful runs.

## Limitations

- No live PLC hardware run was performed in this validation pass.
- The sweep still depends on an active PLC connection and valid network parameters at runtime.
