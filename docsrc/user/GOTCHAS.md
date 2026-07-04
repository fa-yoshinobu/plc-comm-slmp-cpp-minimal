# Gotchas

Use this page only for library-specific caveats.

Shared SLMP setup, profile, point-limit, and end-code symptoms live in the shared
[SLMP Troubleshooting & End Codes](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/slmp/troubleshooting-end-codes/)
page. For profile limits and device availability, use the shared
[SLMP Profile Parameters](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/parameters/)
page.

## Current library-specific caveats

| Area | Symptom | Guidance |
| --- | --- | --- |
| Diagnostics | Bytes are received, but the response is rejected as an invalid SLMP frame. | Verify the PLC Binary data-code setting and inspect `lastRequestFrame()` / `lastResponseFrame()` with `formatHexBytes()`. |
| Buffer sizing | A request fails locally with `BufferTooSmall`. | Increase the TX buffer first for block write, random write, or long password commands. Small direct reads/writes usually fit in `96` bytes; random and block access often need `192..256` bytes. |
| Header selection | Names such as `slmp::highlevel::readTyped` or `slmp::highlevel::Poller` are undefined. | Include `slmp_high_level.h` explicitly in any file that uses the high-level facade. |
