# Python Comparison Checklist

Audience: library maintainers and validation follow-up work.

This file consolidates the earlier root-level `PYTHON_COMPARISON_CHECKLIST.md` and `slmp_connect_python_comparison_checklist.md` into one developer-facing note.

The original Python project referenced by this repository is:

- `slmp-connect-python`: <https://github.com/fa-yoshinobu/plc-comm-slmp-python>

## Purpose

Use this note when you want to compare the original Python implementation with the current C++ library on the same PLC and the same block-access scenarios.

The relevant C++ baseline in this repository is:

- mixed block support in the codec and request builder ([../../src/slmp_minimal.cpp](../../src/slmp_minimal.cpp))
- mock-PLC integration coverage that accepts mixed `writeBlock()` ([../../tests/slmp_socket_integration.cpp](../../tests/slmp_socket_integration.cpp))
- a real-board Atom Matrix result against Mitsubishi iQ-R `R08CPU` showing that a one-request mixed `writeBlock` was rejected on the real PLC ([HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md))

## Confirmed Answer

The 2026-03-14 live Python comparison showed:

- Python sends the same one-request mixed `1406/0002` frame shape for `D300 x2` plus `M200 x1 packed`
- the first mixed write fails on the same PLC with the same `0xC05B`
- word-only and bit-only block writes both pass
- `retry_mixed_on_error=True` recovers by retrying as separate word-only and bit-only block writes

This means the observed PLC rejection is not unique to the C++ encoder. The original Python implementation hit the same PLC-side failure for the same first-pass mixed request shape.

## Current C++ Baseline

Recorded board:

- `m5stack-atom`
- sketch: `examples/atom_matrix_serial_console`
- PLC: Mitsubishi iQ-R `R08CPU`
- date: `2026-03-14`

Current result summary:

| Scenario | C++ result | Notes |
|---|---|---|
| `readBlock/writeBlock words` | pass | `word-only` block write succeeded |
| `readBlock/writeBlock bits` | pass | `bit-only` block write succeeded |
| `readBlock/writeBlock mixed` | fail | PLC returned `0xC05B` during mixed `writeBlock` |

Mixed write case used:

- word block device: `D300`, `2 points`
- bit block device: `M200`, `1 packed word`
- effective bit range: `M200..M215`

Sample failing C++ mixed request captured from the real PLC run:

```text
last request: 54 00 99 00 00 00 00 FF FF 03 00 1E 00 10 00 06 14 02 00 01 01 2C 01 00 00 A8 00 02 00 C8 00 00 00 90 00 01 00 F7 87 BE 80 FE 6D
last response: D4 00 99 00 00 00 00 FF FF 03 00 0B 00 5B C0 00 FF FF 03 00 06 14 02 00
```

Interpreted meaning:

- command: `1406`
- subcommand: `0002`
- word blocks: `1`
- bit blocks: `1`
- word block: `D300`, `2 points`
- bit block: `M200`, `1 packed word`

## Comparison Conditions

Keep the Python-side check as close as possible to the C++ run:

- same PLC: Mitsubishi iQ-R `R08CPU`
- same network path and target settings
- same device addresses
- same point counts
- same packed-bit interpretation for block bit writes
- capture both request hex and response hex

Important first-pass rule:

- keep `split_mixed_blocks=False`
- keep `retry_mixed_on_error=False`
- capture the original one-request mixed frame and the first PLC response before any fallback behavior is enabled

Recommended device set:

- word-only block write: `D300`, `2 points`
- bit-only block write: `M200`, `1 packed word`
- mixed block write: `D300`, `2 points` plus `M200`, `1 packed word`

## Recorded Comparison Result

| Scenario | Devices | Expected comparison target | Python result | End code | Notes |
|---|---|---|---|---|---|
| `readBlock words+bits` | `D300 x2`, `M200 x1 packed` | C++ pass | `same_as_cpp_pass` | `0x0000` | request succeeded; returned `D300=[0x0000, 0x0000]`, `M200=[0x0000]` |
| `writeBlock words only` | `D300 x2` | C++ pass | `same_as_cpp_pass` | `0x0000` | write/readback/restore all `OK` |
| `writeBlock bits only` | `M200 x1 packed` | C++ pass | `same_as_cpp_pass` | `0x0000` | write/readback/restore all `OK` |
| `writeBlock mixed` | `D300 x2` + `M200 x1 packed` | C++ fail with `0xC05B` | `same_as_cpp_fail_same_end_code` | `0xC05B` | first one-request mixed write failed; PLC memory remained unchanged |

## Fallback Verification

The practical workaround was also checked live after the first-pass capture.

| Scenario | API/options | Python result | End code(s) | Notes |
|---|---|---|---|---|
| `writeBlock mixed fallback` | `write_block(..., retry_mixed_on_error=True)` | `different_runtime_behavior` | `0xC05B -> 0x0000 -> 0x0000` | first mixed write failed, then automatic split retry succeeded and restore was `OK` |

## Current C++ Follow-up

The minimal C++ library now exposes the same practical fallback shape for the
synchronous API:

- `slmp::BlockWriteOptions::split_mixed_blocks`
- `slmp::BlockWriteOptions::retry_mixed_on_error`

Current scope:

- implemented and host-tested in both `slmp::SlmpClient::writeBlock(...)` and `beginWriteBlock(..., options, now_ms)`
- retry end-code set matches the Python implementation: `0xC056`, `0xC05B`, `0xC061`
- typed remote control helpers now cover `remoteRun`, `remoteStop`, `remotePause`, and `remoteLatchClear`
- typed reset, self-test, and clear-error helpers now cover `remoteReset`, `selfTestLoopback`, and `clearError`
- passive `recommendProfile(...)` / `applyProfileRecommendation(...)` helpers now provide a lightweight model-based default profile suggestion without active probing
- no new hardware-validation claim is made by this implementation change alone

## Developer Implications

- The historical one-request mixed `writeBlock` failure is a PLC-side interoperability issue seen by both implementations, not just by this C++ library.
- Any library-side mitigation should focus on request-shape control or fallback behavior rather than on re-encoding the same combined frame.
- If future development changes C++ mixed-write behavior, update [HARDWARE_VALIDATION.md](../validation/reports/HARDWARE_VALIDATION.md) together with this note so the historical result and the current shipped behavior stay aligned.

## If You Need To Re-run The Comparison

Use the smallest safe deltas first:

1. Compare mixed `readBlock` (`0406`) against mixed `writeBlock` (`1406`) on the same `D300` + `M200` pair.
2. Compare `TCP 1025` and `UDP 1027`.
3. Compare PLC `RUN` and `STOP` if the target devices are safe in both states.
4. Compare alternate base devices such as `D100/M100`, `D1000/M1000`, and `D300/M200`.
5. Compare alternate mixed pairs such as `D+B`, `W+M`, or `R+M` if those devices are writable on the target.
6. Vary point counts to see whether rejection depends on the mixed shape rather than on the device family alone.

Expected outcome labels:

- `same_as_cpp_pass`
- `same_as_cpp_fail_same_end_code`
- `same_as_cpp_fail_different_end_code`
- `different_request_shape`
- `different_runtime_behavior`
