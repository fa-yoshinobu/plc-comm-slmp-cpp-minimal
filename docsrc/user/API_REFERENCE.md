# SLMP C++ API Reference

This file is generated from Doxygen XML for the public C++ headers.
Do not edit it manually; run `scripts/generate_api_reference.py` instead.

## Header Inputs

- `src/slmp_minimal.h`
- `src/slmp_high_level.h`
- `src/slmp_arduino_transport.h`
- `src/slmp_error_codes.h`
- `src/slmp_utility.h`

## Namespaces

### Namespace `slmp`

Root namespace for all SLMP communication components.

#### Enums

#### `EndCodeLanguage`

Language selector retained for optional external end-code catalogs.

| Value | Description |
| --- | --- |
| `English = 0` |  |
| `Japanese = 1` |  |

#### Functions

#### `endCodeMessage`

```cpp
const char * slmp::endCodeMessage(uint16_t end_code, EndCodeLanguage language=EndCodeLanguage::English)
```

Get a user-facing SLMP end-code message.

Localized message text is not embedded in this library; resolve endCodeString(end_code) in an application-owned catalog. This function is retained as a compatibility hook and returns nullptr.

#### `endCodeMessageEnglish`

```cpp
const char * slmp::endCodeMessageEnglish(uint16_t end_code)
```

Compatibility hook for English message lookup; returns nullptr.

#### `endCodeMessageJapanese`

```cpp
const char * slmp::endCodeMessageJapanese(uint16_t end_code)
```

Compatibility hook for Japanese message lookup; returns nullptr.

### Namespace `slmp::dev`

Fluent API for defining device addresses.

This namespace is the recommended way to create DeviceAddress values in application code. It keeps the call site explicit about whether the PLC uses decimal numbering or MELSEC hexadecimal numbering.

Typical usage: autod100=slmp::dev::D(slmp::dev::dec(100));//decimal-numberedDregister autox1a=slmp::dev::X(slmp::dev::hex(0x1A));//hexadecimal-numberedXinput autord10=slmp::dev::RD(slmp::dev::dec(10));//refreshdataregister autoltn0=slmp::dev::LTN(slmp::dev::dec(0));//longtimercurrentvalue

Device families exposed here match the generic direct-access helpers in SlmpClient. Extended devices such as U\\G, U\\HG, and J\\device use ExtDeviceSpec instead of these factory helpers.

#### Functions

#### `dec`

```cpp
DecNo slmp::dev::dec(uint32_t value)
```

Mark a PLC device number as decimal.

value Decimal device number, for example 100 for D100. Decimal-number wrapper consumed by the device helper factories.

| Parameter | Description |
| --- | --- |
| `value` | Decimal device number, for example 100 for D100. |

Returns: Decimal-number wrapper consumed by the device helper factories.

#### `hex`

```cpp
HexNo slmp::dev::hex(uint32_t value)
```

Mark a PLC device number as hexadecimal.

value Hexadecimal device number, for example 0x1A for X1A. Hexadecimal-number wrapper consumed by the device helper factories.

| Parameter | Description |
| --- | --- |
| `value` | Hexadecimal device number, for example 0x1A for X1A. |

Returns: Hexadecimal-number wrapper consumed by the device helper factories.

#### `FDevice`

```cpp
DeviceAddress slmp::dev::FDevice(DecNo number)
```

Create an annunciator (F) device address.

number Decimal annunciator number such as slmp::dev::dec(10). Device address for F<number>. This helper is named FDevice instead of F because some embedded toolchains define F as a macro.

| Parameter | Description |
| --- | --- |
| `number` | Decimal annunciator number such as slmp::dev::dec(10). |

Returns: Device address for F<number>.

#### `blockRead`

```cpp
DeviceBlockRead slmp::dev::blockRead(DeviceAddress device, uint16_t points)
```

Create a contiguous block-read descriptor.

device First device in the block. points Number of points to read from device onward. Descriptor suitable for SlmpClient::readBlock and related APIs.

| Parameter | Description |
| --- | --- |
| `device` | First device in the block. |
| `points` | Number of points to read from device onward. |

Returns: Descriptor suitable for SlmpClient::readBlock and related APIs.

#### `blockWrite`

```cpp
DeviceBlockWrite slmp::dev::blockWrite(DeviceAddress device, const uint16_t *values, uint16_t points)
```

Create a contiguous block-write descriptor.

device First device in the block. values Caller-owned word values in PLC order. points Number of points to write from values. Descriptor suitable for SlmpClient::writeBlock and related APIs. Bit-block writes also use word-sized values. Each element should be either 0 or 1 when targeting bit devices.

| Parameter | Description |
| --- | --- |
| `device` | First device in the block. |
| `values` | Caller-owned word values in PLC order. |
| `points` | Number of points to write from values. |

Returns: Descriptor suitable for SlmpClient::writeBlock and related APIs.

### Namespace `slmp::module_io`

Named SLMP request-header module I/O numbers for CPU routing.

#### Variables And Constants

#### `ControlCpu`

```cpp
uint16_t slmp::module_io::ControlCpu = 0x03D0
```

Control or active CPU in a redundant CPU system

#### `ActiveCpu`

```cpp
uint16_t slmp::module_io::ActiveCpu = ControlCpu
```

Alias for ControlCpu

#### `StandbyCpu`

```cpp
uint16_t slmp::module_io::StandbyCpu = 0x03D1
```

Standby CPU in a redundant CPU system

#### `TypeACpu`

```cpp
uint16_t slmp::module_io::TypeACpu = 0x03D2
```

Type A CPU in a redundant CPU system

#### `TypeBCpu`

```cpp
uint16_t slmp::module_io::TypeBCpu = 0x03D3
```

Type B CPU in a redundant CPU system

#### `Cpu1`

```cpp
uint16_t slmp::module_io::Cpu1 = 0x03E0
```

CPU No. 1 in a multi-CPU system

#### `Cpu2`

```cpp
uint16_t slmp::module_io::Cpu2 = 0x03E1
```

CPU No. 2 in a multi-CPU system

#### `Cpu3`

```cpp
uint16_t slmp::module_io::Cpu3 = 0x03E2
```

CPU No. 3 in a multi-CPU system

#### `Cpu4`

```cpp
uint16_t slmp::module_io::Cpu4 = 0x03E3
```

CPU No. 4 in a multi-CPU system

#### `ConnectedCpu`

```cpp
uint16_t slmp::module_io::ConnectedCpu = 0x03FF
```

Default connected CPU route

#### `Default`

```cpp
uint16_t slmp::module_io::Default = ConnectedCpu
```

Alias for ConnectedCpu

#### `OwnStation`

```cpp
uint16_t slmp::module_io::OwnStation = ConnectedCpu
```

Alias for ConnectedCpu

## Classes

### Class `slmp::ArduinoClientTransport`

Transport adapter for Arduino 'Client' objects (TCP).

Wraps classes like WiFiClient, EthernetClient, or GSMClient. Implements reliable stream-based communication for SLMP 3E/4E frames.

#### Member Functions

#### `ArduinoClientTransport`

```cpp
slmp::ArduinoClientTransport::ArduinoClientTransport(::Client &client)
```

Wrap an existing Arduino Client.

client Reference to an Arduino Client object (e.g., WiFiClient).

| Parameter | Description |
| --- | --- |
| `client` | Reference to an Arduino Client object (e.g., WiFiClient). |

#### `connect`

```cpp
bool slmp::ArduinoClientTransport::connect(const char *host, uint16_t port) override
```

Connect to PLC via TCP.

#### `close`

```cpp
void slmp::ArduinoClientTransport::close() override
```

Stop the client and close connection.

#### `connected`

```cpp
bool slmp::ArduinoClientTransport::connected() const override
```

Check if the TCP socket is connected.

#### `writeAll`

```cpp
bool slmp::ArduinoClientTransport::writeAll(const uint8_t *data, size_t length) override
```

Block until all data is written to the stream.

#### `readExact`

```cpp
bool slmp::ArduinoClientTransport::readExact(uint8_t *data, size_t length, uint32_t timeout_ms) override
```

Block until exact length is read from the stream or timeout occurs.

#### `write`

```cpp
size_t slmp::ArduinoClientTransport::write(const uint8_t *data, size_t length) override
```

Non-blocking write to the stream.

#### `read`

```cpp
size_t slmp::ArduinoClientTransport::read(uint8_t *data, size_t length) override
```

Non-blocking read from the stream.

#### `available`

```cpp
size_t slmp::ArduinoClientTransport::available() override
```

Check number of bytes available in the stream.

### Class `slmp::ArduinoUdpTransport`

Transport adapter for Arduino 'UDP' objects.

Wraps classes like WiFiUDP or EthernetUDP. Manages remote endpoint (host/port) and packet framing for SLMP.

UDP is connectionless; "connected" state in this class simply means that begin() has been called and remote endpoint is known.

#### Member Functions

#### `ArduinoUdpTransport`

```cpp
slmp::ArduinoUdpTransport::ArduinoUdpTransport(::UDP &udp, uint16_t local_port=0)
```

Wrap an existing Arduino UDP object.

udp Reference to UDP object. local_port Local port to bind to (0 = use same as remote port).

| Parameter | Description |
| --- | --- |
| `udp` | Reference to UDP object. |
| `local_port` | Local port to bind to (0 = use same as remote port). |

#### `connect`

```cpp
bool slmp::ArduinoUdpTransport::connect(const char *host, uint16_t port) override
```

Set remote host and begin listening on local port.

#### `close`

```cpp
void slmp::ArduinoUdpTransport::close() override
```

Stop listening and clear remote endpoint.

#### `connected`

```cpp
bool slmp::ArduinoUdpTransport::connected() const override
```

True if the transport is initialized.

#### `writeAll`

```cpp
bool slmp::ArduinoUdpTransport::writeAll(const uint8_t *data, size_t length) override
```

Send a single UDP packet containing the frame.

#### `readExact`

```cpp
bool slmp::ArduinoUdpTransport::readExact(uint8_t *data, size_t length, uint32_t timeout_ms) override
```

Block until a UDP packet of exact length is received or timeout.

#### `write`

```cpp
size_t slmp::ArduinoUdpTransport::write(const uint8_t *data, size_t length) override
```

Non-blocking packet send. Returns length on success.

#### `read`

```cpp
size_t slmp::ArduinoUdpTransport::read(uint8_t *data, size_t length) override
```

Non-blocking read from the current received packet.

#### `available`

```cpp
size_t slmp::ArduinoUdpTransport::available() override
```

Check if a new packet has arrived and return its size.

### Class `slmp::highlevel::Poller`

Reusable snapshot poller that keeps one compiled read plan.

This class does not sleep. The caller controls timing and repeatedly calls readOnce from a scheduler, main loop, or RTOS task.

#### Member Functions

#### `Poller`

```cpp
slmp::highlevel::Poller::Poller()=default
```

#### `Poller`

```cpp
slmp::highlevel::Poller::Poller(const ReadPlan &plan)
```

