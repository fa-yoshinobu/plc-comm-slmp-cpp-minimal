# SLMP C++ Minimal Quality Overhaul

This record preserves the approved target contract and verification state for the breaking quality overhaul. User documentation describes only the resulting supported behavior.

## CPP-OH-001 — Required connection identity

Scope: `SlmpClient`, `TargetAddress`, connection setup, examples, and generated API documentation.

Target contract: construction requires one concrete PLC profile and all four target-route fields. The target is fixed for the client lifetime. Port remains required by `connect`; zero, empty host, invalid profile, and unusable buffers fail before transport.

Compatibility impact: the former five-argument constructor, post-construction target setter, and independent `setFrameType`／`setCompatibilityMode` controls are removed. Controlled manual frame selection uses `setManualProfile(profile, frame, compatibility)` so the concrete profile is never discarded.

Acceptance criteria:

1. Constructing a client without profile or target is a compile-time error.
2. `PlcProfile::Unspecified`, empty host, and port zero cannot reach transport connection or request transmission.
3. Generated API documentation contains the required constructor arguments and no target setter.
4. Runnable socket validation, including the write-capable matrix tool, requires explicit host and port and rejects a missing/out-of-range port before connection.
5. Normal profile selection derives frame/compatibility, while manual selection requires the concrete profile in the same call; no public per-axis setter can create an `Unspecified` client.
6. Unknown manual frame/compatibility enum values and all profile changes during an active request are rejected without changing profile, frame, or compatibility state.

## CPP-OH-002 — Profile-bound semantic addresses

Scope: `DeviceAddress`, `AddressSpec`, device factories, standalone address utilities, read plans, and every direct-device send path.

Target contract: semantic addresses immutably retain their canonical PLC profile, device code, and wire number. Read-only `profile()`, `code()`, and `number()` accessors expose those values. Standalone parsing, formatting, normalization, and plan compilation require a profile. A client/address or client/plan profile mismatch returns `InvalidArgument` before transport.

Compatibility impact: profileless device factories and standalone address overloads are removed. Public field reads migrate to the accessors; direct field mutation is removed, so changing any component requires constructing a new semantic address.

Acceptance criteria:

1. `X10` parsed for iQ-R retains numeric 16; the iQ-F parse retains numeric 8.
2. Formatting an address with a different profile is rejected.
3. Direct, random, block, monitor, high-level named, and compiled-plan paths reject profile mismatch without sending.
4. Generated API documentation exposes only the required constructor and read-only accessors; no public mutable profile, code, or number field exists.

## CPP-OH-003 — One call, one request

Scope: named/random reads, continuous operations, and mixed block writes.

Target contract: no public chunked API, hidden chunk loop, split option, split fallback, or automatic retry may convert one library call into multiple SLMP requests. Oversized input is rejected before transport.

Compatibility impact: chunked helpers, `BlockWriteOptions`, and `split_mixed_blocks` are removed.

Acceptance criteria:

1. A mixed word/bit block write produces exactly one `1406` request.
2. An oversized named/random plan produces zero requests and returns `InvalidArgument`.
3. Source and user documentation contain no split or chunk API recommendation.

## CPP-OH-004 — Explicit remote-control intent

Scope: synchronous and asynchronous Remote RUN, PAUSE, and RESET APIs.

Target contract: RUN requires `RemoteMode` and `RemoteClearMode`; PAUSE requires `RemoteMode`; unknown enum values fail before transport. RESET fixes command `1006`, subcommand `0000`, request data `0001`, and completes after transmission without waiting for a response.

Compatibility impact: Boolean/default/magic-number remote arguments and RESET response options are removed.

Acceptance criteria:

1. All three clear modes and both operation modes encode their documented wire values.
2. Undefined enum values produce zero transport writes.
3. RESET sends the fixed frame and does not turn the specified no-response behavior into a timeout.

## CPP-OH-005 — Timing, UDP, reconnect, and end-code surface

