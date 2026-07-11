# SLMP C++ Minimal Quality Overhaul

This record preserves the approved target contract and verification state for the breaking quality overhaul. User documentation describes only the resulting supported behavior.

## CPP-OH-001 — Required connection identity

Scope: `SlmpClient`, `TargetAddress`, connection setup, examples, and generated API documentation.

Target contract: construction requires one concrete PLC profile and all four target-route fields. The target is fixed for the client lifetime. Port remains required by `connect`; zero, empty host, invalid profile, and unusable buffers fail before transport.

Compatibility impact: the former five-argument constructor and post-construction target setter are removed.

Acceptance criteria:

1. Constructing a client without profile or target is a compile-time error.
2. `PlcProfile::Unspecified`, empty host, and port zero cannot reach transport connection or request transmission.
3. Generated API documentation contains the required constructor arguments and no target setter.

## CPP-OH-002 — Profile-bound semantic addresses

Scope: `DeviceAddress`, `AddressSpec`, device factories, standalone address utilities, read plans, and every direct-device send path.

Target contract: semantic addresses retain their canonical PLC profile. Standalone parsing, formatting, normalization, and plan compilation require a profile. A client/address or client/plan profile mismatch returns `InvalidArgument` before transport.

Compatibility impact: profileless device factories and standalone address overloads are removed.

Acceptance criteria:

1. `X10` parsed for iQ-R retains numeric 16; the iQ-F parse retains numeric 8.
2. Formatting an address with a different profile is rejected.
3. Direct, random, block, monitor, high-level named, and compiled-plan paths reject profile mismatch without sending.

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

## Verification checklist

- [x] Implementation completed for CPP-OH-001 through CPP-OH-006 in this repository.
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
- Package: `pio pkg pack` passed; archive contents contain the intended public sources, examples, metadata, license, README, security notice, and changelog.
- `git diff --check`: no whitespace error; Git emitted only repository line-ending conversion warnings.

## Live-verification TODO

- [ ] Profile: `melsec:iq-r`; platform: ESP32 WiFiClient; endpoint: `192.168.250.100:1025`; device: `D100`; intent: read only; purpose: verify TCP connection, ordinary read, and platform keepalive operation after more than 30 seconds idle; expected evidence: successful read plus packet/socket evidence showing keepalive starts after the approved idle; restoration: none.
- [ ] Profile: `melsec:iq-r`; platform: ESP32 WiFiUDP; endpoint: `192.168.250.100:1035`; device: `D100`; intent: read only; purpose: verify local port `0` obtains an ephemeral port and receives the matching UDP response; expected evidence: assigned local port and successful read; restoration: none.
- [ ] Profile: `melsec:iq-r`; platform: WIZnet W6300 EVB Pico2; endpoint: `192.168.250.100:1035`; device: `D100`; intent: read only; purpose: verify the currently unavailable board build/runtime and UDP ephemeral binding; expected evidence: successful board build, assigned local port, and read result; restoration: none.

No live PLC communication is authorized by this document.

## Claude review status

Pending user authorization. Before any Claude invocation, present the repository/diff scope, CPP-OH decisions, review purpose, supplied test evidence, and expected findings format, then wait for explicit authorization for that batch.