Construct a poller from an already-compiled plan.

#### `compile`

```cpp
Error slmp::highlevel::Poller::compile(const std::vector< std::string > &addresses)
```

Compile and store one reusable read plan.

addresses Caller-provided high-level addresses. Error::Ok on success. This overload is retained for source compatibility but returns Error::InvalidArgument. Use the overload that receives a PLC profile.

| Parameter | Description |
| --- | --- |
| `addresses` | Caller-provided high-level addresses. |

Returns: Error::Ok on success.

#### `compile`

```cpp
Error slmp::highlevel::Poller::compile(const std::vector< std::string > &addresses, PlcProfile family)
```

Compile and store one reusable read plan with an explicit PLC profile.

#### `readOnce`

```cpp
Error slmp::highlevel::Poller::readOnce(SlmpClient &client, Snapshot &out) const
```

Execute one snapshot read with the stored compiled plan.

client Connected low-level client instance. out Receives the logical values in plan order. Error::Ok on success.

| Parameter | Description |
| --- | --- |
| `client` | Connected low-level client instance. |
| `out` | Receives the logical values in plan order. |

Returns: Error::Ok on success.

#### `plan`

```cpp
const ReadPlan & slmp::highlevel::Poller::plan() const
```

Return the currently stored compiled plan for inspection or reuse.

### Class `slmp::ITransport`

Abstract interface for the underlying transport layer (TCP/UDP/Serial).

Implement this interface to support custom communication stacks. For Arduino-compatible ESP32/RP2040 cores, see ArduinoClientTransport or ArduinoUdpTransport.

#### Member Functions

#### `~ITransport`

```cpp
virtual slmp::ITransport::~ITransport()=default
```

#### `connect`

```cpp
virtual bool slmp::ITransport::connect(const char *host, uint16_t port)=0
```

Connect to host.

#### `close`

```cpp
virtual void slmp::ITransport::close()=0
```

Close connection.

#### `connected`

```cpp
virtual bool slmp::ITransport::connected() const =0
```

Check connection status.

#### `writeAll`

```cpp
virtual bool slmp::ITransport::writeAll(const uint8_t *data, size_t length)=0
```

Block until all data is written.

#### `readExact`

```cpp
virtual bool slmp::ITransport::readExact(uint8_t *data, size_t length, uint32_t timeout_ms)=0
```

Block until exact length is read.

#### `write`

```cpp
virtual size_t slmp::ITransport::write(const uint8_t *data, size_t length)=0
```

Non-blocking write.

#### `read`

```cpp
virtual size_t slmp::ITransport::read(uint8_t *data, size_t length)=0
```

Non-blocking read.

#### `available`

```cpp
virtual size_t slmp::ITransport::available()=0
```

Check pending read data length.

### Class `slmp::SlmpClient`

Main SLMP client implementation.

This class is the core low-level client. It is designed to be: - Memory Efficient: No dynamic allocation. User provides buffers. - Flexible: Supports synchronous (blocking) and asynchronous (non-blocking) calls. - Robust: Validates buffer capacities and protocol state.

Use this class directly when you want deterministic firmware behavior. If you prefer string device addresses such as D100 or D200:F, see the optional helper facade in slmp_high_level.h.

#### Member Functions

#### `SlmpClient`

```cpp
slmp::SlmpClient::SlmpClient(ITransport &transport, uint8_t *tx_buffer, size_t tx_capacity, uint8_t *rx_buffer, size_t rx_capacity)
```

Initialize client with transport and buffers.

transport Reference to transport implementation (must remain valid). tx_buffer Pointer to transmission buffer. tx_capacity Capacity of tx_buffer in bytes. rx_buffer Pointer to reception buffer. rx_capacity Capacity of rx_buffer in bytes.

| Parameter | Description |
| --- | --- |
| `transport` | Reference to transport implementation (must remain valid). |
| `tx_buffer` | Pointer to transmission buffer. |
| `tx_capacity` | Capacity of tx_buffer in bytes. |
| `rx_buffer` | Pointer to reception buffer. |
| `rx_capacity` | Capacity of rx_buffer in bytes. |

#### `connect`

```cpp
bool slmp::SlmpClient::connect(const char *host, uint16_t port)
```

Connect the configured transport to one PLC endpoint.

host PLC IP address or hostname. port SLMP TCP or UDP port such as 1025. True when the underlying transport reports a successful connect. Call this once before using the synchronous helper API such as readWords, readOneDWord, or writeOneFloat32.

| Parameter | Description |
| --- | --- |
| `host` | PLC IP address or hostname. |
| `port` | SLMP TCP or UDP port such as 1025. |

Returns: True when the underlying transport reports a successful connect.

#### `close`

```cpp
void slmp::SlmpClient::close()
```

Close the current transport connection and clear in-flight state.

#### `connected`

```cpp
bool slmp::SlmpClient::connected() const
```

Check whether the transport is currently connected.

#### `setTarget`

```cpp
void slmp::SlmpClient::setTarget(const TargetAddress &target)
```

Set target station routing (e.g. for multi-network routing).

#### `target`

```cpp
const TargetAddress & slmp::SlmpClient::target() const
```

Get current target station routing.

#### `setFrameType`

```cpp
void slmp::SlmpClient::setFrameType(FrameType frame_type)
```

Set frame format (3E/4E). Default is 4E.

#### `frameType`

```cpp
FrameType slmp::SlmpClient::frameType() const
```

Get current frame format.

#### `setCompatibilityMode`

```cpp
void slmp::SlmpClient::setCompatibilityMode(CompatibilityMode mode)
```

Set device access mode (iQ-R/Legacy). Default is iQR.

#### `compatibilityMode`

```cpp
CompatibilityMode slmp::SlmpClient::compatibilityMode() const
```

Get current compatibility mode.

#### `setPlcProfile`

```cpp
void slmp::SlmpClient::setPlcProfile(PlcProfile profile)
```

Set a concrete PLC profile and apply its frame/compatibility defaults.

#### `plcProfile`

```cpp
PlcProfile slmp::SlmpClient::plcProfile() const
```

Return the currently selected PLC profile, or Unspecified after manual low-level overrides.

#### `setManualProfile`

```cpp
void slmp::SlmpClient::setManualProfile(PlcProfile profile, FrameType frame_type, CompatibilityMode mode)
```

Set an explicit PLC profile while manually selecting frame and compatibility mode.

This is intended for low-level verification and compatibility tooling that must emit a specific SLMP frame shape while still keeping profile-based guards active. Normal applications should prefer setPlcProfile.

#### `setStrictProfile`

```cpp
void slmp::SlmpClient::setStrictProfile(bool enabled)
```

Enable or disable strict built-in Ethernet profile feature guards. Default is enabled.

#### `strictProfile`

```cpp
bool slmp::SlmpClient::strictProfile() const
```

Return whether strict built-in Ethernet profile feature guards are enabled.

#### `setBlockAccessEnabled`

```cpp
void slmp::SlmpClient::setBlockAccessEnabled(bool enabled)
```

Enable or disable Read/Write Block commands when the selected PLC profile permits them.

#### `blockAccessEnabled`

```cpp
bool slmp::SlmpClient::blockAccessEnabled() const
```

Return whether Read/Write Block commands are effectively enabled after PLC-profile guards.

#### `setMonitoringTimer`

```cpp
void slmp::SlmpClient::setMonitoringTimer(uint16_t monitoring_timer)
```

Set SLMP monitoring timer (units of 250ms). How long the PLC should wait for internal processing.

#### `monitoringTimer`

```cpp
uint16_t slmp::SlmpClient::monitoringTimer() const
```

Get current monitoring timer value.

#### `setTimeoutMs`

```cpp
void slmp::SlmpClient::setTimeoutMs(uint32_t timeout_ms)
```

Set internal transport timeout in milliseconds.

#### `timeoutMs`

```cpp
uint32_t slmp::SlmpClient::timeoutMs() const
```

Get current timeout value.

#### `lastError`

```cpp
Error slmp::SlmpClient::lastError() const
```

Get the error code from the last operation.

#### `lastEndCode`

```cpp
uint16_t slmp::SlmpClient::lastEndCode() const
```

Get the PLC-specific end code from the last operation. Valid if lastError() is Error::PlcError.

#### `hasLastErrorInfo`

```cpp
bool slmp::SlmpClient::hasLastErrorInfo() const
```

Return true when the last PLC error included a structured 9-byte error information block.

#### `lastErrorInfo`

```cpp
const SlmpErrorInfo & slmp::SlmpClient::lastErrorInfo() const
```

Get structured error information from the last PLC error response.

#### `hasLastProfileFeatureErrorInfo`

```cpp
bool slmp::SlmpClient::hasLastProfileFeatureErrorInfo() const
```

Return true when the last error was a strict profile feature guard.

#### `lastProfileFeatureErrorInfo`

```cpp
const ProfileFeatureErrorInfo & slmp::SlmpClient::lastProfileFeatureErrorInfo() const
```

Get structured information from the last strict profile guard error.

#### `lastRequestFrame`

```cpp
const uint8_t * slmp::SlmpClient::lastRequestFrame() const
```

Get pointer to the raw request frame buffer.

#### `lastRequestFrameLength`

```cpp
size_t slmp::SlmpClient::lastRequestFrameLength() const
```

Get the actual length of the last request frame.

#### `lastResponseFrame`

```cpp
const uint8_t * slmp::SlmpClient::lastResponseFrame() const
```

Get pointer to the raw response frame buffer.

#### `lastResponseFrameLength`

```cpp
size_t slmp::SlmpClient::lastResponseFrameLength() const
```

Get the actual length of the last response frame.

#### `readTypeName`

```cpp
Error slmp::SlmpClient::readTypeName(TypeNameInfo &out)
```

Read PLC model information for diagnostics.

Profile selection is intentionally explicit in the high-level API. Do not infer the active profile from this response. out Receives the PLC model name and optional model code. Operation result.

| Parameter | Description |
| --- | --- |
| `out` | Receives the PLC model name and optional model code. |

Returns: Operation result.

#### `readCpuOperationState`

```cpp
Error slmp::SlmpClient::readCpuOperationState(CpuOperationState &out)
```

Read SD203 and decode the CPU operation state from the lower 4 bits.

out Receives the decoded state and raw masked code. Operation result.

| Parameter | Description |
| --- | --- |
| `out` | Receives the decoded state and raw masked code. |

Returns: Operation result.

#### `readWords`

```cpp
Error slmp::SlmpClient::readWords(const DeviceAddress &device, uint16_t points, uint16_t *values, size_t value_capacity)
```

Read contiguous word devices.

device Start address. points Number of words to read. values Buffer to store read values. value_capacity Capacity of values buffer (in elements).

| Parameter | Description |
| --- | --- |
| `device` | Start address. |
| `points` | Number of words to read. |
| `values` | Buffer to store read values. |
| `value_capacity` | Capacity of values buffer (in elements). |

#### `writeWords`