Scope: timeout, monitoring timer, Arduino TCP keepalive, UDP binding, reconnect helper, and optional end-code helpers.

Target contract: omitted communication timeout is 3000 ms; omitted monitoring timer is `0x0010`; timeout zero and reconnect interval zero are invalid. TCP connection requires successful application of a 30-second keepalive idle through the platform adapter. UDP local port zero is passed to the UDP stack for ephemeral assignment. Localized message/language hooks are absent while numeric code and stable key remain.

Compatibility impact: end-code compatibility hooks are removed; explicit zero reconnect/timeout configurations are rejected.

Acceptance criteria:

1. Default and zero-value timing/reconnect cases are host-tested.
2. ESP32 builds compile the supplied WiFiClient keepalive configurator, and a missing or failed configurator cannot leave an open TCP connection.
3. Arduino UDP source inspection and adapter tests show remote port is never substituted for local zero.
4. API-reference generation contains stable end-code helpers and no language/message hooks.

## CPP-OH-006 — Explicit write and CPU-buffer targets

Scope: random writes, block writes, CPU-buffer helpers, and label abbreviation inputs.

Target contract: random/block writes reject duplicate or overlapping destinations before transport. CPU-buffer helpers require `CpuModule::Cpu1` through `Cpu4`; no helper silently selects CPU No.1. Label and abbreviation counts, pointers, names, units, lengths, and data sizes are validated before encoding.

Compatibility impact: CPU-buffer calls must add the explicit CPU module argument; previously tolerated malformed label inputs and overlapping writes are rejected.

Acceptance criteria:

1. Word/DWord random overlap, duplicate bit writes, and overlapping block ranges produce zero requests.
2. Each CPU module maps to `0x03E0` through `0x03E3`; unknown enum values produce zero requests.
3. A non-zero abbreviation count with a null pointer and an empty label are rejected without dereference or transport.

## CPP-OH-006A — Required long-timer range

Scope: blocking and asynchronous long-timer/long-retentive-timer helpers and their coil/contact projections.

Target contract: every call explicitly supplies the head number and multi-point count. The head must be non-negative, the count must be positive, and four words per timer must fit the active profile's one-request direct-word limit without integer truncation.

Compatibility impact: omitted head/count values remain compile errors; calls above the one-request limit are rejected instead of emitting a wrapped or oversized request.

Acceptance criteria:

1. Negative heads, zero counts, 241 timers, and values that previously truncated through `uint16_t` produce zero requests.
2. Explicit LTN10 and LSTN0 single-point calls encode heads 10 and 0, the correct device family, and exactly four word points.
3. Blocking and begin/state projection helpers share the same validation.

## CPP-OH-007 — Client-bound high-level execution profile

Scope: typed read/write, named read/write, bit-in-word write, examples, and generated API documentation.

Target contract: every high-level operation that can communicate derives its profile, frame, compatibility, device layout, and subcommand family from the client profile. No communicating overload accepts a second `PlcProfile`. Offline address normalization and read-plan compilation remain explicitly profile-bound because they do not own a client; executing a compiled plan requires an exact client/plan profile match.

Compatibility impact: remove the `readTyped`, `writeTyped`, `writeBitInWord`, `readNamed`, and `writeNamed` overloads that accepted both a client and a separate profile. Callers remove the profile argument after constructing the client with the intended canonical profile.

Acceptance criteria:

1. The public header and generated API reference expose no communicating high-level overload with both `SlmpClient&` and `PlcProfile`.
2. Examples invoke typed and named operations through client-derived overloads only.
3. Offline plans retain their profile and `readNamed(client, plan, ...)` rejects a mismatch before transport.
4. Host, Arduino, socket-integration, example, and generated-API gates pass.

## Verification checklist

