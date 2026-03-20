# Validation Report: Q-Series (Legacy SLMP) Compatibility

- **Date**: 2026-03-19
- **Target**: `src/slmp_minimal.cpp`, `examples/w6300_evb_pico2_serial_console`
- **Feature**: `CompatibilityMode` for Q/L Series (Legacy SLMP)
- **Status**: **SUCCESS** (All supported operations passed)

## 1. Issue Analysis (Q06UDVCPU)

Testing against a Mitsubishi **Q06UDVCPU** revealed three critical differences in Legacy SLMP:
1.  **Frame**: Q-series built-in Ethernet only supports 3E frames.
2.  **Device Format**: Requires 3-byte address + 1-byte device code (Total 4 bytes, vs 6 bytes in iQ-R).
3.  **Random Bit Write**: Values must be 1-byte per bit (vs 2-bytes in iQ-R).

## 2. Solution: CompatibilityMode

Introduced `slmp::CompatibilityMode` to dynamically adjust the packet structure.

### Implementation Details:
- `encodeDeviceSpec`: Switched between 4-byte (Legacy) and 6-byte (Modern) formats.
- `beginWriteRandomBits`: Adjusted value packing to 1-byte for Legacy mode.
- `startAsync`: Switched subcommands to `0x0000`/`0x0001` for Legacy mode.

## 3. Verified Result (2026-03-19)

Using `frame 3e` + `compat legacy`, the following were verified on real hardware:
- **Direct Word/Bit R/W**: PASS
- **API Basic/DWord R/W**: PASS
- **Random Word/Bit Write**: PASS (Fixed packing issue)

*Note: TypeName and Block commands are hardware-limited on Q06UDV and return 0xC059, which is expected.*

## 4. Conclusion

The library now structurally supports both modern (iQ-R) and legacy (Q/L) SLMP protocol variants. The addition of the `compat` command provides the necessary flexibility for heterogeneous PLC environments.