```cpp
Error slmp::SlmpClient::writeWords(const DeviceAddress &device, const uint16_t *values, size_t count)
```

Write a contiguous word-device range.

device First word device in the range. values Word values in PLC order. count Number of words to write. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First word device in the range. |
| `values` | Word values in PLC order. |
| `count` | Number of words to write. |

Returns: Operation result.

#### `readBits`

```cpp
Error slmp::SlmpClient::readBits(const DeviceAddress &device, uint16_t points, bool *values, size_t value_capacity)
```

Read a contiguous bit-device range as boolean values.

device First bit device in the range. points Number of bit points to read. values Output buffer for the returned bit states. value_capacity Capacity of values in elements. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First bit device in the range. |
| `points` | Number of bit points to read. |
| `values` | Output buffer for the returned bit states. |
| `value_capacity` | Capacity of values in elements. |

Returns: Operation result.

#### `writeBits`

```cpp
Error slmp::SlmpClient::writeBits(const DeviceAddress &device, const bool *values, size_t count)
```

Write a contiguous bit-device range from boolean values.

device First bit device in the range. values Bit values in PLC order. count Number of bit points to write. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First bit device in the range. |
| `values` | Bit values in PLC order. |
| `count` | Number of bit points to write. |

Returns: Operation result.

#### `readDWords`

```cpp
Error slmp::SlmpClient::readDWords(const DeviceAddress &device, uint16_t points, uint32_t *values, size_t value_capacity)
```

Read a contiguous DWord range as unsigned 32-bit values.

device First device in the range. points Number of 32-bit values to read. values Output buffer for the returned values. value_capacity Capacity of values in elements. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First device in the range. |
| `points` | Number of 32-bit values to read. |
| `values` | Output buffer for the returned values. |
| `value_capacity` | Capacity of values in elements. |

Returns: Operation result.

#### `writeDWords`

```cpp
Error slmp::SlmpClient::writeDWords(const DeviceAddress &device, const uint32_t *values, size_t count)
```

Write a contiguous DWord range from unsigned 32-bit values.

device First device in the range. values 32-bit values in PLC order. count Number of 32-bit values to write. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First device in the range. |
| `values` | 32-bit values in PLC order. |
| `count` | Number of 32-bit values to write. |

Returns: Operation result.

#### `readFloat32s`

```cpp
Error slmp::SlmpClient::readFloat32s(const DeviceAddress &device, uint16_t points, float *values, size_t value_capacity)
```

Read a contiguous range of IEEE-754 float32 values.

device First device in the range. points Number of float32 values to read. values Output buffer for the returned values. value_capacity Capacity of values in elements. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First device in the range. |
| `points` | Number of float32 values to read. |
| `values` | Output buffer for the returned values. |
| `value_capacity` | Capacity of values in elements. |

Returns: Operation result.

#### `writeFloat32s`

```cpp
Error slmp::SlmpClient::writeFloat32s(const DeviceAddress &device, const float *values, size_t count)
```

Write a contiguous range of IEEE-754 float32 values.

device First device in the range. values Float32 values in PLC order. count Number of float32 values to write. Operation result.

| Parameter | Description |
| --- | --- |
| `device` | First device in the range. |
| `values` | Float32 values in PLC order. |
| `count` | Number of float32 values to write. |

Returns: Operation result.

#### `readOneWord`

```cpp
Error slmp::SlmpClient::readOneWord(const DeviceAddress &device, uint16_t &value)
```

Read one 16-bit word value through the synchronous helper API.

#### `writeOneWord`

```cpp
Error slmp::SlmpClient::writeOneWord(const DeviceAddress &device, uint16_t value)
```

Write one 16-bit word value through the synchronous helper API.

#### `readOneBit`

```cpp
Error slmp::SlmpClient::readOneBit(const DeviceAddress &device, bool &value)
```

Read one direct bit-device value through the synchronous helper API.

#### `writeOneBit`

```cpp
Error slmp::SlmpClient::writeOneBit(const DeviceAddress &device, bool value)
```

Write one direct bit-device value through the synchronous helper API.

#### `readOneDWord`

```cpp
Error slmp::SlmpClient::readOneDWord(const DeviceAddress &device, uint32_t &value)
```

Read one unsigned 32-bit value through the synchronous helper API.

#### `writeOneDWord`

```cpp
Error slmp::SlmpClient::writeOneDWord(const DeviceAddress &device, uint32_t value)
```

Write one unsigned 32-bit value through the synchronous helper API.

#### `readOneFloat32`

```cpp
Error slmp::SlmpClient::readOneFloat32(const DeviceAddress &device, float &value)
```

Read one IEEE-754 float32 value through the synchronous helper API.

#### `writeOneFloat32`

```cpp
Error slmp::SlmpClient::writeOneFloat32(const DeviceAddress &device, float value)
```

Write one IEEE-754 float32 value through the synchronous helper API.

#### `readRandom`

```cpp
Error slmp::SlmpClient::readRandom(const DeviceAddress *word_devices, size_t word_count, uint16_t *word_values, size_t word_value_capacity, const DeviceAddress *dword_devices, size_t dword_count, uint32_t *dword_values, size_t dword_value_capacity)
```

Perform random read of multiple word and dword devices.

Allows reading non-contiguous addresses in one request. This is one of the main low-level batching primitives behind the optional high-level snapshot helpers.

#### `writeRandomWords`

```cpp
Error slmp::SlmpClient::writeRandomWords(const DeviceAddress *word_devices, const uint16_t *word_values, size_t word_count, const DeviceAddress *dword_devices, const uint32_t *dword_values, size_t dword_count)
```

Perform random write of multiple word and dword devices.

This is the low-level counterpart to mixed typed writes in the optional helper layer.

#### `writeRandomBits`

```cpp
Error slmp::SlmpClient::writeRandomBits(const DeviceAddress *bit_devices, const bool *bit_values, size_t bit_count)
```

Perform random write of multiple bit devices.

#### `readRandomExt`

```cpp
Error slmp::SlmpClient::readRandomExt(const ExtDeviceSpec *word_devices, size_t word_count, uint16_t *word_values, size_t word_value_capacity, const ExtDeviceSpec *dword_devices, size_t dword_count, uint32_t *dword_values, size_t dword_value_capacity)
```

Random read using extended device specs (module buffer or link direct).

#### `writeRandomWordsExt`

```cpp
Error slmp::SlmpClient::writeRandomWordsExt(const ExtDeviceSpec *word_devices, const uint16_t *word_values, size_t word_count, const ExtDeviceSpec *dword_devices, const uint32_t *dword_values, size_t dword_count)
```

Random write words using extended device specs.

#### `writeRandomBitsExt`

```cpp
Error slmp::SlmpClient::writeRandomBitsExt(const ExtDeviceSpec *bit_devices, const bool *bit_values, size_t bit_count)
```

Random write bits using extended device specs.

#### `registerMonitorDevices`

```cpp
Error slmp::SlmpClient::registerMonitorDevices(const DeviceAddress *word_devices, size_t word_count, const DeviceAddress *dword_devices, size_t dword_count)
```

Register devices for monitoring (command 0x0801).

Register once, then call runMonitorCycle repeatedly to read current values without rebuilding the monitor list every cycle.

#### `registerMonitorDevicesExt`

```cpp
Error slmp::SlmpClient::registerMonitorDevicesExt(const ExtDeviceSpec *word_devices, size_t word_count, const ExtDeviceSpec *dword_devices, size_t dword_count)
```

Register devices for monitoring using extended device specs (command 0x0801).

#### `runMonitorCycle`

```cpp
Error slmp::SlmpClient::runMonitorCycle(uint16_t *word_values, uint16_t word_count, uint32_t *dword_values, uint16_t dword_count)
```

Execute monitor cycle (command 0x0802).

Returns values for the devices previously registered by registerMonitorDevices or registerMonitorDevicesExt.

#### `readBlock`

```cpp
Error slmp::SlmpClient::readBlock(const DeviceBlockRead *word_blocks, size_t word_block_count, const DeviceBlockRead *bit_blocks, size_t bit_block_count, uint16_t *word_values, size_t word_value_capacity, uint16_t *bit_values, size_t bit_value_capacity)
```

Read multiple contiguous blocks in one request. Blocks can be word-units or bit-units.

#### `writeBlock`

```cpp
Error slmp::SlmpClient::writeBlock(const DeviceBlockWrite *word_blocks, size_t word_block_count, const DeviceBlockWrite *bit_blocks, size_t bit_block_count)
```

Write multiple contiguous blocks in one request.

#### `writeBlock`

```cpp
Error slmp::SlmpClient::writeBlock(const DeviceBlockWrite *word_blocks, size_t word_block_count, const DeviceBlockWrite *bit_blocks, size_t bit_block_count, const BlockWriteOptions &options)
```

Write multiple contiguous blocks with options. Supports automatic splitting of mixed word/bit blocks if the PLC doesn't support them.

#### `remoteRun`

```cpp
Error slmp::SlmpClient::remoteRun(bool force=false, uint16_t clear_mode=0U)
```

Remote RUN command. Set PLC to RUN state.

#### `remoteStop`

```cpp
Error slmp::SlmpClient::remoteStop()
```

Remote STOP command. Set PLC to STOP state.

#### `remotePause`

```cpp
Error slmp::SlmpClient::remotePause(bool force=false)
```

Remote PAUSE command. Set PLC to PAUSE state.

#### `remoteLatchClear`

```cpp
Error slmp::SlmpClient::remoteLatchClear()
```

Remote LATCH CLEAR command.

#### `remoteReset`

```cpp
Error slmp::SlmpClient::remoteReset(uint16_t subcommand=0x0000U, bool expect_response=false)
```

Remote RESET command. (Warning: Connection will likely be lost).

#### `selfTestLoopback`

```cpp
Error slmp::SlmpClient::selfTestLoopback(const uint8_t *data, size_t data_length, uint8_t *out, size_t out_capacity, size_t &out_length)
```

Execute Self-test loopback. Verifies communication path by having the PLC echo back the provided data.

#### `clearError`

```cpp
Error slmp::SlmpClient::clearError()
```

Clear PLC error state (Resets the error LED/status).

#### `remotePasswordUnlock`

```cpp
Error slmp::SlmpClient::remotePasswordUnlock(const char *password)
```

Unlock remote password protected access.

#### `remotePasswordLock`

```cpp
Error slmp::SlmpClient::remotePasswordLock(const char *password)
```

Lock remote password protected access.

#### `update`

```cpp
void slmp::SlmpClient::update(uint32_t now_ms)
```

Advance asynchronous state machine. Must be called frequently in your main loop to process pending I/O.

now_ms Current system time in milliseconds.

| Parameter | Description |
| --- | --- |
| `now_ms` | Current system time in milliseconds. |

#### `isBusy`

```cpp
bool slmp::SlmpClient::isBusy() const
```

Check if an asynchronous operation is currently active.

#### `beginReadTypeName`

```cpp
Error slmp::SlmpClient::beginReadTypeName(TypeNameInfo &out, uint32_t now_ms)
```