- [x] Implementation completed for CPP-OH-001 through CPP-OH-007 in this repository.
- [x] Tests added or updated for the machine-verifiable host acceptance criteria.
- [x] Host unit tests, Arduino transport tests, socket integration scenarios, generated API check, device-range parity, compile-only tools, and snapshot example passed.
- [x] ESP32 low-level, high-level, and polling/reconnect builds passed; PlatformIO package archive creation and content inspection passed.
- [ ] PlatformIO static analysis completed — `pio check` did not complete within 120 seconds in this environment; no result is inferred.
- [ ] WIZnet W6300 Pico2 build completed — blocked because the installed Raspberry Pi platform does not define board ID `wiznet_6300_evb_pico2`.
- [ ] Shared cross-language vector function test completed — blocked because sibling repository `plc-comm-slmp-cross-verify` and its four required shared specs are absent.
- [x] Codex self-review completed against the approved contract and cross-language consistency requirements.
- [ ] Claude source review completed and findings recorded — pending user authorization; Claude has not been invoked.
- [ ] Codex resolved or dispositioned every Claude finding and reran affected checks — pending Claude review.
- [ ] Required live-PLC checks passed, or each unavailable check has an explicit release disposition — no live checks executed in this overhaul pass.
- [x] Documentation, migration notes, changelog, examples, and generated API reference agree with implementation.
- [ ] Final acceptance criteria verified and the item marked complete.

## Verification evidence

- `run_ci.bat`: host compile, Arduino keepalive fail-closed tests, live-tool usage smoke, generated API check, unit tests, and five socket scenarios passed.
- `scripts/check_device_range_catalog_parity.py`: passed.
- Compile-only: `high_level_snapshot`, `slmp_live_read_once`, `slmp_matrix_validation`, and `slmp_gxsim3_validation` built with `g++`; no live endpoint was contacted.
- PlatformIO: `esp32-devkitc-low-level`, `esp32-devkitc-high-level`, and `esp32-devkitc-polling-reconnect` passed.
- Installed ESP32 Arduino framework source was inspected on 2026-07-12: `WiFiUDP::begin(uint16_t)` delegates to the address overload, stores the supplied port in `sockaddr_in.sin_port`, and calls `bind`; therefore the library's `begin(0)` reaches the BSD socket bind unchanged and requests an ephemeral port. The current ESP32 wrapper does not expose the actual bound port, so runtime receipt remains unverified rather than being reported as a live pass.
- No installed W6300 board package or EthernetUDP implementation was available to inspect, and the configured W6300 board ID is unavailable in the installed PlatformIO platform. W6300 remains build/runtime unverified and must not be listed as a verified platform from this evidence.
- Package: `pio pkg pack` passed; archive contents contain the intended public sources, examples, metadata, license, README, security notice, and changelog.
- `git diff --check`: no whitespace error; Git emitted only repository line-ending conversion warnings.

## Live-verification TODO

- [ ] Profile: `melsec:iq-r`; platform: ESP32 WiFiClient; endpoint: `192.168.250.100:1025`; device: `D100`; intent: read only; purpose: verify TCP connection, ordinary read, and platform keepalive operation after more than 30 seconds idle; expected evidence: successful read plus packet/socket evidence showing keepalive starts after the approved idle; restoration: none.
- [ ] Profile: `melsec:iq-r`; platform: ESP32 WiFiUDP; endpoint: `192.168.250.100:1035`; device: `D100`; intent: read only; purpose: verify local port `0` obtains an ephemeral port and receives the matching UDP response; expected evidence: assigned local port and successful read; restoration: none.
- [ ] Profile: `melsec:iq-r`; platform: WIZnet W6300 EVB Pico2; endpoint: `192.168.250.100:1035`; device: `D100`; intent: read only; purpose: verify the currently unavailable board build/runtime and UDP ephemeral binding; expected evidence: successful board build, assigned local port, and read result; restoration: none.

No live PLC communication is authorized by this document.

## Claude review status

Pending user authorization. Before any Claude invocation, present the repository/diff scope, CPP-OH decisions, review purpose, supplied test evidence, and expected findings format, then wait for explicit authorization for that batch.
