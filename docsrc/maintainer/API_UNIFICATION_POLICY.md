# API Unification Policy

This document defines the planned public API rules for the SLMP C++ minimal library.
It is a design policy document. It does not claim that every rule is implemented yet.

## Purpose

- Keep the embedded API explicit and allocation-free.
- Keep naming comparable with the Python SLMP library where the operation class is the same.
- Define the optional UDP transport policy before adding the implementation.
- Avoid frame-specific canonical class names when the implementation can switch between Binary 3E and Binary 4E framing.

## Public API Shape

The canonical public protocol class is:

- `slmp::SlmpClient`

The embedded API must stay explicit.
Do not add a generic `read()` or `write()` facade that hides data width or request type.

If a separate string-address facade is ever introduced, reserve this name for that layer:

- `slmp::SlmpDeviceClient`

Canonical sync names:

- `readTypeName`
- `readWords`
- `writeWords`
- `readBits`
- `writeBits`
- `readDWords`
- `writeDWords`
- `readFloat32s`
- `writeFloat32s`
- `readOneWord`
- `writeOneWord`
- `readOneBit`
- `writeOneBit`
- `readOneDWord`
- `writeOneDWord`
- `readOneFloat32`
- `writeOneFloat32`
- `readRandom`
- `writeRandomWords`
- `writeRandomBits`
- `readBlock`
- `writeBlock`
- `remotePasswordUnlock`
- `remotePasswordLock`

## Async Rules

The C++ async model remains the non-blocking state machine style.
Async naming must be derived directly from the sync name.

Canonical async entry points:

- `beginReadTypeName`
- `beginReadWords`
- `beginWriteWords`
- `beginReadBits`
- `beginWriteBits`
- `beginReadDWords`
- `beginWriteDWords`
- `beginReadFloat32s`
- `beginWriteFloat32s`
- `beginReadRandom`
- `beginWriteRandomWords`
- `beginWriteRandomBits`
- `beginReadBlock`
- `beginWriteBlock`
- `beginRemotePasswordUnlock`
- `beginRemotePasswordLock`
- `update`
- `isBusy`

Rules:

- The async operation name must be `begin` + sync name.
- Output buffers and argument ordering must match the sync form as closely as possible.
- Async completion must write results into caller-owned buffers, not heap allocations.

## Python Parity Rules

Where both the Python SLMP library and this C++ library expose the same operation class, the semantic name should match.

Examples:

- `readTypeName` <-> `read_type_name`
- `readRandom` <-> `read_random`
- `readBlock` <-> `read_block`
- `writeRandomBits` <-> `write_random_bits`
- `readDWords` <-> `read_dwords`
- `readFloat32s` <-> `read_float32s`

The class-level naming policy should also stay aligned:

- `slmp::SlmpClient` <-> `SlmpClient`

## Internal Naming Rules

The embedded core may keep short internal names when the scope is narrow and protocol-local.
The following internal names are acceptable because they are closely tied to the state machine and request pipeline.

- `request`
- `setError`
- `startAsync`
- `completeAsync`

Transport helper class names must always include the transport kind.

- `ArduinoClientTransport`
- `ArduinoUdpTransport`

Do not introduce a generic helper name such as `ArduinoTransport` when the implementation is actually transport-specific.

## 32-Bit Value Rules

The library should distinguish raw 32-bit integers from IEEE 754 floating-point values.

- `DWord` means a raw 32-bit unsigned value stored across two PLC words.
- Floating-point helpers should use `Float32` in the name.
- If signed 32-bit helpers are added later, name them `readInt32s`, `writeInt32s`, `readOneInt32`, and `writeOneInt32`.

Default 32-bit word-pair interpretation:

- The default contract is protocol-native low-word-first ordering.
- If alternate word order must be supported, expose it through an explicit enum or option, not a vague suffix.

Preferred internal codec helper patterns:

- `packUInt32LowWordFirst`
- `unpackUInt32LowWordFirst`
- `packFloat32LowWordFirst`
- `unpackFloat32LowWordFirst`

## Optional UDP Transport Policy

The protocol client stays transport-agnostic.
UDP support must be introduced as an optional helper transport, not as a second protocol core.

Planned rules:

1. Keep `slmp::SlmpClient` transport-agnostic.
2. Add a dedicated helper transport header for Arduino-style UDP transport.
3. Gate the helper transport with a build-time macro.
4. Make TCP-only builds avoid UDP-specific includes and code paths.

Planned transport switch:

- Macro: `SLMP_ENABLE_UDP_TRANSPORT`
- Disabled state: UDP helper transport is not compiled.
- Enabled state: UDP helper transport becomes available for projects that need it.

Default policy:

- The core library must stay usable without enabling UDP helper transport.
- Examples that require UDP must explicitly document the required build flag.
- TCP-only examples must not pull UDP helper code.

This policy keeps the minimal library focused on size-sensitive builds while still allowing UDP for applications that need it.