Start async ReadTypeName. Result will be in out when busy becomes false.

#### `beginReadWords`

```cpp
Error slmp::SlmpClient::beginReadWords(const DeviceAddress &device, uint16_t points, uint16_t *values, size_t value_capacity, uint32_t now_ms)
```

Start async ReadWords. Result will be in values when busy becomes false.

#### `beginWriteWords`

```cpp
Error slmp::SlmpClient::beginWriteWords(const DeviceAddress &device, const uint16_t *values, size_t count, uint32_t now_ms)
```

Start async WriteWords.

#### `beginReadBits`

```cpp
Error slmp::SlmpClient::beginReadBits(const DeviceAddress &device, uint16_t points, bool *values, size_t value_capacity, uint32_t now_ms)
```

Start async ReadBits.

#### `beginWriteBits`

```cpp
Error slmp::SlmpClient::beginWriteBits(const DeviceAddress &device, const bool *values, size_t count, uint32_t now_ms)
```

Start async WriteBits.

#### `beginReadDWords`

```cpp
Error slmp::SlmpClient::beginReadDWords(const DeviceAddress &device, uint16_t points, uint32_t *values, size_t value_capacity, uint32_t now_ms)
```

Start async ReadDWords.

#### `beginWriteDWords`

```cpp
Error slmp::SlmpClient::beginWriteDWords(const DeviceAddress &device, const uint32_t *values, size_t count, uint32_t now_ms)
```

Start async WriteDWords.

#### `beginReadFloat32s`

```cpp
Error slmp::SlmpClient::beginReadFloat32s(const DeviceAddress &device, uint16_t points, float *values, size_t value_capacity, uint32_t now_ms)
```

Start async ReadFloat32s.

#### `beginWriteFloat32s`

```cpp
Error slmp::SlmpClient::beginWriteFloat32s(const DeviceAddress &device, const float *values, size_t count, uint32_t now_ms)
```

Start async WriteFloat32s.

#### `beginReadRandom`

```cpp
Error slmp::SlmpClient::beginReadRandom(const DeviceAddress *word_devices, size_t word_count, uint16_t *word_values, size_t word_value_capacity, const DeviceAddress *dword_devices, size_t dword_count, uint32_t *dword_values, size_t dword_value_capacity, uint32_t now_ms)
```

Start async ReadRandom.

#### `beginWriteRandomWords`

```cpp
Error slmp::SlmpClient::beginWriteRandomWords(const DeviceAddress *word_devices, const uint16_t *word_values, size_t word_count, const DeviceAddress *dword_devices, const uint32_t *dword_values, size_t dword_count, uint32_t now_ms)
```

Start async WriteRandomWords.

#### `beginWriteRandomBits`

```cpp
Error slmp::SlmpClient::beginWriteRandomBits(const DeviceAddress *bit_devices, const bool *bit_values, size_t bit_count, uint32_t now_ms)
```

Start async WriteRandomBits.

#### `beginReadRandomExt`

```cpp
Error slmp::SlmpClient::beginReadRandomExt(const ExtDeviceSpec *word_devices, size_t word_count, uint16_t *word_values, size_t word_value_capacity, const ExtDeviceSpec *dword_devices, size_t dword_count, uint32_t *dword_values, size_t dword_value_capacity, uint32_t now_ms)
```

Start async ReadRandomExt.

#### `beginWriteRandomWordsExt`

```cpp
Error slmp::SlmpClient::beginWriteRandomWordsExt(const ExtDeviceSpec *word_devices, const uint16_t *word_values, size_t word_count, const ExtDeviceSpec *dword_devices, const uint32_t *dword_values, size_t dword_count, uint32_t now_ms)
```

Start async WriteRandomWordsExt.

#### `beginWriteRandomBitsExt`

```cpp
Error slmp::SlmpClient::beginWriteRandomBitsExt(const ExtDeviceSpec *bit_devices, const bool *bit_values, size_t bit_count, uint32_t now_ms)
```

Start async WriteRandomBitsExt.

#### `beginRegisterMonitorDevices`

```cpp
Error slmp::SlmpClient::beginRegisterMonitorDevices(const DeviceAddress *word_devices, size_t word_count, const DeviceAddress *dword_devices, size_t dword_count, uint32_t now_ms)
```

Start async RegisterMonitorDevices.

#### `beginRegisterMonitorDevicesExt`

```cpp
Error slmp::SlmpClient::beginRegisterMonitorDevicesExt(const ExtDeviceSpec *word_devices, size_t word_count, const ExtDeviceSpec *dword_devices, size_t dword_count, uint32_t now_ms)
```

Start async RegisterMonitorDevicesExt.

#### `beginRunMonitorCycle`

```cpp
Error slmp::SlmpClient::beginRunMonitorCycle(uint16_t *word_values, uint16_t word_count, uint32_t *dword_values, uint16_t dword_count, uint32_t now_ms)
```

Start async RunMonitorCycle.

#### `beginReadBlock`

```cpp
Error slmp::SlmpClient::beginReadBlock(const DeviceBlockRead *word_blocks, size_t word_block_count, const DeviceBlockRead *bit_blocks, size_t bit_block_count, uint16_t *word_values, size_t word_value_capacity, uint16_t *bit_values, size_t bit_value_capacity, uint32_t now_ms)
```

Start async ReadBlock.

#### `beginWriteBlock`

```cpp
Error slmp::SlmpClient::beginWriteBlock(const DeviceBlockWrite *word_blocks, size_t word_block_count, const DeviceBlockWrite *bit_blocks, size_t bit_block_count, const BlockWriteOptions &options, uint32_t now_ms)
```

Start async WriteBlock.

#### `beginRemoteRun`

```cpp
Error slmp::SlmpClient::beginRemoteRun(bool force, uint16_t clear_mode, uint32_t now_ms)
```

Start async RemoteRun.

#### `beginRemoteStop`

```cpp
Error slmp::SlmpClient::beginRemoteStop(uint32_t now_ms)
```

Start async RemoteStop.

#### `beginRemotePause`

```cpp
Error slmp::SlmpClient::beginRemotePause(bool force, uint32_t now_ms)
```

Start async RemotePause.

#### `beginRemoteLatchClear`

```cpp
Error slmp::SlmpClient::beginRemoteLatchClear(uint32_t now_ms)
```

Start async RemoteLatchClear.

#### `beginRemoteReset`

```cpp
Error slmp::SlmpClient::beginRemoteReset(uint16_t subcommand, bool expect_response, uint32_t now_ms)
```

Start async RemoteReset.

#### `beginSelfTestLoopback`

```cpp
Error slmp::SlmpClient::beginSelfTestLoopback(const uint8_t *data, size_t data_length, uint8_t *out, size_t out_capacity, size_t *out_length, uint32_t now_ms)
```

Start async SelfTestLoopback.

#### `beginClearError`

```cpp
Error slmp::SlmpClient::beginClearError(uint32_t now_ms)
```

Start async ClearError.

#### `beginWriteBlock`

```cpp
Error slmp::SlmpClient::beginWriteBlock(const DeviceBlockWrite *word_blocks, size_t word_block_count, const DeviceBlockWrite *bit_blocks, size_t bit_block_count, uint32_t now_ms)
```

Start async WriteBlock (simple version).

#### `beginRemotePasswordUnlock`

```cpp
Error slmp::SlmpClient::beginRemotePasswordUnlock(const char *password, uint32_t now_ms)
```

Start async RemotePasswordUnlock.

#### `beginRemotePasswordLock`

```cpp
Error slmp::SlmpClient::beginRemotePasswordLock(const char *password, uint32_t now_ms)
```

Start async RemotePasswordLock.

#### `readLongTimer`

```cpp
Error slmp::SlmpClient::readLongTimer(int head_no, int points, LongTimerResult *out, size_t capacity)
```

Read one or more long timers (LTN device, 4 words per entry).

head_no Starting LTN device number (e.g. 0 for LTN0). points Number of timers to read. out Output buffer for decoded results. capacity Capacity of out buffer (in elements).

| Parameter | Description |
| --- | --- |
| `head_no` | Starting LTN device number (e.g. 0 for LTN0). |
| `points` | Number of timers to read. |
| `out` | Output buffer for decoded results. |
| `capacity` | Capacity of out buffer (in elements). |

#### `beginReadLongTimer`

```cpp
Error slmp::SlmpClient::beginReadLongTimer(int head_no, int points, LongTimerResult *out, size_t capacity, uint32_t now_ms)
```

Start async readLongTimer.

#### `readLongRetentiveTimer`

```cpp
Error slmp::SlmpClient::readLongRetentiveTimer(int head_no, int points, LongTimerResult *out, size_t capacity)
```

Read one or more long retentive timers (LSTN device, 4 words per entry).

#### `beginReadLongRetentiveTimer`

```cpp
Error slmp::SlmpClient::beginReadLongRetentiveTimer(int head_no, int points, LongTimerResult *out, size_t capacity, uint32_t now_ms)
```

Start async readLongRetentiveTimer.

#### `readLtcStates`

```cpp
Error slmp::SlmpClient::readLtcStates(int head_no, int points, bool *out, size_t capacity)
```

Read coil states (LTC) for long timers at headNo..headNo+points-1.

#### `readLtsStates`

```cpp
Error slmp::SlmpClient::readLtsStates(int head_no, int points, bool *out, size_t capacity)
```

Read contact states (LTS) for long timers.

#### `readLstcStates`

```cpp
Error slmp::SlmpClient::readLstcStates(int head_no, int points, bool *out, size_t capacity)
```

Read coil states (LSTC) for long retentive timers.

#### `readLstsStates`

```cpp
Error slmp::SlmpClient::readLstsStates(int head_no, int points, bool *out, size_t capacity)
```

Read contact states (LSTS) for long retentive timers.

#### `readWordsModuleBuf`

```cpp
Error slmp::SlmpClient::readWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t *out, size_t capacity)
```

Read words from intelligent module buffer memory.

slot Module slot number in hex (e.g. 3 for U3, 0x3E0 for U3E0). use_hg true for HG (extended buffer), false for G (normal buffer). dev_no Device number (buffer memory address). points Number of words to read. out Output buffer. capacity Capacity of out buffer (in elements).

| Parameter | Description |
| --- | --- |
| `slot` | Module slot number in hex (e.g. 3 for U3, 0x3E0 for U3E0). |
| `use_hg` | true for HG (extended buffer), false for G (normal buffer). |
| `dev_no` | Device number (buffer memory address). |
| `points` | Number of words to read. |
| `out` | Output buffer. |
| `capacity` | Capacity of out buffer (in elements). |

#### `beginReadWordsModuleBuf`

```cpp
Error slmp::SlmpClient::beginReadWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, uint16_t *out, size_t capacity, uint32_t now_ms)
```

Start async readWordsModuleBuf.

#### `writeWordsModuleBuf`

```cpp
Error slmp::SlmpClient::writeWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t *values, size_t count)
```

