# Developer Notes: SLMP C++ Minimal

Internal design details, testing procedures, and resource metrics for maintainers.

## 1. Memory Model

The core library follows a strict **No-Dynamic-Allocation** policy to ensure stability on embedded systems.
- All TX/RX buffers are owned by the caller.
- Memory footprint is predictable and static.

The optional high-level facade in `slmp_high_level.h` is different by design:

- it is user-facing convenience API
- it uses `std::string` and `std::vector`
- it depends only on the public `SlmpClient` core API
- it can be excluded with `SLMP_MINIMAL_ENABLE_HIGH_LEVEL=0`

## 2. Function Size Matrix (ESP32)

Measured on an ESP32-based Arduino target. RAM delta is 0 as all buffers are caller-owned.

| Function set | Flash delta | RAM delta |
|---|---:|---:|
| baseline (`readTypeName`) | `+0` | `0` |
| direct read/write | `+2,068` | `0` |
| password unlock/lock | `+316` | `0` |
| end code text decoding | `+728` | `0` |
| random read/write | `+1,440` | `0` |
| block read/write | `+1,392` | `0` |

## 3. Host Tests & CI

You can verify the protocol core on a desktop PC without a real PLC.

### Running Tests
Use the PlatformIO environment or a local GCC:
```powershell
# Clone plc-comm-slmp-cross-verify next to this repo first, or pass --cross-verify-dir
# Run all host-side tests
python scripts\run_function_tests.py --compiler g++
```

The host tests regenerate `tests/generated_shared_spec.h` from
`../plc-comm-slmp-cross-verify/specs/shared` before building. Use
`--cross-verify-dir <path>` if the verification repo lives elsewhere.

### Coverage
The host tests cover:
- Protocol framing/unframing for all supported device families.
- Endianness handling.
- Timeout and error state transitions in the async state machine.
- Mock PLC server integration (`scripts/mock_plc_server.py`).

## 4. Directory Layout

- `src/`: Core C++ source and headers.
- `src/slmp_high_level.*`: Optional string-address and snapshot facade.
- `scripts/`: Python utilities for testing, size reporting, and mock servers.
- `tests/`: G++ host tests and integration tests.
- `examples/`: Ready-to-use Arduino sketches.
