# Validation Report: 3E Frame Support

- **Date**: 2026-03-18
- **Target**: SLMP 3E Frame implementation and GX Simulator 3 compatibility
- **Status**: SUCCESS (100% Pass)

## 1. Feature Overview

Added support for SLMP 3E binary frames to the C++ minimal client. This allows communication with environments that require 3E subheaders (0x5000) instead of 4E (0x5400).

### Technical Specifications
- **Frame Selection**: Via `plc.setFrameType(slmp4e::FrameType::Frame3E)`.
- **iQ-R Compatibility**: While using 3E headers, the payload maintains iQ-R extension specifications (6-byte device points, subcommands 0x0002/0x0003) to ensure stability with GX Simulator 3 and modern iQ-R/iQ-F series.
- **Header Adjustments**: Automatically handles different offsets for Network No, Station No, and Data Length between 3E and 4E formats.

## 2. Validation Environment

- **Simulator**: GX Simulator 3 (GX Works3)
- **Connection**: `127.0.0.1:5511` (System 1, CPU 1)
- **Device Model**: R08CPU

## 3. Test Results

An exhaustive cross-frame test was performed where data was written using one frame type and read back using the other.

| Test Category | Pattern | Devices | Result |
|:---|:---|:---|:---|
| Single R/W Cross | 3E Write -> 4E Read | SM, SD, X, Y, M, L, F, V, B, D, W, TN, STN, CN, SB, SW, DX, DY, R, ZR | **PASS** (20/20) |
| Single R/W Cross | 4E Write -> 3E Read | SM, SD, X, Y, M, L, F, V, B, D, W, TN, STN, CN, SB, SW, DX, DY, R, ZR | **PASS** (20/20) |
| Block Access Cross | 3E Write -> 4E Read | D500 (1 word) | **PASS** (1/1) |
| **Total** | | | **41/41 PASS** |

## 4. Stability Notes

- **Random Access**: Both 3E and 4E correctly handle random word/bit access.
- **Subcommands**: Verified that `0x0002` (Word) and `0x0003` (Bit) are the most compatible subcommands for GX Simulator 3 even when using 3E frames.
- **Buffer Integrity**: `writeBlock` and `readBlock` logic was hardened with improved capacity checks.

---
*End of Report*
