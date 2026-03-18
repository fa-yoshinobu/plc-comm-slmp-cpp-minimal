# Hardware Validation

Audience: library maintainers and validation follow-up work.

This file tracks both pending real-board coverage and dated validation records after the host-side and mock-PLC release checks.

Treat this file as a validation backlog and library-side evidence log, not as the main bring-up guide.

Use it together with:

- [../README.md](../README.md) for install steps and board entry points
- [../examples/README.md](../examples/README.md) to choose the sketch you want to validate
- [../TROUBLESHOOTING.md](../TROUBLESHOOTING.md) when a board run fails
- [hardware-validation issue template](../.github/ISSUE_TEMPLATE/hardware-validation.md) when you want to file a structured result

## Validation Flow

1. Pick the board and sketch from [../examples/README.md](../examples/README.md).
2. Run the read-only path first before any write-oriented checks.
3. Record board package version, network setup, PLC model, and local config changes.
4. Save frame dumps and end codes for any failure or unexpected response.
5. Open or update a validation issue with the captured data.

## Current Matrix

| Target | Transport | Bring-up sketch | Status | Notes |
|---|---|---|---|---|
| `m5stack-atom` | `WiFiClient` | `examples/atom_matrix_serial_console` | validated | real-board `check`, `funcheck`, `endurance 1000`, `bench pair 1000`, `bench block 300`, and `reconnect` recorded on 2026-03-14 against Mitsubishi iQ-R `R08CPU`; direct path passed, API path passed except mixed `writeBlock`, durability finished 1000/1000, pair benchmark averaged 18 ms per cycle, block benchmark averaged 17 ms per cycle, and reconnect recovered twice after transport errors |
| `wiznet_6300_evb_pico2` | `WiFiClient` / `WiFiUDP` via `W6300lwIP` | `examples/w6300_evb_pico2_serial_console` | partial | serial console now exposes `transport tcp|udp`, `frame 3e|4e`, and `txlimit sweep [all|words|block]` (linear and `binary` variants) plus BOOTSEL shortcuts; build verified on 2026-03-19, real-board capture still pending |
| real Mitsubishi PLC | TCP | any supported sketch | partial | Atom Matrix `check` and `funcheck` recorded against Mitsubishi iQ-R `R08CPU` on 2026-03-14; mixed `writeBlock` returned `0xC05B`, and the same first-pass result was later confirmed against the original Python implementation |

## Console Comparison Snapshot

| Target | Transport | `bench` command | `endurance` command | Latest recorded result | Notes |
|---|---|---|---|---|---|
| `m5stack-atom` | `WiFiClient` | yes | yes | `bench pair 1000`: `avg_cycle_ms=18`, `avg_req_ms=9`, `req_per_sec=110`, `max_ms=68`, `elapsed_ms=18161`; `bench block 300`: `avg_cycle_ms=17`, `avg_req_ms=8`, `req_per_sec=110`, `max_ms=45`, `elapsed_ms=5414`; `endurance 1000`: `ok=1000`, `fail=0`, `avg_ms=276`, `max_ms=366`, `elapsed_ms=296813`; `reconnect` peer-reset run: `attempts=188`, `fail=14`, `recoveries=2`, `max_consecutive_failures=10`, `elapsed_ms=66133`; `reconnect` PLC-reset run: `attempts=149`, `fail=5`, `recoveries=2`, `max_consecutive_failures=3`, `elapsed_ms=55260`; `reconnect` Wi-Fi power-off run: `attempts=214`, `fail=18`, `recoveries=1`, `max_consecutive_failures=18`, `elapsed_ms=190516` | real-board `check`, `funcheck`, `bench`, `endurance`, and `reconnect` recorded on 2026-03-14 |
| `wiznet_6300_evb_pico2` | `WiFiClient` / `WiFiUDP` via `W6300lwIP` | yes | no | build verified | `bench` command compiled and ready; no real-board benchmark or durability numbers recorded yet; serial transport/frame switching is now available from the console |

## Recorded Results

### 2026-03-14: Atom Matrix interactive console against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Result: the manual `check` scenario completed end-to-end on a real board and a real PLC
- Verified operator flow: `check` printed `guide`, `target`, and `command` before each write; the Atom front button accepted `OK` and advanced to the next step
- Confirmed on this PLC through the recorded run: `X`, `Y`, `SD`, `D`, `SM`, `M`, `L`, `F`, `V`, `B`, `W`, `TS`, `TC`, `TN`, `STS`, `STC`, `STN`, `CS`, `CC`, `CN`, `SB`, `SW`, `DX`, `DY`, `R`, `ZR`
- Observed as unsupported on this PLC and auto-skipped during the recorded run: `LTS`, `LTC`, `LTN`, `LSTS`, `LSTC`, `LSTN`, `S`, `Z`, `LZ`, `RD`, `G`, `HG`
- Library-side unsupported by design after this run: `LCS`, `LCC`, `LCN`
- Current shipped Atom Matrix scenario uses non-leading addresses such as `SD100`, `SM100`, `D100`, `M100`, `R200`, and `ZR300`
- Current shipped Atom Matrix scenario uses decimal write values and silently clears accepted `OK` writes back to `0`