Write words to intelligent module buffer memory.

#### `beginWriteWordsModuleBuf`

```cpp
Error slmp::SlmpClient::beginWriteWordsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const uint16_t *values, size_t count, uint32_t now_ms)
```

Start async writeWordsModuleBuf.

#### `readBitsModuleBuf`

```cpp
Error slmp::SlmpClient::readBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool *out, size_t capacity)
```

Read bits from intelligent module buffer memory (sub 0x0081/0x0083).

#### `beginReadBitsModuleBuf`

```cpp
Error slmp::SlmpClient::beginReadBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, uint16_t points, bool *out, size_t capacity, uint32_t now_ms)
```

Start async readBitsModuleBuf.

#### `writeBitsModuleBuf`

```cpp
Error slmp::SlmpClient::writeBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool *values, size_t count)
```

Write bits to intelligent module buffer memory (sub 0x0081/0x0083).

#### `beginWriteBitsModuleBuf`

```cpp
Error slmp::SlmpClient::beginWriteBitsModuleBuf(uint16_t slot, bool use_hg, uint32_t dev_no, const bool *values, size_t count, uint32_t now_ms)
```

Start async writeBitsModuleBuf.

#### `readWordsLinkDirect`

```cpp
Error slmp::SlmpClient::readWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t *out, size_t capacity)
```

