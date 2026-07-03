# Latest communication verification

Current live-device verification for `plc-comm-slmp-cpp-minimal` is summarized here.

## Latest Verified Set

| Date | Target | PLC / CPU | Canonical profile | Transport | Verified scope | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-03-14 | `m5stack-atom` | MELSEC iQ-R `R08CPU` | `melsec:iq-r` | `WiFiClient` | `check`, `funcheck`, pair/block benchmark, endurance, reconnect recovery | Real-board path passed. |
| 2026-03-19 | `wiznet_6300_evb_pico2` | MELSEC iQ-R `R120PCPU` | `melsec:iq-r` | `WiFiClient` / `WiFiUDP` via `W6300lwIP` | 3E/4E frame switching, block benchmark, `funcheck` | Real-board path passed. |
| 2026-03-19 | `wiznet_6300_evb_pico2` | MELSEC Q-series `Q06UDVCPU` | `melsec:qnudv` | `WiFiClient` via `W6300lwIP` | Legacy 3E compatibility check | Initial compatibility failures were resolved with the Legacy compatibility mode. |
