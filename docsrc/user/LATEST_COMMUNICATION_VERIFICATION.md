# Latest communication verification

Live-device verification for `plc-comm-slmp-cpp-minimal` is summarized here.
Detailed board evidence, command logs, historical notes, and follow-up items are
maintained in [Hardware validation](../validation/reports/HARDWARE_VALIDATION.md).

## Latest Verified Set

| Date | Target | PLC / CPU | API profile | Transport | Verified scope | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-03-14 | `m5stack-atom` | MELSEC iQ-R `R08CPU` | `slmp::highlevel::PlcProfile::IqR` | `WiFiClient` | `check`, `funcheck`, pair/block benchmark, endurance, reconnect recovery | Real-board path passed. Historical mixed `writeBlock` notes in the detailed log predate the payload layout fix. |
| 2026-03-19 | `wiznet_6300_evb_pico2` | MELSEC iQ-R `R120PCPU` | `slmp::highlevel::PlcProfile::IqR` | `WiFiClient` / `WiFiUDP` via `W6300lwIP` | 3E/4E frame switching, block benchmark, `funcheck` | See the detailed log for operation-level notes and retained frame observations. |
| 2026-03-19 | `wiznet_6300_evb_pico2` | MELSEC Q-series `Q06UDVCPU` | `slmp::highlevel::PlcProfile::QnUDV` | `WiFiClient` via `W6300lwIP` | Legacy 3E compatibility check | Initial compatibility failures were resolved with the Legacy compatibility mode. |

## Retained Evidence

Use [Hardware validation](../validation/reports/HARDWARE_VALIDATION.md) for:

- board package versions and network setup notes
- frame dumps and end-code observations
- older validation records that are kept as historical evidence
- follow-up notes for future real-board validation