### 2026-03-14: Atom Matrix `funcheck` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- `funcheck direct` result: `ok=26`, `skip=12`, `fail=0`
- `funcheck api` result: `ok=13`, `skip=0`, `fail=1`
- Confirmed on this PLC through the recorded `funcheck api` run:
  - `readTypeName`
  - `target` request-header reflection
  - `monitor` / `timeout` request-header reflection
  - `read/write` one-word, multi-word, one-bit, multi-bit, one-dword, multi-dword
  - `readRandom` / `writeRandomWords`
  - `writeRandomBits`
  - `readBlock` / `writeBlock` with `word-only`
  - `readBlock` / `writeBlock` with `bit-only`
- Observed failure on this PLC:
  - `readBlock` / `writeBlock` with mixed word + bit blocks in one request
  - Returned PLC end code: `0xC05B` (`direct_g_hg_path_rejected`) during the mixed `writeBlock` request
  - Test payload attempted to write `D300..D301` plus packed `M200..M215` in the same `1406/0002` frame
- Follow-up note:
  - the same mixed `writeBlock` scenario was later compared against the original Python implementation
  - the comparison result is summarized in [PYTHON_COMPARISON_CHECKLIST.md](./PYTHON_COMPARISON_CHECKLIST.md)
  - both implementations sent the same first-pass mixed request shape and hit the same `0xC05B` PLC end code

### 2026-03-14: Atom Matrix `endurance 1000` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Command: `endurance 1000`
- Result: completed `1000/1000` attempts with `ok=1000`, `fail=0`
- Elapsed time: `296813 ms` total for the full run
- Reported cycle timing summary from the board:
  - `avg_ms=276`
  - `last_cycle_ms=270`
  - `max_ms=366`
  - `min_ms=0` as printed by the current sketch output
- Device set exercised during the recorded run:
  - `D500` for `row/wow`
  - `D510..D511` for `rw/ww`
  - `M500` for `rb/wb`
  - `M510..M513` for `rbits/wbits`
  - `D600` for `rod/wod`
  - `D610..D613` for `rdw/wdw`
  - `D520`, `D521`, and `D620` for `wrand/rr`
- `M520`, `M521` for `wrandb`
- `D540..D541` and packed `M540` for `rblk/wblk`
- Observed stability: no disconnect, PLC error, or auto-stop event occurred during the recorded 1000-cycle run

### 2026-03-14: Atom Matrix `txlimit probe` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Command: `txlimit probe`
- Buffer assumptions printed by the board:
  - `tx_buffer_size=512`
  - `request_header_size=19`
  - `max_payload_fit=493`
- Calculated maximum-fit write sizes printed by the board:
  - `writeWords max_count=242`
  - `writeDWords max_count=121`
  - `writeBits max_points=970`
  - `writeRandomWords max_word_devices=61`
  - `writeRandomBits max_devices=61`
  - `writeBlock words-only max_points=241`
- Probed boundary results on the real board and PLC:
  - `writeWords exact_fit=ok count=242`
  - `writeWords one_over=buffer_too_small`
- `writeBlock words exact_fit=ok points=241`
- `writeBlock words one_over=buffer_too_small`
- Result: the library accepted exact-fit requests and rejected one-over requests locally with `buffer_too_small`, matching the current `tx_buffer` calculation logic

### 2026-03-14: Atom Matrix `bench pair 1000` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Command: `bench pair 1000`
- Result: completed `1000` cycles and `2000` requests with `fail=0`
- Reported timing summary from the board:
  - `elapsed_ms=18161`
  - `avg_cycle_ms=18`
  - `avg_req_ms=9`
  - `min_ms=14`
  - `max_ms=68`
  - `req_per_sec=110`
- Reported frame sizes at the end of the run:
  - `last_req_bytes=27`
  - `last_resp_bytes=17`
- Interpretation: the Atom Matrix sustained about `110` paired write+read request pairs per second against the recorded Mitsubishi iQ-R `R08CPU` setup

### 2026-03-14: Atom Matrix `bench block 300` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Command: `bench block 300`
- Result: completed `300` cycles and `600` requests with `fail=0`
- Reported timing summary from the board:
  - `elapsed_ms=5414`
  - `avg_cycle_ms=17`
  - `avg_req_ms=8`
  - `min_ms=13`
  - `max_ms=45`
  - `req_per_sec=110`
- Reported frame sizes at the end of the run:
  - `last_req_bytes=29`
  - `last_resp_bytes=47`
- Interpretation: the Atom Matrix sustained about `110` block write+read request pairs per second against the recorded Mitsubishi iQ-R `R08CPU` setup, with slightly lower average cycle time than the recorded `pair` run

### 2026-03-19: W6300 `txlimit sweep` console and CLI verification

