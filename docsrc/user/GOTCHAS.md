# Gotchas

Use this page as a short symptom index. For PLC response codes, use the shared
[SLMP Troubleshooting & End Codes](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/slmp/troubleshooting-end-codes/)
page. For profile limits and device availability, use the shared
[SLMP Profile Parameters](https://fa-yoshinobu.github.io/plc-comm-docs-site/slmp/profile-reference/parameters/)
page.
For PLC-side Ethernet settings, use the shared
[MELSEC SLMP PLC Setup Guide](https://fa-yoshinobu.github.io/plc-comm-docs-site/plc-setup/slmp/).
Check Binary communication data code, port/open settings, and RUN-time write permission there before debugging application code.

## `connect()` or transport I/O fails

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `connect()` fails, a transport call times out, or `TransportError` is returned before a valid SLMP response is parsed. | PLC IP address, port, transport path, or local network route is wrong. | Check the PLC setup guide first. The usual built-in Ethernet TCP port is `1025`; use UDP only when the PLC is configured for UDP. |

## `ProtocolError` is returned

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Bytes are received, but the response is rejected as an invalid SLMP frame. | The endpoint is not speaking the expected SLMP binary frame, or the response length does not match the request. | Verify the PLC Binary data-code setting and inspect `lastRequestFrame()` / `lastResponseFrame()` with `formatHexBytes()`. |

## `BufferTooSmall` is returned

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A request fails locally with `BufferTooSmall`. | Caller-owned TX/RX buffers are too small for the selected command and point count. | Increase the TX buffer first for block write, random write, or long password commands. Small direct reads/writes usually fit in `96` bytes; random and block access often need `192..256` bytes. |

## Large requests fail with point-limit end codes

| Symptom | Root cause | Fix |
| --- | --- | --- |
| A large read, write, random request, or monitor request fails with `C051`, `C052`, `C053`, or `C054`. | The request exceeds the selected profile's per-request point limit. | Split the request or use the chunked helper. Check the shared profile parameter table for the limit. |

```cpp
std::vector<uint16_t> words;
const slmp::Error err = slmp::highlevel::readWordsChunked(plc, "D1000", 2000, words, 480, true);
```

## Mixed word and bit write fails

| Symptom | Root cause | Fix |
| --- | --- | --- |
| One write containing word values and bit values fails. | Some PLC paths reject mixed word and bit block writes. | Send word writes and bit writes as separate calls. |

## iQ-F X/Y or DX/DY addresses fail

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `X`/`Y` points look shifted, or `DX`/`DY` is rejected on iQ-F. | iQ-F uses octal text for `X`/`Y`, and the iQ-F profile does not support `DX`/`DY`. | Use profile-aware high-level parsing with `slmp::highlevel::PlcProfile::IqF`; use `X` and `Y` on iQ-F. |

```cpp
slmp::highlevel::AddressSpec spec{};
slmp::highlevel::parseAddressSpec("X20", slmp::highlevel::PlcProfile::IqF, spec);
```

## Long timer/counter/index values look wrong

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `LTN`, `LSTN`, `LCN`, or `LZ` looks truncated or shifted. | These current-value families are 32-bit values. | Use `:D` or `:L` in high-level addresses. |
| `LCS` or `LCC` behaves unlike a word value. | Long counter state devices are bits. | Read or write them as `:BIT`. |

```cpp
slmp::highlevel::readTyped(plc, kProfile, "LTN0:D", value);
```

## High-level helpers are not found by the compiler

| Symptom | Root cause | Fix |
| --- | --- | --- |
| Names such as `slmp::highlevel::readTyped` or `slmp::highlevel::Poller` are undefined. | `slmp_high_level.h` is optional and is not included by `slmp_minimal.h`. | Include `slmp_high_level.h` explicitly in any file that uses the high-level facade. |

## D50.D reads bit 13 instead of a 32-bit value

| Symptom | Root cause | Fix |
| --- | --- | --- |
| `D50.D` behaves like one bit rather than a double word. | Dot notation is bit-in-word access; `D` after the dot is hexadecimal bit index 13. | Use `D50:D` for unsigned 32-bit data and reserve `D50.D` for bit 13. |