Read words from a link direct device (CC-Link IE J&#92;device).

j_net J-network number (0..255). code Device code. dev_no Device number. points Number of words to read. out Output buffer. capacity Capacity of out buffer (in elements).

| Parameter | Description |
| --- | --- |
| `j_net` | J-network number (0..255). |
| `code` | Device code. |
| `dev_no` | Device number. |
| `points` | Number of words to read. |
| `out` | Output buffer. |
| `capacity` | Capacity of out buffer (in elements). |

#### `beginReadWordsLinkDirect`

```cpp
Error slmp::SlmpClient::beginReadWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, uint16_t *out, size_t capacity, uint32_t now_ms)
```

Start async readWordsLinkDirect.

#### `writeWordsLinkDirect`

```cpp
Error slmp::SlmpClient::writeWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t *values, size_t count)
```

Write words to a link direct device.

#### `beginWriteWordsLinkDirect`

```cpp
Error slmp::SlmpClient::beginWriteWordsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const uint16_t *values, size_t count, uint32_t now_ms)
```

Start async writeWordsLinkDirect.

#### `readBitsLinkDirect`

```cpp
Error slmp::SlmpClient::readBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool *out, size_t capacity)
```

Read bits from a link direct device (16-point units).

#### `beginReadBitsLinkDirect`

```cpp
Error slmp::SlmpClient::beginReadBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, uint16_t points, bool *out, size_t capacity, uint32_t now_ms)
```

Start async readBitsLinkDirect.

#### `writeBitsLinkDirect`

```cpp
Error slmp::SlmpClient::writeBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool *values, size_t count)
```

Write bits to a link direct device (16-point units).

#### `beginWriteBitsLinkDirect`

```cpp
Error slmp::SlmpClient::beginWriteBitsLinkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no, const bool *values, size_t count, uint32_t now_ms)
```

Start async writeBitsLinkDirect.

#### `readMemoryWords`

```cpp
Error slmp::SlmpClient::readMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t *out, size_t capacity)
```

Read words from PLC memory (command 0x0613).

head_address 32-bit starting memory address. word_length Number of words to read. out Output buffer. capacity Capacity of out buffer (in elements).

| Parameter | Description |
| --- | --- |
| `head_address` | 32-bit starting memory address. |
| `word_length` | Number of words to read. |
| `out` | Output buffer. |
| `capacity` | Capacity of out buffer (in elements). |

#### `beginReadMemoryWords`

```cpp
Error slmp::SlmpClient::beginReadMemoryWords(uint32_t head_address, uint16_t word_length, uint16_t *out, size_t capacity, uint32_t now_ms)
```

Start async readMemoryWords.

#### `writeMemoryWords`

```cpp
Error slmp::SlmpClient::writeMemoryWords(uint32_t head_address, const uint16_t *values, size_t count)
```

Write words to PLC memory (command 0x1613).

#### `beginWriteMemoryWords`

```cpp
Error slmp::SlmpClient::beginWriteMemoryWords(uint32_t head_address, const uint16_t *values, size_t count, uint32_t now_ms)
```

Start async writeMemoryWords.

#### `readExtendUnitBytes`

```cpp
Error slmp::SlmpClient::readExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t *out, size_t capacity)
```

Read raw bytes from an extend unit (command 0x0601).

head_address 32-bit starting address. byte_length Number of bytes to read. module_no Extend unit module I/O number (e.g. 0x03E0 for CPU buffer). out Output buffer. capacity Capacity of out buffer in bytes.

| Parameter | Description |
| --- | --- |
| `head_address` | 32-bit starting address. |
| `byte_length` | Number of bytes to read. |
| `module_no` | Extend unit module I/O number (e.g. 0x03E0 for CPU buffer). |
| `out` | Output buffer. |
| `capacity` | Capacity of out buffer in bytes. |

#### `beginReadExtendUnitBytes`

```cpp
Error slmp::SlmpClient::beginReadExtendUnitBytes(uint32_t head_address, uint16_t byte_length, uint16_t module_no, uint8_t *out, size_t capacity, uint32_t now_ms)
```

Start async readExtendUnitBytes.

#### `readExtendUnitWords`

```cpp
Error slmp::SlmpClient::readExtendUnitWords(uint32_t head_address, uint16_t word_length, uint16_t module_no, uint16_t *out, size_t capacity)
```

Read words from an extend unit (command 0x0601).

#### `readExtendUnitWord`

```cpp
Error slmp::SlmpClient::readExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t &value)
```

Read single word from an extend unit.

#### `readExtendUnitDWord`

```cpp
Error slmp::SlmpClient::readExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t &value)
```

Read single DWord (2 words, little-endian) from an extend unit.

#### `writeExtendUnitBytes`

```cpp
Error slmp::SlmpClient::writeExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t *data, size_t byte_length)
```

Write raw bytes to an extend unit (command 0x1601).

#### `beginWriteExtendUnitBytes`

```cpp
Error slmp::SlmpClient::beginWriteExtendUnitBytes(uint32_t head_address, uint16_t module_no, const uint8_t *data, size_t byte_length, uint32_t now_ms)
```

Start async writeExtendUnitBytes.

#### `writeExtendUnitWords`

```cpp
Error slmp::SlmpClient::writeExtendUnitWords(uint32_t head_address, uint16_t module_no, const uint16_t *values, size_t count)
```

Write words to an extend unit (command 0x1601).

#### `writeExtendUnitWord`

```cpp
Error slmp::SlmpClient::writeExtendUnitWord(uint32_t head_address, uint16_t module_no, uint16_t value)
```

Write single word to an extend unit.

#### `writeExtendUnitDWord`

```cpp
Error slmp::SlmpClient::writeExtendUnitDWord(uint32_t head_address, uint16_t module_no, uint32_t value)
```

Write single DWord (2 words, little-endian) to an extend unit.

#### `readCpuBufferBytes`

```cpp
Error slmp::SlmpClient::readCpuBufferBytes(uint32_t head_address, uint16_t byte_length, uint8_t *out, size_t capacity)
```

Read bytes from the CPU buffer (extend unit 0x03E0).

#### `readCpuBufferWords`

```cpp
Error slmp::SlmpClient::readCpuBufferWords(uint32_t head_address, uint16_t word_length, uint16_t *out, size_t capacity)
```

Read words from the CPU buffer (extend unit 0x03E0).

#### `readCpuBufferWord`

```cpp
Error slmp::SlmpClient::readCpuBufferWord(uint32_t head_address, uint16_t &value)
```

Read single word from the CPU buffer.

#### `readCpuBufferDWord`

```cpp
Error slmp::SlmpClient::readCpuBufferDWord(uint32_t head_address, uint32_t &value)
```

Read single DWord (2 words, little-endian) from the CPU buffer.

#### `writeCpuBufferBytes`

```cpp
Error slmp::SlmpClient::writeCpuBufferBytes(uint32_t head_address, const uint8_t *data, size_t byte_length)
```

Write bytes to the CPU buffer (extend unit 0x03E0).

#### `writeCpuBufferWords`

```cpp
Error slmp::SlmpClient::writeCpuBufferWords(uint32_t head_address, const uint16_t *values, size_t count)
```

Write words to the CPU buffer (extend unit 0x03E0).

#### `writeCpuBufferWord`

```cpp
Error slmp::SlmpClient::writeCpuBufferWord(uint32_t head_address, uint16_t value)
```

Write single word to the CPU buffer.

#### `writeCpuBufferDWord`

```cpp
Error slmp::SlmpClient::writeCpuBufferDWord(uint32_t head_address, uint32_t value)
```

Write single DWord (2 words, little-endian) to the CPU buffer.

#### `readArrayLabels`

```cpp
Error slmp::SlmpClient::readArrayLabels(const LabelArrayReadPoint *points, size_t point_count, LabelArrayReadResult *out, size_t out_capacity, size_t *out_count, const LabelName *abbrevs=nullptr, size_t abbrev_count=0)
```

Read array labels from the PLC (command 0x041A).

points Array of read point descriptors. point_count Number of points. out Output buffer for results. result.data pointers are valid until the next operation. out_capacity Capacity of out buffer (in elements). out_count Receives the number of results written. abbrevs Optional abbreviation label names (may be nullptr). abbrev_count Number of abbreviation labels.

| Parameter | Description |
| --- | --- |
| `points` | Array of read point descriptors. |
| `point_count` | Number of points. |
| `out` | Output buffer for results. result.data pointers are valid until the next operation. |
| `out_capacity` | Capacity of out buffer (in elements). |
| `out_count` | Receives the number of results written. |
| `abbrevs` | Optional abbreviation label names (may be nullptr). |
| `abbrev_count` | Number of abbreviation labels. |

#### `beginReadArrayLabels`

```cpp
Error slmp::SlmpClient::beginReadArrayLabels(const LabelArrayReadPoint *points, size_t point_count, LabelArrayReadResult *out, size_t out_capacity, size_t *out_count, const LabelName *abbrevs, size_t abbrev_count, uint32_t now_ms)
```

Start async readArrayLabels.

#### `writeArrayLabels`

```cpp
Error slmp::SlmpClient::writeArrayLabels(const LabelArrayWritePoint *points, size_t point_count, const LabelName *abbrevs=nullptr, size_t abbrev_count=0)
```

Write array labels to the PLC (command 0x141A).

#### `beginWriteArrayLabels`

```cpp
Error slmp::SlmpClient::beginWriteArrayLabels(const LabelArrayWritePoint *points, size_t point_count, const LabelName *abbrevs, size_t abbrev_count, uint32_t now_ms)
```

Start async writeArrayLabels.

#### `readRandomLabels`

```cpp
Error slmp::SlmpClient::readRandomLabels(const LabelName *labels, size_t label_count, LabelRandomReadResult *out, size_t out_capacity, size_t *out_count, const LabelName *abbrevs=nullptr, size_t abbrev_count=0)
```

Read random labels from the PLC (command 0x041C).

labels Array of label names. label_count Number of labels. out Output buffer for results. out_capacity Capacity of out buffer. out_count Receives the number of results written. abbrevs Optional abbreviation labels. abbrev_count Number of abbreviation labels.

| Parameter | Description |
| --- | --- |
| `labels` | Array of label names. |
| `label_count` | Number of labels. |
| `out` | Output buffer for results. |
| `out_capacity` | Capacity of out buffer. |
| `out_count` | Receives the number of results written. |
| `abbrevs` | Optional abbreviation labels. |
| `abbrev_count` | Number of abbreviation labels. |

#### `beginReadRandomLabels`

```cpp
Error slmp::SlmpClient::beginReadRandomLabels(const LabelName *labels, size_t label_count, LabelRandomReadResult *out, size_t out_capacity, size_t *out_count, const LabelName *abbrevs, size_t abbrev_count, uint32_t now_ms)
```

Start async readRandomLabels.

#### `writeRandomLabels`

```cpp
Error slmp::SlmpClient::writeRandomLabels(const LabelRandomWritePoint *points, size_t point_count, const LabelName *abbrevs=nullptr, size_t abbrev_count=0)
```

Write random labels to the PLC (command 0x141B).

#### `beginWriteRandomLabels`

```cpp
Error slmp::SlmpClient::beginWriteRandomLabels(const LabelRandomWritePoint *points, size_t point_count, const LabelName *abbrevs, size_t abbrev_count, uint32_t now_ms)
```

Start async writeRandomLabels.

### Class `slmp::ReconnectHelper`

Automates PLC connection maintenance.

Handles periodic retry logic when the connection is lost, ensuring the application remains connected without blocking the main loop.

#### Member Functions

#### `ReconnectHelper`

```cpp
slmp::ReconnectHelper::ReconnectHelper(SlmpClient &client, const char *host, uint16_t port, const ReconnectOptions &options=ReconnectOptions())
```

Initialize helper with client and target endpoint.

client Reference to an initialized SlmpClient. host Target IP address or hostname. port Target Port (usually 1025 or 1280). options Reconnection configuration.

| Parameter | Description |
| --- | --- |
| `client` | Reference to an initialized SlmpClient. |
| `host` | Target IP address or hostname. |
| `port` | Target Port (usually 1025 or 1280). |
| `options` | Reconnection configuration. |

#### `ensureConnected`

```cpp
bool slmp::ReconnectHelper::ensureConnected(uint32_t now_ms)
```

Check and restore connection if needed.

If already connected, returns true immediately. If disconnected, attempts to connect only if ReconnectOptions::retry_interval_ms has elapsed since the last attempt.

now_ms Current system time in milliseconds. true if currently connected (or just successfully reconnected).

| Parameter | Description |
| --- | --- |
| `now_ms` | Current system time in milliseconds. |

Returns: true if currently connected (or just successfully reconnected).

#### `consumeConnectedEdge`

```cpp
bool slmp::ReconnectHelper::consumeConnectedEdge()
```

Detect a rising edge of the connection state.

Returns true only ONCE when a new connection is established. Useful for triggering initialization logic like reading PLC type or setting up monitors.

true if a NEW connection was established since the last call to this method.

Returns: true if a NEW connection was established since the last call to this method.

#### `forceReconnect`

```cpp
void slmp::ReconnectHelper::forceReconnect(uint32_t now_ms)
```

Manually trigger a reconnection attempt on the next call to ensureConnected.

Closes the current connection and resets the retry timer to allow an immediate attempt if desired.

## Structs

### Struct `slmp::highlevel::Value`

Tagged logical value used by typed read, write, and named snapshots.

Only the field that matches type is meaningful. This struct is intentionally simple so it can be stack-allocated, copied into snapshots, and passed through firmware code without pulling in a heavier variant type.

#### Fields

#### `type`

```cpp
ValueType slmp::highlevel::Value::type = ValueType::U16
```

Declared logical type of this value

#### `bit`

```cpp
bool slmp::highlevel::Value::bit = false
```

Boolean payload for ValueType::Bit

#### `u16`

```cpp
uint16_t slmp::highlevel::Value::u16 = 0U
```

Unsigned 16-bit payload

#### `s16`

```cpp
int16_t slmp::highlevel::Value::s16 = 0
```

Signed 16-bit payload

#### `u32`

```cpp
uint32_t slmp::highlevel::Value::u32 = 0UL
```

Unsigned 32-bit payload

#### `s32`

```cpp
int32_t slmp::highlevel::Value::s32 = 0L
```

Signed 32-bit payload

#### `f32`

```cpp
float slmp::highlevel::Value::f32 = 0.0f
```

Float32 payload

#### Member Functions

#### `bitValue`

```cpp
static Value slmp::highlevel::Value::bitValue(bool value)
```

Create a boolean logical value.

value Boolean payload. Tagged Value initialized as ValueType::Bit.

| Parameter | Description |
| --- | --- |
| `value` | Boolean payload. |

Returns: Tagged Value initialized as ValueType::Bit.

#### `u16Value`

```cpp
static Value slmp::highlevel::Value::u16Value(uint16_t value)
```

Create an unsigned 16-bit logical value.

value Unsigned 16-bit payload. Tagged Value initialized as ValueType::U16.

| Parameter | Description |
| --- | --- |
| `value` | Unsigned 16-bit payload. |

Returns: Tagged Value initialized as ValueType::U16.

#### `s16Value`

```cpp
static Value slmp::highlevel::Value::s16Value(int16_t value)
```

Create a signed 16-bit logical value.

value Signed 16-bit payload. Tagged Value initialized as ValueType::S16.

| Parameter | Description |
| --- | --- |
| `value` | Signed 16-bit payload. |

Returns: Tagged Value initialized as ValueType::S16.

#### `u32Value`

```cpp
static Value slmp::highlevel::Value::u32Value(uint32_t value)
```

Create an unsigned 32-bit logical value.

value Unsigned 32-bit payload. Tagged Value initialized as ValueType::U32.

| Parameter | Description |
| --- | --- |
| `value` | Unsigned 32-bit payload. |

Returns: Tagged Value initialized as ValueType::U32.

#### `s32Value`

```cpp
static Value slmp::highlevel::Value::s32Value(int32_t value)
```

Create a signed 32-bit logical value.

value Signed 32-bit payload. Tagged Value initialized as ValueType::S32.

| Parameter | Description |
| --- | --- |
| `value` | Signed 32-bit payload. |

Returns: Tagged Value initialized as ValueType::S32.

#### `float32Value`

```cpp
static Value slmp::highlevel::Value::float32Value(float value)
```

Create an IEEE-754 float32 logical value.

value Float payload. Tagged Value initialized as ValueType::Float32.

| Parameter | Description |
| --- | --- |
| `value` | Float payload. |

Returns: Tagged Value initialized as ValueType::Float32.

### Struct `slmp::highlevel::AddressSpec`

Parsed logical address accepted by the high-level helpers.

Examples: - D100:U - D100:S - D200:F - D50.3 - M1000:BIT

Device families accepted by the parser include: - standard relay/register devices such as D, M, X, Y, R, ZR - long timer families LTN, LTS, LTC - long retentive timer families LSTN, LSTS, LSTC - long counter families LCN, LCS, LCC - index/register families Z, LZ, and RD

The .bit notation is valid only for word devices and maps to a read-modify-write flow on writes.

#### Fields

#### `device`

```cpp
DeviceAddress slmp::highlevel::AddressSpec::device {}
```

Base device address used by the low-level API

#### `type`

```cpp
ValueType slmp::highlevel::AddressSpec::type = ValueType::U16
```

Logical value type requested by the caller

#### `bit_index`

```cpp
int slmp::highlevel::AddressSpec::bit_index = -1
```

Bit index for D50.3 style access, otherwise -1

#### `explicit_type`

```cpp
bool slmp::highlevel::AddressSpec::explicit_type = false
```

True when the input used an explicit suffix such as :F

### Struct `slmp::highlevel::ReadPlanEntry`

One compiled entry in a named-read or poll plan.

A plan entry keeps both the original caller string and the normalized low-level interpretation so repeated polling does not need to re-parse the address expression.

#### Fields

#### `address`

```cpp
std::string slmp::highlevel::ReadPlanEntry::address
```

Original caller-provided address string

#### `spec`

```cpp
AddressSpec slmp::highlevel::ReadPlanEntry::spec
```

Parsed logical address

#### `kind`

```cpp
BatchKind slmp::highlevel::ReadPlanEntry::kind = BatchKind::None
```

Batching strategy selected for this address

### Struct `slmp::highlevel::ReadPlan`

Compiled snapshot plan reused by readNamed and Poller.

The entries vector preserves caller order for user-facing results. The word_devices and dword_devices vectors hold deduplicated batchable devices in the order chosen by the compiler so the runtime can perform one random-read request for many named addresses.

#### Fields

#### `entries`

```cpp
std::vector<ReadPlanEntry> slmp::highlevel::ReadPlan::entries
```

Snapshot entries in caller order

#### `word_devices`

```cpp
std::vector<DeviceAddress> slmp::highlevel::ReadPlan::word_devices
```

Unique batched word devices

#### `dword_devices`

```cpp
std::vector<DeviceAddress> slmp::highlevel::ReadPlan::dword_devices
```

Unique batched dword devices

### Struct `slmp::highlevel::NamedValue`

One result item in a named snapshot.

#### Fields

#### `address`

```cpp
std::string slmp::highlevel::NamedValue::address
```

Original caller-provided address string

#### `value`

```cpp
Value slmp::highlevel::NamedValue::value
```

Decoded logical value

### Struct `slmp::highlevel::PlcProfileDefaults`

Resolved fixed defaults for one PlcProfile.

#### Fields

#### `frame_type`

```cpp
FrameType slmp::highlevel::PlcProfileDefaults::frame_type
```

Derived SLMP frame type

#### `compatibility_mode`

```cpp
CompatibilityMode slmp::highlevel::PlcProfileDefaults::compatibility_mode
```

Derived low-level compatibility mode

#### `address_profile`

```cpp
PlcProfile slmp::highlevel::PlcProfileDefaults::address_profile
```

String-address profile used by the helper parser

#### `range_profile`

```cpp
PlcProfile slmp::highlevel::PlcProfileDefaults::range_profile
```

PLC profile used for SD-range helpers

### Struct `slmp::highlevel::DeviceRangeEntry`

One device-range row for one public device code such as D or STS.

#### Fields

#### `device`

```cpp
std::string slmp::highlevel::DeviceRangeEntry::device
```

Public device code such as D or X

#### `category`

```cpp
DeviceRangeCategory slmp::highlevel::DeviceRangeEntry::category = DeviceRangeCategory::Word
```

Logical category for grouping

#### `is_bit_device`

```cpp
bool slmp::highlevel::DeviceRangeEntry::is_bit_device = false
```

True when this device is normally bit-addressed

#### `supported`

```cpp
bool slmp::highlevel::DeviceRangeEntry::supported = false
```

False when the PLC profile does not support this device

#### `lower_bound`

```cpp
uint32_t slmp::highlevel::DeviceRangeEntry::lower_bound = 0U
```

Always zero in the current profile rules

#### `upper_bound`

```cpp
uint32_t slmp::highlevel::DeviceRangeEntry::upper_bound = 0U
```

Inclusive last address when has_upper_bound is true

#### `has_upper_bound`

```cpp
bool slmp::highlevel::DeviceRangeEntry::has_upper_bound = false
```

True when a finite last address is known

#### `point_count`

```cpp
uint32_t slmp::highlevel::DeviceRangeEntry::point_count = 0U
```

Configured point count when has_point_count is true

#### `has_point_count`

```cpp
bool slmp::highlevel::DeviceRangeEntry::has_point_count = false
```

True when the PLC/configuration exposes a usable count

#### `address_range`

```cpp
std::string slmp::highlevel::DeviceRangeEntry::address_range
```

Preformatted range such as X0000-X1777 or empty when unavailable

#### `notation`

```cpp
DeviceRangeNotation slmp::highlevel::DeviceRangeEntry::notation = DeviceRangeNotation::Base10
```

Rendered numbering style

#### `source`

```cpp
std::string slmp::highlevel::DeviceRangeEntry::source
```

Rule source such as SD300 or Fixed family limit

#### `notes`

```cpp
std::string slmp::highlevel::DeviceRangeEntry::notes
```

Optional profile-specific note

### Struct `slmp::highlevel::DeviceRangeCatalog`

Resolved device-range catalog for one explicit PLC profile selection.

#### Fields

#### `model`

```cpp
std::string slmp::highlevel::DeviceRangeCatalog::model
```

Synthetic profile label such as IQ-F

#### `model_code`

```cpp
uint16_t slmp::highlevel::DeviceRangeCatalog::model_code = 0U
```

Always zero for explicit profile reads

#### `has_model_code`

```cpp
bool slmp::highlevel::DeviceRangeCatalog::has_model_code = false
```

False because no type-name query is used here

#### `plc_profile`

```cpp
PlcProfile slmp::highlevel::DeviceRangeCatalog::plc_profile = PlcProfile::IqR
```

Caller-selected PLC profile

#### `entries`

```cpp
std::vector<DeviceRangeEntry> slmp::highlevel::DeviceRangeCatalog::entries
```

Device rows in stable output order

### Struct `slmp::SlmpErrorInfo`

Structured SLMP error information returned after a non-zero end code.

#### Fields

#### `present`

```cpp
bool slmp::SlmpErrorInfo::present = false
```

True when the PLC response contained the 9-byte error information block

#### `network`

```cpp
uint8_t slmp::SlmpErrorInfo::network = 0U
```

Network number reported by the PLC

#### `station`

```cpp
uint8_t slmp::SlmpErrorInfo::station = 0U
```

Station number reported by the PLC

#### `module_io`

```cpp
uint16_t slmp::SlmpErrorInfo::module_io = 0U
```

Module I/O number reported by the PLC

#### `multidrop`

```cpp
uint8_t slmp::SlmpErrorInfo::multidrop = 0U
```

Multidrop station number reported by the PLC

#### `command`

```cpp
uint16_t slmp::SlmpErrorInfo::command = 0U
```

Command code associated with the PLC error

#### `subcommand`

```cpp
uint16_t slmp::SlmpErrorInfo::subcommand = 0U
```

Subcommand code associated with the PLC error

#### `raw`

```cpp
uint8_t slmp::SlmpErrorInfo::raw[9] = {}
```

Raw 9-byte error information block

### Struct `slmp::ProfileFeatureErrorInfo`

Structured information for Error::ProfileFeatureBlocked.

#### Fields

#### `present`

```cpp
bool slmp::ProfileFeatureErrorInfo::present = false
```

True when the last error was produced by the profile guard

#### `profile`

```cpp
PlcProfile slmp::ProfileFeatureErrorInfo::profile = PlcProfile::Unspecified
```

Selected PLC profile

#### `profile_id`

```cpp
const char* slmp::ProfileFeatureErrorInfo::profile_id = nullptr
```

Canonical profile ID such as melsec:qnudv

#### `feature_key`

```cpp
const char* slmp::ProfileFeatureErrorInfo::feature_key = nullptr
```

Capability feature key such as block

#### `state`

```cpp
const char* slmp::ProfileFeatureErrorInfo::state = nullptr
```

Capability state, blocked or unverified

#### `evidence`

```cpp
const char* slmp::ProfileFeatureErrorInfo::evidence = nullptr
```

Evidence or policy note from the embedded table

#### `disable_hint`

```cpp
const char* slmp::ProfileFeatureErrorInfo::disable_hint = nullptr
```

How to intentionally bypass the feature guard

### Struct `slmp::DeviceAddress`

Represents a specific device and its numeric address.

#### Fields

#### `code`

```cpp
DeviceCode slmp::DeviceAddress::code
```

Device type code (e.g. D, M, X)

#### `number`

```cpp
uint32_t slmp::DeviceAddress::number
```

Numeric address (index). Use dev::dec or dev::hex

### Struct `slmp::DeviceBlockRead`

Description for a contiguous block of devices to read.

#### Fields

#### `device`

```cpp
DeviceAddress slmp::DeviceBlockRead::device
```

Starting device address

#### `points`

```cpp
uint16_t slmp::DeviceBlockRead::points
```

Number of points to read

### Struct `slmp::DeviceBlockWrite`

Description for a contiguous block of devices to write.

#### Fields

#### `device`

```cpp
DeviceAddress slmp::DeviceBlockWrite::device
```

Starting device address

#### `values`

```cpp
const uint16_t* slmp::DeviceBlockWrite::values
```

Pointer to word values to write

#### `points`

```cpp
uint16_t slmp::DeviceBlockWrite::points
```

Number of points to write

### Struct `slmp::BlockWriteOptions`

Configuration for block write operations.

#### Fields

#### `split_mixed_blocks`

```cpp
bool slmp::BlockWriteOptions::split_mixed_blocks
```

Split bit and word blocks into separate requests

### Struct `slmp::dev::DecNo`

Wrapper type used to mark a device number as decimal.

#### Fields

#### `value`

```cpp
uint32_t slmp::dev::DecNo::value
```

### Struct `slmp::dev::HexNo`

Wrapper type used to mark a device number as hexadecimal.

#### Fields

#### `value`

```cpp
uint32_t slmp::dev::HexNo::value
```

### Struct `slmp::LongTimerResult`

Decoded result for one long timer or long retentive timer entry.

Each entry occupies 4 words in the device memory: [current_lo, current_hi, status_word, reserved].

This structure is returned by the dedicated long timer helpers instead of exposing the raw 4-word payload directly. The helper keeps the wire format available through status_word while also decoding the current value and the commonly-used contact / coil bits.

#### Fields

#### `current_value`

```cpp
uint32_t slmp::LongTimerResult::current_value
```

Current timer value assembled from current_lo and current_hi

#### `contact`

```cpp
bool slmp::LongTimerResult::contact
```

Contact state (LTS / LSTS, decoded from status_word & 0x0002)

#### `coil`

```cpp
bool slmp::LongTimerResult::coil
```

Coil state (LTC / LSTC, decoded from status_word & 0x0001)

#### `status_word`

```cpp
uint16_t slmp::LongTimerResult::status_word
```

Raw status word returned by the PLC for callers that need bit-exact inspection

### Struct `slmp::LabelName`

UTF-16-LE label name for label commands.

Use 2-byte characters as-is from the label string. staticconstuint16_tkMyLabel[]={'V','a','r','1'}; slmp::LabelNamename={kMyLabel,4};

#### Fields

#### `chars`

```cpp
const uint16_t* slmp::LabelName::chars
```

UTF-16-LE characters

#### `length`

```cpp
uint16_t slmp::LabelName::length
```

Number of characters (not bytes)

### Struct `slmp::LabelArrayReadPoint`

Single label read entry for SlmpClient::readArrayLabels.

#### Fields

#### `label`

```cpp
LabelName slmp::LabelArrayReadPoint::label
```

#### `unit_specification`

```cpp
uint8_t slmp::LabelArrayReadPoint::unit_specification
```

0=word (2 bytes/element), 1=byte (1 byte/element)

#### `array_data_length`

```cpp
uint16_t slmp::LabelArrayReadPoint::array_data_length
```

Number of elements

### Struct `slmp::LabelArrayReadResult`

Single label read result from SlmpClient::readArrayLabels.

data points into the internal rx_buffer and is valid only until the next operation.

#### Fields

#### `dt_id`

```cpp
uint8_t slmp::LabelArrayReadResult::dt_id
```

Data type identifier

#### `unit_specification`

```cpp
uint8_t slmp::LabelArrayReadResult::unit_specification
```

0=word, 1=byte

#### `array_data_length`

```cpp
uint16_t slmp::LabelArrayReadResult::array_data_length
```

Number of elements

#### `data`

```cpp
const uint8_t* slmp::LabelArrayReadResult::data
```

Pointer into rx_buffer (valid until next operation)

#### `data_bytes`

```cpp
size_t slmp::LabelArrayReadResult::data_bytes
```

Byte length of data

### Struct `slmp::LabelArrayWritePoint`

Single label write entry for SlmpClient::writeArrayLabels.

#### Fields

#### `label`

```cpp
LabelName slmp::LabelArrayWritePoint::label
```

#### `unit_specification`

```cpp
uint8_t slmp::LabelArrayWritePoint::unit_specification
```

0=word, 1=byte

#### `array_data_length`

```cpp
uint16_t slmp::LabelArrayWritePoint::array_data_length
```

Number of elements

#### `data`

```cpp
const uint8_t* slmp::LabelArrayWritePoint::data
```

Data to write

#### `data_bytes`

```cpp
size_t slmp::LabelArrayWritePoint::data_bytes
```

Byte length of data

### Struct `slmp::LabelRandomReadResult`

Single result from SlmpClient::readRandomLabels.

data points into the internal rx_buffer and is valid only until the next operation.

#### Fields

#### `dt_id`

```cpp
uint8_t slmp::LabelRandomReadResult::dt_id
```

Data type identifier

#### `spare`

```cpp
uint8_t slmp::LabelRandomReadResult::spare
```

Reserved byte

#### `result_length`

```cpp
uint16_t slmp::LabelRandomReadResult::result_length
```

Byte length of data

#### `data`

```cpp
const uint8_t* slmp::LabelRandomReadResult::data
```

Pointer into rx_buffer (valid until next operation)

### Struct `slmp::LabelRandomWritePoint`

Single write entry for SlmpClient::writeRandomLabels.

#### Fields

#### `label`

```cpp
LabelName slmp::LabelRandomWritePoint::label
```

#### `data`

```cpp
const uint8_t* slmp::LabelRandomWritePoint::data
```

Data to write

#### `data_bytes`

```cpp
uint16_t slmp::LabelRandomWritePoint::data_bytes
```

Byte length of data

### Struct `slmp::ExtDeviceSpec`

Extended device address for Ext variants (readRandomExt, etc.).

Represents either a module buffer (U&#92;G / U&#92;HG) or link direct (J&#92;device) target. Use the static factory methods to construct.

#### Enums

#### `Kind`

| Value | Description |
| --- | --- |
| `ModuleBuf` |  |
| `LinkDirect` |  |

#### Fields

#### `kind`

```cpp
enum slmp::ExtDeviceSpec::Kind slmp::ExtDeviceSpec::kind
```

#### `dev_no`

```cpp
uint32_t slmp::ExtDeviceSpec::dev_no
```

#### `mod`

```cpp
struct slmp::ExtDeviceSpec slmp::ExtDeviceSpec::mod
```

#### `slot`

```cpp
uint16_t slmp::ExtDeviceSpec::slot
```

#### `use_hg`

```cpp
bool slmp::ExtDeviceSpec::use_hg
```

#### `code`

```cpp
DeviceCode slmp::ExtDeviceSpec::code
```

#### `j_net`

```cpp
uint8_t slmp::ExtDeviceSpec::j_net
```

#### `link`

```cpp
struct slmp::ExtDeviceSpec slmp::ExtDeviceSpec::link
```

#### Member Functions

#### `moduleBuf`

```cpp
static ExtDeviceSpec slmp::ExtDeviceSpec::moduleBuf(uint16_t slot, bool use_hg, uint32_t dev_no) noexcept
```

Construct a module buffer device (U&#92;G or U&#92;HG).

#### `linkDirect`

```cpp
static ExtDeviceSpec slmp::ExtDeviceSpec::linkDirect(uint8_t j_net, DeviceCode code, uint32_t dev_no) noexcept
```

Construct a link direct device (J&#92;device).

### Struct `slmp::TargetAddress`

SLMP target station routing information.

#### Fields

#### `network`

```cpp
uint8_t slmp::TargetAddress::network = 0x00
```

Network number (0=Local)

#### `station`

```cpp
uint8_t slmp::TargetAddress::station = 0xFF
```

Station number (255=Control CPU)

#### `module_io`

```cpp
uint16_t slmp::TargetAddress::module_io = ::slmp::module_io::ConnectedCpu
```

Module I/O number (0x03FF=Own Station)

#### `multidrop`

```cpp
uint8_t slmp::TargetAddress::multidrop = 0x00
```

Multidrop station number

### Struct `slmp::TypeNameInfo`

PLC model and hardware information.

#### Fields

#### `model`

```cpp
char slmp::TypeNameInfo::model[17]
```

Model name string (max 16 chars + null)

#### `model_code`

```cpp
uint16_t slmp::TypeNameInfo::model_code
```

Internal model code

#### `has_model_code`

```cpp
bool slmp::TypeNameInfo::has_model_code
```

True if model code is valid

### Struct `slmp::CpuOperationState`

Decoded CPU operation state read from SD203.

#### Fields

#### `status`

```cpp
CpuOperationStatus slmp::CpuOperationState::status = CpuOperationStatus::Unknown
```

Decoded operation state

#### `raw_status_word`

```cpp
uint16_t slmp::CpuOperationState::raw_status_word = 0U
```

Full raw word value read from SD203

#### `raw_code`

```cpp
uint8_t slmp::CpuOperationState::raw_code = 0U
```

Lower 4-bit masked status code from SD203

### Struct `slmp::SlmpClient::AsyncContext`

#### Enums

#### `Type`

| Value | Description |
| --- | --- |
| `None` |  |
| `ReadTypeName` |  |
| `ReadWords` |  |
| `WriteWords` |  |
| `ReadBits` |  |
| `WriteBits` |  |
| `ReadDWords` |  |
| `WriteDWords` |  |
| `ReadFloat32s` |  |
| `WriteFloat32s` |  |
| `ReadRandom` |  |
| `WriteRandomWords` |  |
| `WriteRandomBits` |  |
| `ReadBlock` |  |
| `WriteBlock` |  |
| `RemoteRun` |  |
| `RemoteStop` |  |
| `RemotePause` |  |
| `RemoteLatchClear` |  |
| `RemoteReset` |  |
| `SelfTest` |  |
| `ClearError` |  |
| `PasswordUnlock` |  |
| `PasswordLock` |  |
| `ReadLongTimer` | Reads LTN/LSTN words and decodes to LongTimerResult[] |
| `ReadModuleBufWords` | Extended device read (module buffer), word subcommand (0x0080/0x0082) |
| `WriteModuleBufWords` | Extended device write (module buffer), word subcommand (0x0080/0x0082) |
| `ReadModuleBufBits` | Extended device read (module buffer), bit subcommand (0x0081/0x0083) |
| `WriteModuleBufBits` | Extended device write (module buffer), bit subcommand (0x0081/0x0083) |
| `ReadLinkDirectWords` | Link direct read, sub 0x0080 |
| `WriteWordsLinkDirect` | Link direct write words, sub 0x0080 |
| `ReadLinkDirectBits` | Link direct read bits, sub 0x0081 |
| `WriteBitsLinkDirect` | Link direct write bits, sub 0x0081 |
| `ReadMemoryWords` | Memory read (0x0613) |
| `WriteMemoryWords` | Memory write (0x1613) |
| `ReadExtendUnitBytes` | Extend unit read (0x0601) |
| `WriteExtendUnitBytes` | Extend unit write (0x1601) |
| `ReadArrayLabels` | Label array read (0x041A) |
| `WriteArrayLabels` | Label array write (0x141A) |
| `ReadRandomLabels` | Label random read (0x041C) |
| `WriteRandomLabels` | Label random write (0x141B) |
| `ReadRandomExt` | Extended random read (0x0403 sub 0x0080/0x0082) |
| `WriteRandomWordsExt` | Extended random write words (0x1402 sub 0x0080/0x0082) |
| `WriteRandomBitsExt` | Extended random write bits (0x1402 sub 0x0081/0x0083) |
| `RegisterMonitorDevices` | Register monitor devices (0x0801 sub 0x0000/0x0002) |
| `RegisterMonitorDevicesExt` | Register monitor devices ext (0x0801 sub 0x0080/0x0082) |
| `RunMonitorCycle` | Run monitor cycle (0x0802) |

#### `WriteBlockStage`

| Value | Description |
| --- | --- |
| `Direct` |  |
| `SplitWord` |  |
| `SplitBit` |  |

#### Fields

#### `type`

```cpp
Type slmp::SlmpClient::AsyncContext::type = Type::None
```

#### `common`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::common
```

#### `points`

```cpp
uint16_t slmp::SlmpClient::AsyncContext::points
```

#### `values`

```cpp
void* slmp::SlmpClient::AsyncContext::values
```

#### `out`

```cpp
TypeNameInfo* slmp::SlmpClient::AsyncContext::out
```

#### `readTypeName`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readTypeName
```

#### `dword_count`

```cpp
uint16_t slmp::SlmpClient::AsyncContext::dword_count
```

#### `dword_values`

```cpp
uint32_t* slmp::SlmpClient::AsyncContext::dword_values
```

#### `readRandom`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readRandom
```

#### `word_count`

```cpp
uint16_t slmp::SlmpClient::AsyncContext::word_count
```

#### `word_values`

```cpp
uint16_t* slmp::SlmpClient::AsyncContext::word_values
```

#### `bit_values`

```cpp
uint16_t* slmp::SlmpClient::AsyncContext::bit_values
```

#### `readBlock`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readBlock
```

#### `total_bit_points`

```cpp
size_t slmp::SlmpClient::AsyncContext::total_bit_points
```

#### `total_word_points`

```cpp
size_t slmp::SlmpClient::AsyncContext::total_word_points
```

#### `expect_response`

```cpp
bool slmp::SlmpClient::AsyncContext::expect_response
```

#### `remoteReset`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::remoteReset
```

#### `subcommand`

```cpp
uint16_t slmp::SlmpClient::AsyncContext::subcommand
```

#### `out`

```cpp
uint8_t* slmp::SlmpClient::AsyncContext::out
```

#### `out_capacity`

```cpp
size_t slmp::SlmpClient::AsyncContext::out_capacity
```

#### `out_length`

```cpp
size_t* slmp::SlmpClient::AsyncContext::out_length
```

#### `selfTest`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::selfTest
```

#### `word_blocks`

```cpp
const DeviceBlockWrite* slmp::SlmpClient::AsyncContext::word_blocks
```

#### `word_block_count`

```cpp
size_t slmp::SlmpClient::AsyncContext::word_block_count
```

#### `bit_blocks`

```cpp
const DeviceBlockWrite* slmp::SlmpClient::AsyncContext::bit_blocks
```

#### `bit_block_count`

```cpp
size_t slmp::SlmpClient::AsyncContext::bit_block_count
```

#### `options`

```cpp
BlockWriteOptions slmp::SlmpClient::AsyncContext::options
```

#### `stage`

```cpp
WriteBlockStage slmp::SlmpClient::AsyncContext::stage
```

#### `has_mixed_blocks`

```cpp
bool slmp::SlmpClient::AsyncContext::has_mixed_blocks
```

#### `writeBlock`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::writeBlock
```

#### `out`

```cpp
LongTimerResult* slmp::SlmpClient::AsyncContext::out
```

#### `readLongTimer`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readLongTimer
```

#### `capacity`

```cpp
size_t slmp::SlmpClient::AsyncContext::capacity
```

#### `count`

```cpp
size_t slmp::SlmpClient::AsyncContext::count
```

#### `out`

```cpp
LabelArrayReadResult* slmp::SlmpClient::AsyncContext::out
```

#### `out_count`

```cpp
size_t* slmp::SlmpClient::AsyncContext::out_count
```

#### `readArrayLabels`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readArrayLabels
```

#### `out`

```cpp
LabelRandomReadResult* slmp::SlmpClient::AsyncContext::out
```

#### `readRandomLabels`

```cpp
struct slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::readRandomLabels
```

#### `data`

```cpp
union slmp::SlmpClient::AsyncContext slmp::SlmpClient::AsyncContext::data
```

### Struct `slmp::ReconnectOptions`

Configuration for the ReconnectHelper.

#### Fields

#### `retry_interval_ms`

```cpp
uint32_t slmp::ReconnectOptions::retry_interval_ms = 3000
```

Minimum time between connection attempts. Prevents hammering the network or PLC when the connection is physically down

#### Member Functions

#### `ReconnectOptions`

```cpp
slmp::ReconnectOptions::ReconnectOptions()=default
```

#### `ReconnectOptions`

```cpp
slmp::ReconnectOptions::ReconnectOptions(uint32_t retry_interval_ms_value)
```