- Target: `wiznet_6300_evb_pico2` with `examples/w6300_evb_pico2_serial_console`
- PLC connection: not exercised in this validation pass
- New command path: `txlimit sweep [all|words|block]` plus the faster `txlimit sweep binary [all|words|block]`
- PC-side runner: `scripts/w6300_console_cli.py`
- Verification performed:
  - `python -m py_compile scripts/w6300_console_cli.py`
  - `python .\scripts\w6300_console_cli.py --help`
  - `python -m platformio run -e wiznet_6300_evb_pico2`
- Result:
  - Console and CLI compiled successfully
  - The sweep command is available for runtime upper-bound testing once a PLC is connected

### 2026-03-14: Atom Matrix `reconnect` against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Command: `reconnect`
- Result at manual stop: `attempts=188`, `ok=174`, `fail=14`, `recoveries=2`
- Reported reconnect summary from the board:
  - `consecutive_failures=0` at stop
  - `max_consecutive_failures=10`
  - `elapsed_ms=66133`
  - `last_cycle_ms=18`
  - `avg_ms=101`
  - `max_ms=3004`
  - `min_ms=0` as printed by the current sketch output
- Observed transport behavior during the run:
  - first recovery occurred after `4` failed attempts
  - second recovery occurred after `10` failed attempts
  - repeated ESP32-side socket logs showed `errno: 104` (`Connection reset by peer`) while the PLC side was refusing or resetting the socket
  - the mode continued running through the failures and only stopped when the front button long-press was used
- Interpretation: the reconnect mode behaved as intended for recovery testing, counting failures without auto-stopping and confirming that the session could recover from at least two real transport loss windows against the recorded Mitsubishi iQ-R `R08CPU` setup

### 2026-03-14: Atom Matrix `reconnect` during PLC reset against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Trigger: PLC reset while `reconnect` was running
- Command: `reconnect`
- Result at manual stop: `attempts=149`, `ok=144`, `fail=5`, `recoveries=2`
- Reported reconnect summary from the board:
  - `consecutive_failures=0` at stop
  - `max_consecutive_failures=3`
  - `elapsed_ms=55260`
  - `last_cycle_ms=7`
  - `avg_ms=112`
  - `max_ms=3004`
  - `min_ms=0` as printed by the current sketch output
  - `retry_gap_ms=250` at stop after the failure streaks had cleared
- Observed transport behavior during the run:
  - the first PLC reset window recovered after `2` failed attempts
  - the second PLC reset window recovered after `3` failed attempts
  - the board logged `transport_error end=0x0` on the failing `readOneWord` cycle, then returned to steady 250 ms retry cadence after recovery
  - the mode continued running through both reset windows and only stopped when the front button long-press was used
- Interpretation: against the recorded Mitsubishi iQ-R `R08CPU` setup, PLC reset recovery was better than the earlier peer-reset run, with shorter failure streaks and successful return to steady-state reconnect timing after each reset

### 2026-03-14: Atom Matrix `reconnect` during Wi-Fi AP power-off against Mitsubishi iQ-R `R08CPU`

- Target: `m5stack-atom` with `examples/atom_matrix_serial_console`
- PLC model: Mitsubishi iQ-R `R08CPU`
- Transport: `WiFiClient`
- Trigger: Wi-Fi AP power-off while `reconnect` was running
- Command: `reconnect`
- Result at manual stop: `attempts=214`, `ok=196`, `fail=18`, `recoveries=1`
- Reported reconnect summary from the board:
  - `consecutive_failures=0` at stop
  - `max_consecutive_failures=18`
  - `elapsed_ms=190516`
  - `last_cycle_ms=10`
  - `avg_ms=320`
  - `max_ms=10013`
  - `min_ms=0` as printed by the current sketch output
  - `retry_gap_ms=250` at stop after the recovery had completed
- Observed transport behavior during the run:
  - while the AP was powered off, the board reported `wifi_status=no_ssid`, `local ip=0.0.0.0`, and repeated `wifi connect timeout`
  - after the AP came back, the board reported `wifi_status=connected` and recovered a valid local IP address
  - after Wi-Fi returned, PLC reconnect still failed with repeated ESP32-side `errno: 104` (`Connection reset by peer`) logs until the PLC itself was reset
  - after the PLC reset, the session recovered after `18` failed attempts and returned to steady 250 ms retry cadence
- Interpretation: from the recorded logs, Wi-Fi recovery alone was not enough for end-to-end recovery in this setup; the likely cause is a stale PLC-side session or socket that remained after the Wi-Fi-side power loss. This is an inference from the observed behavior, not a protocol-level proof.

## Suggested Issue Titles

- `hardware: validate atom matrix interactive serial console`
- `hardware: compare original python implementation against R08CPU mixed block write result`
- `hardware: validate W6300-EVB-Pico2 serial console`
- `hardware: verify real PLC end-code behavior against documented strings`

## Suggested Capture Data

When running a board validation pass, record:

- board and core package version
- transport type and network setup
- PLC model and firmware if known
- sketch path and any local configuration changes
- observed request/response frame dump on failure
- whether the same scenario passed against `scripts/mock_plc_server.py`
