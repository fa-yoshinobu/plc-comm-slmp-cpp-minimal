# Gotchas

## LTN/LSTN/LCN/LZ reads return unexpected values

These are 32-bit families. Using `U16` yields wrong data.

Fix: use `U32` or `S32` value type.

```cpp
slmp::highlevel::Value value;
const slmp::Error err = slmp::highlevel::readTyped(
    plc,
    slmp::highlevel::PlcProfile::IqR,
    "LTN0:D",
    value);
```

## Mixed block write rejected by PLC

Command `0x1406` does not accept word + bit combinations in one request.

Fix: separate word writes and bit writes into distinct calls.

```cpp
const uint16_t wordValue = 1234U;
const bool bitValue = true;

slmp::Error wordErr = plc.writeOneWord(slmp::dev::D(slmp::dev::dec(9000)), wordValue);
slmp::Error bitErr = plc.writeOneBit(slmp::dev::M(slmp::dev::dec(9000)), bitValue);
```

## G or HG address fails

`G` and `HG` are not in the public high-level surface for normal direct device access.

Fix: use raw `slmp::SlmpClient` methods for module buffer access.

```cpp
uint16_t words[4] = {};
const slmp::Error err = plc.readWordsModuleBuf(3, false, 100, 4, words, 4);
```

## DX or DY fails on iQ-F

`DX` and `DY` are not valid for the iQ-F profile.

Fix: use `X` and `Y` instead.

```cpp
slmp::highlevel::Value input;
const slmp::Error err = slmp::highlevel::readTyped(
    plc,
    slmp::highlevel::PlcProfile::IqF,
    "X20",
    input);
```

## PLC profile not set

The library requires explicit profile selection to derive frame type and device ranges. There is no default.

Fix: call `slmp::highlevel::configureClientForPlcProfile(client, profile)` before any reads or writes.

```cpp
constexpr auto profile = slmp::highlevel::PlcProfile::IqR;
slmp::highlevel::configureClientForPlcProfile(plc, profile);
```

## High-level helpers unavailable

If `slmp::highlevel::readTyped` or `slmp::highlevel::readNamed` are undefined, `slmp_high_level.h` is not included by default; it must be explicitly included.

Fix: add `#include <slmp_high_level.h>`.

```cpp
#include <slmp_high_level.h>
```

## X or Y string address is rejected

String `X` and `Y` addresses require an explicit PLC profile because iQ-F uses octal notation while other supported profiles use hexadecimal notation.

Fix: use a profile-aware overload.

```cpp
slmp::highlevel::Value input;
const slmp::Error err = slmp::highlevel::readTyped(
    plc,
    slmp::highlevel::PlcProfile::IqR,
    "X20",
    input);
```

## D50.D reads the wrong thing

`D50.D` is bit `0xD` of `D50`, not a 32-bit type request. Type suffixes use `:`, while bit-in-word access uses `.`.

Fix: use `D50:D` for unsigned 32-bit data and `D50.D` only for bit 13.

```cpp
slmp::highlevel::Value dword;
slmp::highlevel::readTyped(plc, slmp::highlevel::PlcProfile::IqR, "D50:D", dword);

slmp::highlevel::Value bit13;
slmp::highlevel::readTyped(plc, slmp::highlevel::PlcProfile::IqR, "D50.D", bit13);
```
