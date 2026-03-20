/**
 * @file atom_matrix_serial_console.h
 * @brief Interactive SLMP debug console for M5Stack Atom Matrix.
 * 
 * Provides a feature-rich serial console for manual SLMP command testing,
 * automated functional checks, and endurance (stress) testing.
 * Uses the onboard 5x5 LED matrix for status visualization.
 */

#pragma once

#include <esp32-hal-rmt.h>
#include <esp_system.h>

/**
 * @defgroup SLMP_Examples Examples & Tools
 * @brief Demonstration applications and debug utilities.
 * @{
 */

/**
 * @namespace atom_matrix_serial_console
 * @brief Implementation of the Atom Matrix serial console example.
 */
namespace atom_matrix_serial_console {
/** @brief Handle commands specific to the Atom Matrix hardware. */
bool handleAtomCustomCommand(char* tokens[], int token_count);
/** @brief Show help for Atom Matrix specific commands. */
bool handleAtomHelpCommand(char* tokens[], int token_count);
/** @brief Print help for hardware-specific features (LED matrix, Button). */
void printAtomExtraHelp();
/** @brief Print full command reference to the serial port. */
void printFullCommandHelp();
/** @brief Initialize hardware (Matrix, Button) and console state. */
void setupConsole();
/** @brief Update the LED matrix color. */
void renderSolidMatrix(uint8_t red, uint8_t green, uint8_t blue);
/** @brief Print hint for manual verification steps. */
void printAtomPendingJudgeHint();
/** @brief Command: Run a sequence of protocol functional checks. */
void funcheckCommand(char* tokens[], int token_count);
/** @brief Command: Start/stop long-running stress test. */
void enduranceCommand(char* tokens[], int token_count);
/** @brief Command: Start/stop connection cycle test. */
void reconnectCommand(char* tokens[], int token_count);
/** @brief Command: Test frame size limits. */
void txlimitCommand(char* tokens[], int token_count);
/** @brief Command: Run performance benchmark. */
void benchCommand(char* tokens[], int token_count);
/** @brief Periodic worker for endurance test. */
void pollEnduranceTest();
/** @brief Periodic worker for reconnection test. */
void pollReconnectTest();
/** @brief Periodic worker for benchmark. */
void pollBenchmark();
/** @brief Stop active endurance test. */
void stopEndurance(bool print_summary, bool failed);
/** @brief Stop active reconnection test. */
void stopReconnect(bool print_summary);
/** @brief Stop active benchmark. */
void stopBenchmark(bool print_summary, bool failed);
/** @brief Generate next predictable value for functional check. */
uint16_t nextFuncheckWordValue();
/** @brief Generate next predictable value for functional check. */
uint32_t nextFuncheckDWordValue();
}

/** @} */ // end of SLMP_Examples

#define SLMP_WIFI_SERIAL_CONSOLE_CONFIG_HEADER "../atom_matrix_serial_console/config.h"
#define SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE atom_matrix_serial_console
#define SLMP_WIFI_SERIAL_CONSOLE_BANNER "SLMP Atom Matrix serial debug console"
#define SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS 0
#define SLMP_WIFI_SERIAL_CONSOLE_DISABLE_BUILTIN_BENCH 1
#define SLMP_WIFI_SERIAL_CONSOLE_EXTRA_HELP() printAtomExtraHelp()
#define SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_COMMAND_HANDLER(tokens, token_count) handleAtomCustomCommand(tokens, token_count)
#define SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_HELP_HANDLER(tokens, token_count) handleAtomHelpCommand(tokens, token_count)
#define SLMP_WIFI_SERIAL_CONSOLE_PRINT_PENDING_JUDGE_HINT() printAtomPendingJudgeHint()

#include "../shared/slmp_wifi_serial_console.h"

namespace atom_matrix_serial_console {

constexpr uint8_t kButtonPin = G39;
constexpr uint8_t kMatrixPin = 27;
constexpr size_t kMatrixWidth = 5;
constexpr size_t kMatrixHeight = 5;
constexpr size_t kMatrixLedCount = kMatrixWidth * kMatrixHeight;
constexpr size_t kMatrixBitCount = kMatrixLedCount * 24;
constexpr uint32_t kStartupSplashColorMs = 140;
constexpr uint32_t kStartupSplashOffMs = 120;
constexpr uint32_t kStartupPhaseFrameMs = 90;
constexpr uint32_t kStartupCompleteHoldMs = 220;
constexpr uint32_t kButtonDebounceMs = 60;
constexpr uint32_t kButtonLongPressMs = 1200;
constexpr uint32_t kDemoPollIntervalMs = 200;
constexpr uint32_t kEnduranceCycleGapMs = 20;
constexpr uint32_t kEnduranceReportIntervalMs = 5000;
constexpr uint32_t kEnduranceBarMaxMs = 500;
constexpr uint32_t kReconnectCycleGapMs = 250;
constexpr uint32_t kReconnectMaxCycleGapMs = 5000;
constexpr uint32_t kReconnectReportIntervalMs = 5000;
constexpr uint32_t kReconnectBarMaxMs = 500;
constexpr uint32_t kBenchReportIntervalMs = 3000;
constexpr uint32_t kBenchBarMaxMs = 200;
constexpr size_t kSlmpRequestHeaderSize = 19;
constexpr size_t kTxPayloadBudget = example_config::kTxBufferSize > kSlmpRequestHeaderSize
                                        ? (example_config::kTxBufferSize - kSlmpRequestHeaderSize)
                                        : 0;
constexpr slmp::DeviceAddress kButtonIncrementDevice = {slmp::DeviceCode::D, 0};
constexpr slmp::DeviceAddress kDemoBitDevice = {slmp::DeviceCode::M, 0};
constexpr slmp::DeviceAddress kFuncheckOneWordDevice = {slmp::DeviceCode::D, 120};
constexpr slmp::DeviceAddress kFuncheckWordArrayDevice = {slmp::DeviceCode::D, 130};
constexpr slmp::DeviceAddress kFuncheckOneBitDevice = {slmp::DeviceCode::M, 120};
constexpr slmp::DeviceAddress kFuncheckBitArrayDevice = {slmp::DeviceCode::M, 130};
constexpr slmp::DeviceAddress kFuncheckOneDWordDevice = {slmp::DeviceCode::D, 220};
constexpr slmp::DeviceAddress kFuncheckDWordArrayDevice = {slmp::DeviceCode::D, 230};
constexpr slmp::DeviceAddress kFuncheckRandomWordDevices[] = {
    {slmp::DeviceCode::D, 140},
    {slmp::DeviceCode::D, 141},
};
constexpr slmp::DeviceAddress kFuncheckRandomDWordDevices[] = {
    {slmp::DeviceCode::D, 240},
};
constexpr slmp::DeviceAddress kFuncheckRandomBitDevices[] = {
    {slmp::DeviceCode::M, 140},
    {slmp::DeviceCode::M, 141},
};
constexpr slmp::DeviceAddress kFuncheckBlockWordDevice = {slmp::DeviceCode::D, 300};
constexpr slmp::DeviceAddress kFuncheckBlockBitDevice = {slmp::DeviceCode::M, 200};
constexpr slmp::DeviceAddress kEnduranceOneWordDevice = {slmp::DeviceCode::D, 500};
constexpr slmp::DeviceAddress kEnduranceWordArrayDevice = {slmp::DeviceCode::D, 510};
constexpr slmp::DeviceAddress kEnduranceOneBitDevice = {slmp::DeviceCode::M, 500};
constexpr slmp::DeviceAddress kEnduranceBitArrayDevice = {slmp::DeviceCode::M, 510};
constexpr slmp::DeviceAddress kEnduranceOneDWordDevice = {slmp::DeviceCode::D, 600};
constexpr slmp::DeviceAddress kEnduranceDWordArrayDevice = {slmp::DeviceCode::D, 610};
constexpr slmp::DeviceAddress kEnduranceRandomWordDevices[] = {
    {slmp::DeviceCode::D, 520},
    {slmp::DeviceCode::D, 521},
};
constexpr slmp::DeviceAddress kEnduranceRandomDWordDevices[] = {
    {slmp::DeviceCode::D, 620},
};
constexpr slmp::DeviceAddress kEnduranceRandomBitDevices[] = {
    {slmp::DeviceCode::M, 520},
    {slmp::DeviceCode::M, 521},
};
constexpr slmp::DeviceAddress kEnduranceBlockWordDevice = {slmp::DeviceCode::D, 540};
constexpr slmp::DeviceAddress kEnduranceBlockBitDevice = {slmp::DeviceCode::M, 540};
constexpr slmp::DeviceAddress kTxLimitWordDevice = {slmp::DeviceCode::D, 700};
constexpr slmp::DeviceAddress kTxLimitBlockWordDevice = {slmp::DeviceCode::D, 1100};
constexpr slmp::DeviceAddress kBenchOneWordDevice = {slmp::DeviceCode::D, 800};
constexpr slmp::DeviceAddress kBenchWordArrayDevice = {slmp::DeviceCode::D, 820};
constexpr slmp::DeviceAddress kBenchBlockWordDevice = {slmp::DeviceCode::D, 900};
constexpr size_t kBenchWordPoints = 8;
constexpr size_t kBenchBlockPoints = 16;
constexpr size_t kTxLimitWriteWordsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 2U) : 0U;
constexpr size_t kTxLimitWriteDWordsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 4U) : 0U;
constexpr size_t kTxLimitWriteBitsMaxCount = kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) * 2U) : 0U;
constexpr size_t kTxLimitWriteRandomBitsMaxCount = kTxPayloadBudget >= 1U ? ((kTxPayloadBudget - 1U) / 8U) : 0U;
constexpr size_t kTxLimitWriteRandomWordsMaxCount = kTxPayloadBudget >= 2U ? ((kTxPayloadBudget - 2U) / 8U) : 0U;
constexpr size_t kTxLimitWriteBlockWordMaxPoints = kTxPayloadBudget >= 10U ? ((kTxPayloadBudget - 10U) / 2U) : 0U;
static_assert(kTxLimitWriteWordsMaxCount > 0, "txlimit writeWords max count must be positive");
static_assert(kTxLimitWriteBlockWordMaxPoints > 0, "txlimit block max points must be positive");

bool button_raw_pressed = false;
bool button_stable_pressed = false;
uint32_t button_last_change_ms = 0;
uint32_t button_press_started_ms = 0;
bool button_long_press_handled = false;
bool demo_mode_enabled = false;
uint32_t demo_last_poll_ms = 0;
struct ManualCheckSession {
    bool active = false;
    bool waiting_for_judge = false;
    size_t next_step_index = 0;
    char last_guide[kSerialLineCapacity] = {};
    char last_preview[kSerialLineCapacity] = {};
    char last_command[kSerialLineCapacity] = {};
};
constexpr size_t kManualCheckStepCount = sizeof(example_config::kManualCheckSteps) / sizeof(example_config::kManualCheckSteps[0]);
ManualCheckSession manual_check = {};
enum class FuncheckResult : uint8_t {
    Ok = 0,
    Fail,
    Skip,
};
struct FuncheckSummary {
    size_t ok = 0;
    size_t fail = 0;
    size_t skip = 0;
};
struct EnduranceSession {
    bool active = false;
    uint32_t started_ms = 0;
    uint32_t next_cycle_due_ms = 0;
    uint32_t last_report_ms = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    uint32_t cycle_limit = 0;
    size_t attempts = 0;
    size_t ok = 0;
    size_t fail = 0;
    char last_step[48] = {};
    char last_issue[64] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};
struct ReconnectSession {
    bool active = false;
    uint32_t started_ms = 0;
    uint32_t next_cycle_due_ms = 0;
    uint32_t last_report_ms = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    uint32_t cycle_limit = 0;
    size_t attempts = 0;
    size_t ok = 0;
    size_t fail = 0;
    size_t recoveries = 0;
    size_t consecutive_failures = 0;
    size_t max_consecutive_failures = 0;
    char last_step[48] = {};
    char last_issue[64] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};
enum class BenchMode : uint8_t {
    None = 0,
    Row,
    Wow,
    Pair,
    Rw,
    Ww,
    Block,
};
struct BenchmarkSession {
    bool active = false;
    BenchMode mode = BenchMode::None;
    uint32_t started_ms = 0;
    uint32_t next_cycle_due_ms = 0;
    uint32_t last_report_ms = 0;
    uint32_t last_cycle_ms = 0;
    uint32_t min_cycle_ms = 0;
    uint32_t max_cycle_ms = 0;
    uint64_t total_cycle_ms = 0;
    uint32_t cycle_limit = 0;
    size_t cycles = 0;
    size_t requests = 0;
    size_t fail = 0;
    size_t last_request_bytes = 0;
    size_t last_response_bytes = 0;
    char last_step[48] = {};
    char last_issue[64] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};
rmt_obj_t* matrix_rmt = nullptr;
rmt_data_t matrix_led_data[kMatrixBitCount] = {};
EnduranceSession endurance = {};
ReconnectSession reconnect = {};
BenchmarkSession bench = {};
uint16_t txlimit_word_values[kTxLimitWriteWordsMaxCount + 1U] = {};
uint16_t txlimit_word_readback[kTxLimitWriteWordsMaxCount + 1U] = {};
uint16_t txlimit_block_values[kTxLimitWriteBlockWordMaxPoints + 1U] = {};
uint16_t txlimit_block_readback[kTxLimitWriteBlockWordMaxPoints + 1U] = {};
uint16_t bench_word_values[kBenchWordPoints] = {};
uint16_t bench_word_readback[kBenchWordPoints] = {};
uint16_t bench_block_values[kBenchBlockPoints] = {};
uint16_t bench_block_readback[kBenchBlockPoints] = {};

void encodeMatrixByte(size_t bit_offset, uint8_t value) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const bool is_one = (value & (1U << (7U - bit))) != 0;
        matrix_led_data[bit_offset + bit].level0 = 1;
        matrix_led_data[bit_offset + bit].duration0 = is_one ? 8 : 4;
        matrix_led_data[bit_offset + bit].level1 = 0;
        matrix_led_data[bit_offset + bit].duration1 = is_one ? 4 : 8;
    }
}

size_t logicalToPhysicalLedIndex(size_t logical_index) {
    const size_t row = logical_index / kMatrixWidth;
    const size_t col = logical_index % kMatrixWidth;
    if ((row & 1U) == 0U) {
        return row * kMatrixWidth + col;
    }
    return row * kMatrixWidth + (kMatrixWidth - 1U - col);
}

void setMatrixPhysicalPixel(size_t physical_index, uint8_t red, uint8_t green, uint8_t blue) {
    if (physical_index >= kMatrixLedCount) {
        return;
    }
    const size_t bit_offset = physical_index * 24U;
    encodeMatrixByte(bit_offset + 0U, green);
    encodeMatrixByte(bit_offset + 8U, red);
    encodeMatrixByte(bit_offset + 16U, blue);
}

void setMatrixLogicalPixel(size_t logical_index, uint8_t red, uint8_t green, uint8_t blue) {
    setMatrixPhysicalPixel(logicalToPhysicalLedIndex(logical_index), red, green, blue);
}

void fillMatrix(uint8_t red, uint8_t green, uint8_t blue) {
    for (size_t i = 0; i < kMatrixLedCount; ++i) {
        setMatrixPhysicalPixel(i, red, green, blue);
    }
}

bool ensureMatrixReady() {
    if (matrix_rmt != nullptr) {
        return true;
    }
    matrix_rmt = rmtInit(kMatrixPin, RMT_TX_MODE, RMT_MEM_64);
    if (matrix_rmt == nullptr) {
        Serial.println("matrix init failed");
        return false;
    }
    (void)rmtSetTick(matrix_rmt, 100);
    fillMatrix(0, 0, 0);
    (void)rmtWrite(matrix_rmt, matrix_led_data, kMatrixBitCount);
    return true;
}

void showMatrix() {
    if (!ensureMatrixReady()) {
        return;
    }
    (void)rmtWrite(matrix_rmt, matrix_led_data, kMatrixBitCount);
}

void showStartupSplash() {
    renderSolidMatrix(20, 0, 0);
    delay(kStartupSplashColorMs);
    renderSolidMatrix(0, 18, 0);
    delay(kStartupSplashColorMs);
    renderSolidMatrix(0, 0, 20);
    delay(kStartupSplashColorMs);
    renderSolidMatrix(12, 12, 12);
    delay(kStartupSplashColorMs);
    renderSolidMatrix(0, 0, 0);
    delay(kStartupSplashOffMs);
}

void renderStartupProgress(size_t lit_count, uint8_t red, uint8_t green, uint8_t blue) {
    if (lit_count > kMatrixLedCount) {
        lit_count = kMatrixLedCount;
    }
    fillMatrix(0, 0, 0);
    for (size_t i = 0; i < lit_count; ++i) {
        setMatrixLogicalPixel(i, red, green, blue);
    }
    showMatrix();
}

void showStartupPhase(size_t lit_count, uint8_t red, uint8_t green, uint8_t blue) {
    renderStartupProgress(lit_count, red, green, blue);
    delay(kStartupPhaseFrameMs);
}

void renderDemoInactive() {
    fillMatrix(0, 0, 0);
    showMatrix();
}

void renderDemoWaitingForWiFi() {
    fillMatrix(0, 0, 0);
    setMatrixLogicalPixel(12, 0, 0, 16);
    showMatrix();
}

void renderDemoWaitingForPlc() {
    fillMatrix(0, 0, 0);
    setMatrixLogicalPixel(12, 16, 8, 0);
    showMatrix();
}

void renderDemoReadError() {
    fillMatrix(4, 0, 0);
    setMatrixLogicalPixel(12, 20, 0, 0);
    showMatrix();
}

void renderDemoBits(const bool* values) {
    fillMatrix(0, 0, 3);
    for (size_t i = 0; i < kMatrixLedCount; ++i) {
        if (values[i]) {
            setMatrixLogicalPixel(i, 0, 28, 0);
        }
    }
    showMatrix();
}

void renderSolidMatrix(uint8_t red, uint8_t green, uint8_t blue) {
    fillMatrix(red, green, blue);
    showMatrix();
}

bool isIncrementButtonPressed() {
    return digitalRead(kButtonPin) == LOW;
}

void printAtomPendingJudgeHint() {
    Serial.println("press Atom button for yes");
    Serial.println("type: ng [note] when the result is bad");
    Serial.println("type: ok [note] if you want to accept from the console");
}

void printAtomExtraHelp() {
    Serial.println("  demo | demo off");
    Serial.println("  check [start|next|status|list|stop]");
    Serial.println("  funcheck [all|direct|api|list]");
    Serial.println("  endurance [start [cycles]|status|stop|list]");
    Serial.println("  reconnect [start [cycles]|status|stop|list]");
    Serial.println("  txlimit [calc|probe]");
    Serial.println("  bench [row|wow|pair|rw|ww|block] [cycles]");
    Serial.println("  ng [note]");
    Serial.println("  skip [note]");
    Serial.println("  led <off|red|green|blue|white>");
    Serial.println("  ledtest");
    Serial.println("  help all");
}

void printFullCommandHelp() {
    Serial.println("commands:");
    Serial.println("  help");
    Serial.println("  status");
    Serial.println("  connect | close | reinit | type | dump");
    Serial.println("  target [network station module_io multidrop]");
    Serial.println("  monitor [value]");
    Serial.println("  timeout <ms>");
    Serial.println("  rw <device> [points]");
    Serial.println("  ww <device> <value...>");
    Serial.println("  row <device>");
    Serial.println("  wow <device> <value>");
    Serial.println("  rb <device>");
    Serial.println("  wb <device> <0|1>");
    Serial.println("  rbits <device> <points>");
    Serial.println("  wbits <device> <0|1...>");
    Serial.println("  rdw <device> [points]");
    Serial.println("  wdw <device> <value...>");
    Serial.println("  rod <device>");
    Serial.println("  wod <device> <value>");
    Serial.println("  rr <word_count> <word_devices...> <dword_count> <dword_devices...>");
    Serial.println("  wrand <word_count> <device value...> <dword_count> <device value...>");
    Serial.println("  wrandb <count> <device value...>");
    Serial.println("  rblk <word_blocks> <device points...> <bit_blocks> <device points...>");
    Serial.println("  wblk <word_blocks> <device points values...> <bit_blocks> <device points packed...>");
    Serial.println("  unlock <password>");
    Serial.println("  lock <password>");
    Serial.println("  verifyw <device> <value...>");
    Serial.println("  verifyb <device> <0|1>");
    Serial.println("  pending | judge <ok|ng> [note]");
    printAtomExtraHelp();
    Serial.println("examples:");
    Serial.println("  demo");
    Serial.println("  check");
    Serial.println("  check next");
    Serial.println("  funcheck");
    Serial.println("  funcheck api");
    Serial.println("  endurance");
    Serial.println("  endurance 1000");
    Serial.println("  reconnect");
    Serial.println("  reconnect 1000");
    Serial.println("  txlimit");
    Serial.println("  txlimit probe");
    Serial.println("  bench");
    Serial.println("  bench row 1000");
    Serial.println("  ng lamp did not change");
    Serial.println("  skip device not supported");
    Serial.println("  target 0 255 0x03FF 0");
    Serial.println("  row D100");
    Serial.println("  rdw D200 2");
    Serial.println("  rbits M100 4");
    Serial.println("  rr 2 D100 D101 1 D200");
    Serial.println("  wrand 1 D120 123 1 D200 0x12345678");
    Serial.println("  rblk 1 D300 2 1 M200 1");
    Serial.println("  wblk 1 D300 2 10 20 1 M200 1 0x0005");
    Serial.println("hex-address devices: X, Y, B, W, SB, SW, DX, DY");
    Serial.println("bit block points use packed 16-bit words");
}

void printAtomCompactHelp() {
    Serial.println("commands:");
    Serial.println("  demo");
    Serial.println("  check");
    Serial.println("  funcheck");
    Serial.println("  endurance");
    Serial.println("  reconnect");
    Serial.println("  txlimit");
    Serial.println("  bench");
    Serial.println("  ng [note]");
    Serial.println("  skip [note]");
    Serial.println("  ledtest");
    Serial.println("  led <color>");
    Serial.println("  status");
    Serial.println("  connect");
    Serial.println("  row <device>");
    Serial.println("  wow <device> <value>");
    Serial.println("  rb <device>");
    Serial.println("  wb <device> <0|1>");
    Serial.println("  dump");
    Serial.println("  help all");
}

bool handleAtomHelpCommand(char* tokens[], int token_count) {
    if (token_count > 1) {
        uppercaseInPlace(tokens[1]);
        if (strcmp(tokens[1], "ALL") == 0 || strcmp(tokens[1], "FULL") == 0) {
            printFullCommandHelp();
            return true;
        }
    }
    printAtomCompactHelp();
    return true;
}

void stopDemoForManualLedControl() {
    if (!demo_mode_enabled) {
        return;
    }
    demo_mode_enabled = false;
    demo_last_poll_ms = 0;
    Serial.println("demo_mode=off");
}

bool manualCheckConfigLooksUnset() {
    return kManualCheckStepCount == 0 ||
           example_config::kManualCheckSteps[0].command == nullptr ||
           example_config::kManualCheckSteps[0].command[0] == '\0';
}

bool manualCheckStepIsEmpty(const example_config::ManualCheckStep& step) {
    return step.command == nullptr || step.command[0] == '\0';
}

void resetManualCheckSession() {
    manual_check = ManualCheckSession();
}

const char* funcheckResultText(FuncheckResult result) {
    switch (result) {
        case FuncheckResult::Ok:
            return "ok";
        case FuncheckResult::Fail:
            return "fail";
        case FuncheckResult::Skip:
            return "skip";
        default:
            return "unknown";
    }
}

void recordFuncheckResult(FuncheckSummary& summary, FuncheckResult result) {
    switch (result) {
        case FuncheckResult::Ok:
            ++summary.ok;
            return;
        case FuncheckResult::Fail:
            ++summary.fail;
            return;
        case FuncheckResult::Skip:
            ++summary.skip;
            return;
        default:
            return;
    }
}

bool dwordArraysEqual(const uint32_t* lhs, const uint32_t* rhs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

bool bitArraysEqual(const bool* lhs, const bool* rhs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

uint16_t readFrameLe16(const uint8_t* bytes) {
    if (bytes == nullptr) {
        return 0;
    }
    return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8U);
}

void copyText(char* out, size_t out_capacity, const char* text) {
    if (out == nullptr || out_capacity == 0) {
        return;
    }
    size_t i = 0;
    if (text != nullptr) {
        for (; i + 1 < out_capacity && text[i] != '\0'; ++i) {
            out[i] = text[i];
        }
    }
    out[i] = '\0';
}

void clearWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    const uint16_t zeros[kMaxWordPoints] = {};
    (void)plc.writeWords(device, zeros, count);
}

void clearWordRangeSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || !connectPlc(false)) {
        return;
    }
    const uint16_t zeros[kMaxWordPoints] = {};
    size_t offset = 0;
    while (offset < count) {
        const size_t chunk = (count - offset) > kMaxWordPoints ? kMaxWordPoints : (count - offset);
        const slmp::DeviceAddress chunk_device = {device.code, device.number + static_cast<uint32_t>(offset)};
        (void)plc.writeWords(chunk_device, zeros, chunk);
        offset += chunk;
    }
}

void clearBitsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    if (count == 1) {
        (void)plc.writeOneBit(device, false);
        return;
    }
    bool zeros[kMaxWordPoints] = {};
    (void)plc.writeBits(device, zeros, count);
}

void clearDWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || count > kMaxWordPoints || !connectPlc(false)) {
        return;
    }
    if (count == 1) {
        (void)plc.writeOneDWord(device, 0U);
        return;
    }
    uint32_t zeros[kMaxWordPoints] = {};
    (void)plc.writeDWords(device, zeros, count);
}

void clearRandomWordsSilently(
    const slmp::DeviceAddress* word_devices,
    size_t word_count,
    const slmp::DeviceAddress* dword_devices,
    size_t dword_count
) {
    if (!connectPlc(false)) {
        return;
    }
    const uint16_t zero_words[kMaxRandomWordDevices] = {};
    const uint32_t zero_dwords[kMaxRandomDWordDevices] = {};
    (void)plc.writeRandomWords(word_devices, zero_words, word_count, dword_devices, zero_dwords, dword_count);
}

void clearRandomBitsSilently(const slmp::DeviceAddress* bit_devices, size_t bit_count) {
    if (!connectPlc(false)) {
        return;
    }
    const bool zero_bits[kMaxRandomBitDevices] = {};
    (void)plc.writeRandomBits(bit_devices, zero_bits, bit_count);
}

void clearPackedBitWordsSilently(const slmp::DeviceAddress& device, uint16_t packed_word_count) {
    if (!connectPlc(false)) {
        return;
    }
    uint16_t zero_words[kMaxBlockPoints] = {};
    if (packed_word_count <= kMaxBlockPoints) {
        const slmp::DeviceBlockWrite bit_block = {device, zero_words, packed_word_count};
        if (plc.writeBlock(nullptr, 0, &bit_block, 1) == slmp::Error::Ok) {
            return;
        }
    }
    const uint32_t bit_count = static_cast<uint32_t>(packed_word_count) * 16U;
    for (uint32_t offset = 0; offset < bit_count; ++offset) {
        const slmp::DeviceAddress bit_device = {device.code, device.number + offset};
        (void)plc.writeOneBit(bit_device, false);
    }
}

void clearBlockSilently(
    const slmp::DeviceAddress& word_device,
    uint16_t word_points,
    const slmp::DeviceAddress& bit_device,
    uint16_t bit_points
) {
    if (word_points > 0) {
        clearWordRangeSilently(word_device, word_points);
    }
    if (bit_points > 0) {
        clearPackedBitWordsSilently(bit_device, bit_points);
    }
}

void clearEnduranceTargetsSilently() {
    clearWordsSilently(kEnduranceOneWordDevice, 1);
    clearWordsSilently(kEnduranceWordArrayDevice, 2);
    clearBitsSilently(kEnduranceOneBitDevice, 1);
    clearBitsSilently(kEnduranceBitArrayDevice, 4);
    clearDWordsSilently(kEnduranceOneDWordDevice, 1);
    clearDWordsSilently(kEnduranceDWordArrayDevice, 2);
    clearRandomWordsSilently(
        kEnduranceRandomWordDevices,
        sizeof(kEnduranceRandomWordDevices) / sizeof(kEnduranceRandomWordDevices[0]),
        kEnduranceRandomDWordDevices,
        sizeof(kEnduranceRandomDWordDevices) / sizeof(kEnduranceRandomDWordDevices[0])
    );
    clearRandomBitsSilently(
        kEnduranceRandomBitDevices,
        sizeof(kEnduranceRandomBitDevices) / sizeof(kEnduranceRandomBitDevices[0])
    );
    clearBlockSilently(kEnduranceBlockWordDevice, 2, kEnduranceBlockBitDevice, 1);
}

void printFuncheckApiDevices(const char* label, const slmp::DeviceAddress* devices, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        printDeviceAddress(devices[i]);
    }
    Serial.println();
}

void printManualCheckList() {
    if (manualCheckConfigLooksUnset()) {
        Serial.println("check config empty; edit config.h");
        return;
    }
    Serial.print("check_steps=");
    Serial.println(kManualCheckStepCount);
    for (size_t i = 0; i < kManualCheckStepCount; ++i) {
        const example_config::ManualCheckStep& step = example_config::kManualCheckSteps[i];
        if (manualCheckStepIsEmpty(step)) {
            continue;
        }
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(": ");
        if (step.guide != nullptr && step.guide[0] != '\0') {
            Serial.println(step.guide);
            if (step.preview != nullptr && step.preview[0] != '\0') {
                Serial.print("     preview: ");
                Serial.println(step.preview);
            }
            Serial.print("     command: ");
        }
        Serial.println(step.command);
    }
}

void printManualCheckStatus() {
    Serial.print("check_config_steps=");
    Serial.println(kManualCheckStepCount);
    Serial.print("check_active=");
    Serial.println(manual_check.active ? "yes" : "no");
    Serial.print("check_waiting_for_judge=");
    Serial.println(manual_check.waiting_for_judge ? "yes" : "no");
    Serial.print("check_next_step=");
    if (manual_check.next_step_index >= kManualCheckStepCount) {
        Serial.println("complete");
    } else {
        Serial.print(manual_check.next_step_index + 1);
        Serial.print("/");
        Serial.println(kManualCheckStepCount);
    }
    if (manual_check.last_command[0] != '\0') {
        Serial.print("check_last_command=");
        Serial.println(manual_check.last_command);
    }
    if (manual_check.last_guide[0] != '\0') {
        Serial.print("check_last_guide=");
        Serial.println(manual_check.last_guide);
    }
    if (manual_check.last_preview[0] != '\0') {
        Serial.print("check_last_preview=");
        Serial.println(manual_check.last_preview);
    }
    if (manual_check.waiting_for_judge || verification.active) {
        printVerificationSummary();
    }
}

void printFuncheckList() {
    Serial.println("funcheck modes:");
    Serial.println("  funcheck");
    Serial.println("    runs direct + api suites");
    Serial.println("  funcheck direct");
    Serial.println("    auto-runs check devices with readback judgement and 0 clear");
    Serial.println("  funcheck api");
    Serial.println("    auto-runs representative SlmpClient APIs and clears writes to 0");
    Serial.println("funcheck api devices:");
    Serial.print("  row/wow: ");
    printDeviceAddress(kFuncheckOneWordDevice);
    Serial.println();
    Serial.print("  rw/ww: ");
    printDeviceAddress(kFuncheckWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckWordArrayDevice, 1);
    Serial.println();
    Serial.print("  rb/wb: ");
    printDeviceAddress(kFuncheckOneBitDevice);
    Serial.println();
    Serial.print("  rbits/wbits: ");
    printDeviceAddress(kFuncheckBitArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckBitArrayDevice, 3);
    Serial.println();
    Serial.print("  rod/wod: ");
    printDeviceAddress(kFuncheckOneDWordDevice);
    Serial.println();
    Serial.print("  rdw/wdw: ");
    printDeviceAddress(kFuncheckDWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckDWordArrayDevice, 3);
    Serial.println();
    printFuncheckApiDevices("  wrand/rr words: ", kFuncheckRandomWordDevices, sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]));
    printFuncheckApiDevices("  wrand/rr dwords: ", kFuncheckRandomDWordDevices, sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0]));
    printFuncheckApiDevices("  wrandb bits: ", kFuncheckRandomBitDevices, sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0]));
    Serial.print("  rblk/wblk words: ");
    printDeviceAddress(kFuncheckBlockWordDevice);
    Serial.print("..");
    printDeviceAddress(kFuncheckBlockWordDevice, 1);
    Serial.println();
    Serial.print("  rblk/wblk bits: ");
    printDeviceAddress(kFuncheckBlockBitDevice);
    Serial.println(" packed 1 word");
}

void renderEnduranceFailure() {
    fillMatrix(5, 0, 0);
    setMatrixLogicalPixel(12, 20, 0, 0);
    showMatrix();
}

void renderEnduranceStatusBar() {
    if (!endurance.active) {
        return;
    }
    if (endurance.fail > 0) {
        renderEnduranceFailure();
        return;
    }

    fillMatrix(0, 0, 0);
    const uint32_t bounded_ms = endurance.last_cycle_ms > kEnduranceBarMaxMs ? kEnduranceBarMaxMs : endurance.last_cycle_ms;
    size_t lit_count = 1;
    if (kEnduranceBarMaxMs > 0) {
        lit_count = 1U + static_cast<size_t>((static_cast<uint64_t>(bounded_ms) * (kMatrixLedCount - 1U)) / kEnduranceBarMaxMs);
    }
    if (lit_count > kMatrixLedCount) {
        lit_count = kMatrixLedCount;
    }

    uint8_t red = 0;
    uint8_t green = 18;
    uint8_t blue = 0;
    if (endurance.last_cycle_ms >= 250) {
        red = 18;
        green = 10;
    } else if (endurance.last_cycle_ms >= 100) {
        red = 16;
        green = 16;
    }
    for (size_t i = 0; i < lit_count; ++i) {
        setMatrixLogicalPixel(i, red, green, blue);
    }
    setMatrixLogicalPixel((endurance.attempts % kMatrixLedCount), 0, 0, 18);
    showMatrix();
}

void printEnduranceList() {
    Serial.println("endurance modes:");
    Serial.println("  endurance");
    Serial.println("    starts continuous read/write durability test");
    Serial.println("  endurance 1000");
    Serial.println("    runs 1000 cycles, then stops");
    Serial.println("  endurance status");
    Serial.println("    prints counters and response times");
    Serial.println("  endurance stop");
    Serial.println("    stops the running durability test");
    Serial.println("    long-press the Atom button while endurance is running");
    Serial.println("endurance devices:");
    Serial.print("  row/wow: ");
    printDeviceAddress(kEnduranceOneWordDevice);
    Serial.println();
    Serial.print("  rw/ww: ");
    printDeviceAddress(kEnduranceWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kEnduranceWordArrayDevice, 1);
    Serial.println();
    Serial.print("  rb/wb: ");
    printDeviceAddress(kEnduranceOneBitDevice);
    Serial.println();
    Serial.print("  rbits/wbits: ");
    printDeviceAddress(kEnduranceBitArrayDevice);
    Serial.print("..");
    printDeviceAddress(kEnduranceBitArrayDevice, 3);
    Serial.println();
    Serial.print("  rod/wod: ");
    printDeviceAddress(kEnduranceOneDWordDevice);
    Serial.println();
    Serial.print("  rdw/wdw: ");
    printDeviceAddress(kEnduranceDWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kEnduranceDWordArrayDevice, 3);
    Serial.println();
    printFuncheckApiDevices("  wrand/rr words: ", kEnduranceRandomWordDevices, sizeof(kEnduranceRandomWordDevices) / sizeof(kEnduranceRandomWordDevices[0]));
    printFuncheckApiDevices("  wrand/rr dwords: ", kEnduranceRandomDWordDevices, sizeof(kEnduranceRandomDWordDevices) / sizeof(kEnduranceRandomDWordDevices[0]));
    printFuncheckApiDevices("  wrandb bits: ", kEnduranceRandomBitDevices, sizeof(kEnduranceRandomBitDevices) / sizeof(kEnduranceRandomBitDevices[0]));
    Serial.print("  rblk/wblk words: ");
    printDeviceAddress(kEnduranceBlockWordDevice);
    Serial.print("..");
    printDeviceAddress(kEnduranceBlockWordDevice, 1);
    Serial.println();
    Serial.print("  rblk/wblk bits: ");
    printDeviceAddress(kEnduranceBlockBitDevice);
    Serial.println(" packed 1 word");
}

void printEnduranceSummary(const char* label) {
    Serial.print(label);
    Serial.print(" attempts=");
    Serial.print(endurance.attempts);
    Serial.print(" ok=");
    Serial.print(endurance.ok);
    Serial.print(" fail=");
    Serial.print(endurance.fail);
    Serial.print(" elapsed_ms=");
    Serial.print(endurance.started_ms == 0 ? 0U : (millis() - endurance.started_ms));
    Serial.print(" last_cycle_ms=");
    Serial.print(endurance.last_cycle_ms);
    Serial.print(" avg_ms=");
    if (endurance.attempts == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(endurance.total_cycle_ms / endurance.attempts));
    }
    Serial.print(" min_ms=");
    Serial.print(endurance.attempts == 0 ? 0U : endurance.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.println(endurance.max_cycle_ms);
}

void printEnduranceStatus() {
    Serial.print("endurance_active=");
    Serial.println(endurance.active ? "yes" : "no");
    Serial.print("endurance_cycle_limit=");
    Serial.println(endurance.cycle_limit);
    Serial.print("endurance_last_step=");
    Serial.println(endurance.last_step[0] == '\0' ? "<none>" : endurance.last_step);
    Serial.print("endurance_last_issue=");
    Serial.println(endurance.last_issue[0] == '\0' ? "<none>" : endurance.last_issue);
    Serial.print("endurance_last_error=");
    Serial.println(slmp::errorString(endurance.last_error));
    Serial.print("endurance_last_end_code=0x");
    Serial.println(endurance.last_end_code, HEX);
    printEnduranceSummary("endurance status:");
}

void renderReconnectFailure() {
    fillMatrix(5, 1, 0);
    setMatrixLogicalPixel(12, 20, 0, 0);
    showMatrix();
}

void renderReconnectStatusBar() {
    if (!reconnect.active) {
        return;
    }
    fillMatrix(0, 0, 0);
    if (reconnect.consecutive_failures > 0) {
        const size_t lit_count = reconnect.consecutive_failures > kMatrixLedCount
                                     ? kMatrixLedCount
                                     : reconnect.consecutive_failures;
        for (size_t i = 0; i < lit_count; ++i) {
            setMatrixLogicalPixel(i, 20, 8, 0);
        }
        setMatrixLogicalPixel(12, 20, 0, 0);
        showMatrix();
        return;
    }

    const uint32_t bounded_ms = reconnect.last_cycle_ms > kReconnectBarMaxMs ? kReconnectBarMaxMs : reconnect.last_cycle_ms;
    size_t lit_count = 1;
    if (kReconnectBarMaxMs > 0) {
        lit_count = 1U + static_cast<size_t>((static_cast<uint64_t>(bounded_ms) * (kMatrixLedCount - 1U)) / kReconnectBarMaxMs);
    }
    if (lit_count > kMatrixLedCount) {
        lit_count = kMatrixLedCount;
    }

    uint8_t red = 0;
    uint8_t green = 16;
    uint8_t blue = 10;
    if (reconnect.last_cycle_ms >= 250) {
        red = 12;
        green = 12;
        blue = 8;
    } else if (reconnect.last_cycle_ms >= 100) {
        red = 8;
        green = 16;
        blue = 10;
    }
    for (size_t i = 0; i < lit_count; ++i) {
        setMatrixLogicalPixel(i, red, green, blue);
    }
    setMatrixLogicalPixel((reconnect.attempts % kMatrixLedCount), 0, 0, 18);
    showMatrix();
}

void printReconnectList() {
    Serial.println("reconnect modes:");
    Serial.println("  reconnect");
    Serial.println("    starts endless reconnect verification");
    Serial.println("  reconnect 1000");
    Serial.println("    runs 1000 attempts, then stops");
    Serial.println("  reconnect status");
    Serial.println("    prints counters, retries, and recovery counts");
    Serial.println("  reconnect stop");
    Serial.println("    stops the running reconnect test");
    Serial.println("    long-press the Atom button while reconnect is running");
    Serial.println("reconnect behavior:");
    Serial.println("  communication failures do not stop the mode");
    Serial.println("  each cycle retries Wi-Fi / PLC connect, then reads one probe word");
    Serial.print("  retry gap: ");
    Serial.print(kReconnectCycleGapMs);
    Serial.print("..");
    Serial.print(kReconnectMaxCycleGapMs);
    Serial.println(" ms with backoff after repeated failures");
    Serial.print("  probe device: ");
    printDeviceAddress(kEnduranceOneWordDevice);
    Serial.println();
}

uint32_t reconnectRetryGapMs() {
    uint32_t retry_gap_ms = kReconnectCycleGapMs;
    size_t remaining_failures = reconnect.consecutive_failures;
    while (remaining_failures > 1 && retry_gap_ms < kReconnectMaxCycleGapMs) {
        const uint32_t doubled_gap_ms = retry_gap_ms * 2U;
        retry_gap_ms = doubled_gap_ms > kReconnectMaxCycleGapMs ? kReconnectMaxCycleGapMs : doubled_gap_ms;
        --remaining_failures;
    }
    return retry_gap_ms;
}

void printReconnectSummary(const char* label) {
    Serial.print(label);
    Serial.print(" attempts=");
    Serial.print(reconnect.attempts);
    Serial.print(" ok=");
    Serial.print(reconnect.ok);
    Serial.print(" fail=");
    Serial.print(reconnect.fail);
    Serial.print(" recoveries=");
    Serial.print(reconnect.recoveries);
    Serial.print(" consecutive_failures=");
    Serial.print(reconnect.consecutive_failures);
    Serial.print(" max_consecutive_failures=");
    Serial.print(reconnect.max_consecutive_failures);
    Serial.print(" elapsed_ms=");
    Serial.print(reconnect.started_ms == 0 ? 0U : (millis() - reconnect.started_ms));
    Serial.print(" last_cycle_ms=");
    Serial.print(reconnect.last_cycle_ms);
    Serial.print(" avg_ms=");
    if (reconnect.attempts == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(reconnect.total_cycle_ms / reconnect.attempts));
    }
    Serial.print(" min_ms=");
    Serial.print(reconnect.attempts == 0 ? 0U : reconnect.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.print(reconnect.max_cycle_ms);
    Serial.print(" retry_gap_ms=");
    Serial.println(reconnectRetryGapMs());
}

void printReconnectStatus() {
    Serial.print("reconnect_active=");
    Serial.println(reconnect.active ? "yes" : "no");
    Serial.print("reconnect_cycle_limit=");
    Serial.println(reconnect.cycle_limit);
    Serial.print("reconnect_last_step=");
    Serial.println(reconnect.last_step[0] == '\0' ? "<none>" : reconnect.last_step);
    Serial.print("reconnect_last_issue=");
    Serial.println(reconnect.last_issue[0] == '\0' ? "<none>" : reconnect.last_issue);
    Serial.print("reconnect_last_error=");
    Serial.println(slmp::errorString(reconnect.last_error));
    Serial.print("reconnect_last_end_code=0x");
    Serial.println(reconnect.last_end_code, HEX);
    printReconnectSummary("reconnect status:");
}

void fillTxlimitWords(uint16_t* values, size_t count, uint16_t seed) {
    for (size_t i = 0; i < count; ++i) {
        values[i] = static_cast<uint16_t>(seed + i);
    }
}

void printTxlimitSummary() {
    Serial.print("tx_buffer_size=");
    Serial.println(example_config::kTxBufferSize);
    Serial.print("request_header_size=");
    Serial.println(kSlmpRequestHeaderSize);
    Serial.print("max_payload_fit=");
    Serial.println(kTxPayloadBudget);
    Serial.print("writeWords max_count=");
    Serial.print(kTxLimitWriteWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + (kTxLimitWriteWordsMaxCount * 2U));
    Serial.print("writeDWords max_count=");
    Serial.print(kTxLimitWriteDWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + (kTxLimitWriteDWordsMaxCount * 4U));
    Serial.print("writeBits max_points=");
    Serial.print(kTxLimitWriteBitsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 8U + ((kTxLimitWriteBitsMaxCount + 1U) / 2U));
    Serial.print("writeRandomWords max_word_devices=");
    Serial.print(kTxLimitWriteRandomWordsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 2U + (kTxLimitWriteRandomWordsMaxCount * 8U));
    Serial.print("writeRandomBits max_devices=");
    Serial.print(kTxLimitWriteRandomBitsMaxCount);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 1U + (kTxLimitWriteRandomBitsMaxCount * 8U));
    Serial.print("writeBlock words-only max_points=");
    Serial.print(kTxLimitWriteBlockWordMaxPoints);
    Serial.print(" frame_bytes=");
    Serial.println(kSlmpRequestHeaderSize + 10U + (kTxLimitWriteBlockWordMaxPoints * 2U));
    Serial.println("note: library uses the same tx_buffer for payload and full request frame");
}

bool runTxlimitProbeWriteWords() {
    fillTxlimitWords(txlimit_word_values, kTxLimitWriteWordsMaxCount, 1000U);
    if (plc.writeWords(kTxLimitWordDevice, txlimit_word_values, kTxLimitWriteWordsMaxCount) != slmp::Error::Ok) {
        printLastPlcError("txlimit writeWords exact");
        return false;
    }
    if (plc.readWords(
            kTxLimitWordDevice,
            static_cast<uint16_t>(kTxLimitWriteWordsMaxCount),
            txlimit_word_readback,
            kTxLimitWriteWordsMaxCount) != slmp::Error::Ok) {
        printLastPlcError("txlimit readWords exact");
        return false;
    }
    if (!wordArraysEqual(txlimit_word_values, txlimit_word_readback, static_cast<uint16_t>(kTxLimitWriteWordsMaxCount))) {
        Serial.println("txlimit writeWords exact failed: readback mismatch");
        return false;
    }
    Serial.print("txlimit writeWords exact_fit=ok count=");
    Serial.print(kTxLimitWriteWordsMaxCount);
    Serial.print(" request_bytes=");
    Serial.println(plc.lastRequestFrameLength());
    clearWordRangeSilently(kTxLimitWordDevice, kTxLimitWriteWordsMaxCount);

    fillTxlimitWords(txlimit_word_values, kTxLimitWriteWordsMaxCount + 1U, 2000U);
    const slmp::Error error = plc.writeWords(kTxLimitWordDevice, txlimit_word_values, kTxLimitWriteWordsMaxCount + 1U);
    Serial.print("txlimit writeWords one_over=");
    Serial.println(slmp::errorString(error));
    return error == slmp::Error::BufferTooSmall;
}

bool runTxlimitProbeWordBlock() {
    fillTxlimitWords(txlimit_block_values, kTxLimitWriteBlockWordMaxPoints, 3000U);
    const slmp::DeviceBlockWrite exact_block = {kTxLimitBlockWordDevice, txlimit_block_values, static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints)};
    const slmp::DeviceBlockRead exact_read_block = {kTxLimitBlockWordDevice, static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints)};
    if (plc.writeBlock(&exact_block, 1, nullptr, 0) != slmp::Error::Ok) {
        printLastPlcError("txlimit writeBlock exact");
        return false;
    }
    if (plc.readBlock(
            &exact_read_block,
            1,
            nullptr,
            0,
            txlimit_block_readback,
            kTxLimitWriteBlockWordMaxPoints,
            nullptr,
            0) != slmp::Error::Ok) {
        printLastPlcError("txlimit readBlock exact");
        return false;
    }
    if (!wordArraysEqual(txlimit_block_values, txlimit_block_readback, static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints))) {
        Serial.println("txlimit writeBlock exact failed: readback mismatch");
        return false;
    }
    Serial.print("txlimit writeBlock words exact_fit=ok points=");
    Serial.print(kTxLimitWriteBlockWordMaxPoints);
    Serial.print(" request_bytes=");
    Serial.println(plc.lastRequestFrameLength());
    clearWordRangeSilently(kTxLimitBlockWordDevice, kTxLimitWriteBlockWordMaxPoints);

    fillTxlimitWords(txlimit_block_values, kTxLimitWriteBlockWordMaxPoints + 1U, 4000U);
    const slmp::DeviceBlockWrite over_block = {
        kTxLimitBlockWordDevice,
        txlimit_block_values,
        static_cast<uint16_t>(kTxLimitWriteBlockWordMaxPoints + 1U)
    };
    const slmp::Error error = plc.writeBlock(&over_block, 1, nullptr, 0);
    Serial.print("txlimit writeBlock words one_over=");
    Serial.println(slmp::errorString(error));
    return error == slmp::Error::BufferTooSmall;
}

void runTxlimitProbe() {
    stopDemoForManualLedControl();
    stopEndurance(false, false);
    if (!connectPlc(false)) {
        Serial.println("txlimit probe failed: plc not connected");
        return;
    }
    printTxlimitSummary();
    const bool write_words_ok = runTxlimitProbeWriteWords();
    const bool block_ok = runTxlimitProbeWordBlock();
    Serial.print("txlimit probe summary: writeWords=");
    Serial.print(write_words_ok ? "ok" : "fail");
    Serial.print(" writeBlockWords=");
    Serial.println(block_ok ? "ok" : "fail");
}

void txlimitCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        printTxlimitSummary();
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "CALC") == 0 || strcmp(tokens[1], "INFO") == 0 || strcmp(tokens[1], "STATUS") == 0) {
        printTxlimitSummary();
        return;
    }
    if (strcmp(tokens[1], "PROBE") == 0 || strcmp(tokens[1], "TEST") == 0) {
        runTxlimitProbe();
        return;
    }

    Serial.println("txlimit usage: txlimit [calc|probe]");
}

const char* benchModeText(BenchMode mode) {
    switch (mode) {
        case BenchMode::Row:
            return "row";
        case BenchMode::Wow:
            return "wow";
        case BenchMode::Pair:
            return "pair";
        case BenchMode::Rw:
            return "rw";
        case BenchMode::Ww:
            return "ww";
        case BenchMode::Block:
            return "block";
        default:
            return "none";
    }
}

void resetBenchmarkSession() {
    bench = BenchmarkSession();
}

void clearBenchTargetsSilently() {
    clearWordRangeSilently(kBenchOneWordDevice, 1);
    clearWordRangeSilently(kBenchWordArrayDevice, kBenchWordPoints);
    clearWordRangeSilently(kBenchBlockWordDevice, kBenchBlockPoints);
}

void renderBenchmarkFailure() {
    fillMatrix(5, 0, 0);
    setMatrixLogicalPixel(0, 18, 0, 0);
    setMatrixLogicalPixel(12, 18, 0, 0);
    setMatrixLogicalPixel(24, 18, 0, 0);
    showMatrix();
}

void renderBenchmarkStatusBar() {
    if (!bench.active) {
        return;
    }
    if (bench.fail > 0) {
        renderBenchmarkFailure();
        return;
    }
    fillMatrix(0, 0, 0);
    const uint32_t bounded_ms = bench.last_cycle_ms > kBenchBarMaxMs ? kBenchBarMaxMs : bench.last_cycle_ms;
    size_t lit_count = 1;
    if (kBenchBarMaxMs > 0) {
        lit_count = 1U + static_cast<size_t>((static_cast<uint64_t>(bounded_ms) * (kMatrixLedCount - 1U)) / kBenchBarMaxMs);
    }
    if (lit_count > kMatrixLedCount) {
        lit_count = kMatrixLedCount;
    }
    uint8_t red = 0;
    uint8_t green = 10;
    uint8_t blue = 18;
    if (bench.last_cycle_ms >= 100) {
        red = 12;
        green = 10;
        blue = 12;
    }
    for (size_t i = 0; i < lit_count; ++i) {
        setMatrixLogicalPixel(i, red, green, blue);
    }
    setMatrixLogicalPixel((bench.cycles % kMatrixLedCount), 0, 18, 0);
    showMatrix();
}

void printBenchmarkList() {
    Serial.println("bench modes:");
    Serial.println("  bench");
    Serial.println("    starts pair benchmark: writeOneWord + readOneWord");
    Serial.println("  bench row [cycles]");
    Serial.println("  bench wow [cycles]");
    Serial.println("  bench pair [cycles]");
    Serial.println("  bench rw [cycles]");
    Serial.println("  bench ww [cycles]");
    Serial.println("  bench block [cycles]");
    Serial.println("  bench status");
    Serial.println("  bench stop");
    Serial.println("bench devices:");
    Serial.print("  row/wow/pair: ");
    printDeviceAddress(kBenchOneWordDevice);
    Serial.println();
    Serial.print("  rw/ww: ");
    printDeviceAddress(kBenchWordArrayDevice);
    Serial.print("..");
    printDeviceAddress(kBenchWordArrayDevice, static_cast<uint32_t>(kBenchWordPoints - 1U));
    Serial.println();
    Serial.print("  block: ");
    printDeviceAddress(kBenchBlockWordDevice);
    Serial.print("..");
    printDeviceAddress(kBenchBlockWordDevice, static_cast<uint32_t>(kBenchBlockPoints - 1U));
    Serial.println();
}

void printBenchmarkSummary(const char* label) {
    Serial.print(label);
    Serial.print(" mode=");
    Serial.print(benchModeText(bench.mode));
    Serial.print(" cycles=");
    Serial.print(bench.cycles);
    Serial.print(" requests=");
    Serial.print(bench.requests);
    Serial.print(" fail=");
    Serial.print(bench.fail);
    Serial.print(" elapsed_ms=");
    Serial.print(bench.started_ms == 0 ? 0U : (millis() - bench.started_ms));
    Serial.print(" last_cycle_ms=");
    Serial.print(bench.last_cycle_ms);
    Serial.print(" avg_cycle_ms=");
    if (bench.cycles == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(bench.total_cycle_ms / bench.cycles));
    }
    Serial.print(" avg_req_ms=");
    if (bench.requests == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>(bench.total_cycle_ms / bench.requests));
    }
    Serial.print(" min_ms=");
    Serial.print(bench.cycles == 0 ? 0U : bench.min_cycle_ms);
    Serial.print(" max_ms=");
    Serial.print(bench.max_cycle_ms);
    Serial.print(" req_per_sec=");
    const uint32_t elapsed_ms = bench.started_ms == 0 ? 0U : (millis() - bench.started_ms);
    if (elapsed_ms == 0 || bench.requests == 0) {
        Serial.print(0);
    } else {
        Serial.print(static_cast<uint32_t>((static_cast<uint64_t>(bench.requests) * 1000U) / elapsed_ms));
    }
    Serial.print(" last_req_bytes=");
    Serial.print(bench.last_request_bytes);
    Serial.print(" last_resp_bytes=");
    Serial.println(bench.last_response_bytes);
}

void printBenchmarkStatus() {
    Serial.print("bench_active=");
    Serial.println(bench.active ? "yes" : "no");
    Serial.print("bench_mode=");
    Serial.println(benchModeText(bench.mode));
    Serial.print("bench_cycle_limit=");
    Serial.println(bench.cycle_limit);
    Serial.print("bench_last_step=");
    Serial.println(bench.last_step[0] == '\0' ? "<none>" : bench.last_step);
    Serial.print("bench_last_issue=");
    Serial.println(bench.last_issue[0] == '\0' ? "<none>" : bench.last_issue);
    Serial.print("bench_last_error=");
    Serial.println(slmp::errorString(bench.last_error));
    Serial.print("bench_last_end_code=0x");
    Serial.println(bench.last_end_code, HEX);
    printBenchmarkSummary("bench status:");
}

void stopBenchmark(bool print_summary, bool failed) {
    if (!bench.active) {
        return;
    }
    bench.active = false;
    clearBenchTargetsSilently();
    if (failed) {
        renderBenchmarkFailure();
    } else {
        renderDemoInactive();
    }
    if (print_summary) {
        printBenchmarkSummary("bench summary:");
    }
}

bool failBenchmarkCycle(const char* step, const char* issue, const char* plc_label, bool use_plc_error) {
    copyText(bench.last_step, sizeof(bench.last_step), step);
    copyText(bench.last_issue, sizeof(bench.last_issue), issue);
    bench.last_error = use_plc_error ? plc.lastError() : slmp::Error::Ok;
    bench.last_end_code = use_plc_error ? plc.lastEndCode() : 0;
    ++bench.fail;
    Serial.print("bench failed at ");
    Serial.print(step);
    Serial.print(": ");
    Serial.println(issue);
    if (use_plc_error && plc_label != nullptr) {
        printLastPlcError(plc_label);
    }
    stopBenchmark(true, true);
    return false;
}

void recordBenchmarkStep(const char* step, size_t request_count) {
    copyText(bench.last_step, sizeof(bench.last_step), step);
    copyText(bench.last_issue, sizeof(bench.last_issue), "none");
    bench.last_error = slmp::Error::Ok;
    bench.last_end_code = 0;
    bench.last_request_bytes = plc.lastRequestFrameLength();
    bench.last_response_bytes = plc.lastResponseFrameLength();
    bench.requests += request_count;
}

void finishBenchmarkCycle(uint32_t started_ms) {
    bench.last_cycle_ms = millis() - started_ms;
    if (bench.cycles == 0 || bench.last_cycle_ms < bench.min_cycle_ms) {
        bench.min_cycle_ms = bench.last_cycle_ms;
    }
    if (bench.last_cycle_ms > bench.max_cycle_ms) {
        bench.max_cycle_ms = bench.last_cycle_ms;
    }
    bench.total_cycle_ms += bench.last_cycle_ms;
    ++bench.cycles;
    renderBenchmarkStatusBar();
    if (millis() - bench.last_report_ms >= kBenchReportIntervalMs) {
        printBenchmarkSummary("bench progress:");
        bench.last_report_ms = millis();
    }
}

bool runBenchmarkCycle() {
    if (!connectPlc(false)) {
        return failBenchmarkCycle("connect", "plc not connected", nullptr, false);
    }

    const uint32_t started_ms = millis();
    if (bench.mode == BenchMode::Row) {
        uint16_t value = 0;
        if (plc.readOneWord(kBenchOneWordDevice, value) != slmp::Error::Ok) {
            return failBenchmarkCycle("readOneWord", "plc read failed", "bench readOneWord", true);
        }
        recordBenchmarkStep("readOneWord", 1);
    } else if (bench.mode == BenchMode::Wow) {
        const uint16_t value = nextFuncheckWordValue();
        if (plc.writeOneWord(kBenchOneWordDevice, value) != slmp::Error::Ok) {
            return failBenchmarkCycle("writeOneWord", "plc write failed", "bench writeOneWord", true);
        }
        recordBenchmarkStep("writeOneWord", 1);
    } else if (bench.mode == BenchMode::Pair) {
        const uint16_t expected = nextFuncheckWordValue();
        uint16_t readback = 0;
        if (plc.writeOneWord(kBenchOneWordDevice, expected) != slmp::Error::Ok) {
            return failBenchmarkCycle("writeOneWord", "plc write failed", "bench writeOneWord", true);
        }
        if (plc.readOneWord(kBenchOneWordDevice, readback) != slmp::Error::Ok) {
            return failBenchmarkCycle("readOneWord", "plc read failed", "bench readOneWord", true);
        }
        if (readback != expected) {
            return failBenchmarkCycle("pair", "readback mismatch", nullptr, false);
        }
        recordBenchmarkStep("pair", 2);
    } else if (bench.mode == BenchMode::Rw) {
        if (plc.readWords(kBenchWordArrayDevice, static_cast<uint16_t>(kBenchWordPoints), bench_word_readback, kBenchWordPoints) != slmp::Error::Ok) {
            return failBenchmarkCycle("readWords", "plc read failed", "bench readWords", true);
        }
        recordBenchmarkStep("readWords", 1);
    } else if (bench.mode == BenchMode::Ww) {
        fillTxlimitWords(bench_word_values, kBenchWordPoints, nextFuncheckWordValue());
        if (plc.writeWords(kBenchWordArrayDevice, bench_word_values, kBenchWordPoints) != slmp::Error::Ok) {
            return failBenchmarkCycle("writeWords", "plc write failed", "bench writeWords", true);
        }
        recordBenchmarkStep("writeWords", 1);
    } else if (bench.mode == BenchMode::Block) {
        fillTxlimitWords(bench_block_values, kBenchBlockPoints, nextFuncheckWordValue());
        const slmp::DeviceBlockWrite block = {kBenchBlockWordDevice, bench_block_values, static_cast<uint16_t>(kBenchBlockPoints)};
        const slmp::DeviceBlockRead read_block = {kBenchBlockWordDevice, static_cast<uint16_t>(kBenchBlockPoints)};
        if (plc.writeBlock(&block, 1, nullptr, 0) != slmp::Error::Ok) {
            return failBenchmarkCycle("writeBlock", "plc write failed", "bench writeBlock", true);
        }
        if (plc.readBlock(&read_block, 1, nullptr, 0, bench_block_readback, kBenchBlockPoints, nullptr, 0) != slmp::Error::Ok) {
            return failBenchmarkCycle("readBlock", "plc read failed", "bench readBlock", true);
        }
        if (!wordArraysEqual(bench_block_values, bench_block_readback, static_cast<uint16_t>(kBenchBlockPoints))) {
            return failBenchmarkCycle("block", "readback mismatch", nullptr, false);
        }
        recordBenchmarkStep("block", 2);
    } else {
        return failBenchmarkCycle("mode", "unknown mode", nullptr, false);
    }

    finishBenchmarkCycle(started_ms);
    return true;
}

void startBenchmark(BenchMode mode, uint32_t cycle_limit) {
    stopDemoForManualLedControl();
    stopEndurance(false, false);
    resetManualCheckSession();
    resetVerificationRecord();
    clearBenchTargetsSilently();
    resetBenchmarkSession();
    bench.active = true;
    bench.mode = mode;
    bench.started_ms = millis();
    bench.next_cycle_due_ms = bench.started_ms;
    bench.last_report_ms = bench.started_ms;
    bench.cycle_limit = cycle_limit;
    copyText(bench.last_step, sizeof(bench.last_step), "starting");
    copyText(bench.last_issue, sizeof(bench.last_issue), "none");
    renderSolidMatrix(0, 8, 12);
    Serial.println("bench=on");
    Serial.print("bench_mode=");
    Serial.println(benchModeText(mode));
    Serial.print("bench_cycle_limit=");
    Serial.println(cycle_limit);
}

void benchCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        startBenchmark(BenchMode::Pair, 0);
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STATUS") == 0) {
        printBenchmarkStatus();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
        Serial.println("bench=off");
        stopBenchmark(true, false);
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printBenchmarkList();
        return;
    }

    BenchMode mode = BenchMode::None;
    if (strcmp(tokens[1], "ROW") == 0 || strcmp(tokens[1], "READ") == 0) {
        mode = BenchMode::Row;
    } else if (strcmp(tokens[1], "WOW") == 0 || strcmp(tokens[1], "WRITE") == 0) {
        mode = BenchMode::Wow;
    } else if (strcmp(tokens[1], "PAIR") == 0) {
        mode = BenchMode::Pair;
    } else if (strcmp(tokens[1], "RW") == 0) {
        mode = BenchMode::Rw;
    } else if (strcmp(tokens[1], "WW") == 0) {
        mode = BenchMode::Ww;
    } else if (strcmp(tokens[1], "BLOCK") == 0) {
        mode = BenchMode::Block;
    }
    if (mode == BenchMode::None) {
        Serial.println("bench usage: bench [row|wow|pair|rw|ww|block] [cycles]");
        return;
    }

    uint32_t cycle_limit = 0;
    if (token_count >= 3) {
        unsigned long parsed_limit = 0;
        if (!parseUnsignedValue(tokens[2], parsed_limit, 10)) {
            Serial.println("bench usage: bench [row|wow|pair|rw|ww|block] [cycles]");
            return;
        }
        cycle_limit = static_cast<uint32_t>(parsed_limit);
    }
    startBenchmark(mode, cycle_limit);
}

void pollBenchmark() {
    if (!bench.active) {
        return;
    }
    if (millis() < bench.next_cycle_due_ms) {
        return;
    }
    if (!runBenchmarkCycle()) {
        printPrompt();
        return;
    }
    if (bench.cycle_limit > 0 && bench.cycles >= bench.cycle_limit) {
        Serial.println("bench limit reached");
        stopBenchmark(true, false);
        printPrompt();
        return;
    }
    bench.next_cycle_due_ms = millis();
}

bool advanceManualCheckCursorIfReady() {
    if (!manual_check.waiting_for_judge) {
        return true;
    }
    if (verification.active && !verification.judged) {
        Serial.println("check waiting for judgement: button=yes, console 'ng [note]'=no");
        return false;
    }
    manual_check.waiting_for_judge = false;
    ++manual_check.next_step_index;
    return true;
}

bool manualCheckFailureShouldAutoSkip() {
    if (plc.lastError() == slmp::Error::UnsupportedDevice) {
        return true;
    }
    if (plc.lastError() != slmp::Error::PlcError) {
        return false;
    }
    switch (plc.lastEndCode()) {
        case 0xC059:
        case 0xC05B:
        case 0xC061:
            return true;
        default:
            return false;
    }
}

void clearVerificationTargetSilently() {
    if (!verification.active) {
        return;
    }
    switch (verification.kind) {
        case VerificationKind::WordWrite:
            clearWordsSilently(verification.device, verification.points);
            return;
        case VerificationKind::BitWrite:
            clearBitsSilently(verification.device, 1);
            return;
        default:
            return;
    }
}

uint16_t nextFuncheckWordValue() {
    const uint16_t value = static_cast<uint16_t>((esp_random() % 60000U) + 1U);
    return value == 0U ? 1U : value;
}

uint32_t nextFuncheckDWordValue() {
    const uint32_t value = (static_cast<uint32_t>(esp_random()) & 0x7FFFFFFFU) + 1U;
    return value == 0U ? 1U : value;
}

void printFuncheckWordValues(const char* label, const uint16_t* values, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        Serial.print(values[i]);
    }
    Serial.println();
}

void printFuncheckDWordValues(const char* label, const uint32_t* values, size_t count) {
    Serial.print(label);
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            Serial.print(", ");
        }
        Serial.print(values[i]);
    }
    Serial.println();
}

FuncheckResult runFuncheckVerifyWords(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("funcheck direct failed: verifyw command is incomplete");
        return FuncheckResult::Fail;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("funcheck direct failed: verifyw device parse failed");
        return FuncheckResult::Fail;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.println("funcheck direct failed: verifyw accepts 1..8 values");
        return FuncheckResult::Fail;
    }
    uint16_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        uint16_t parsed_value = 0;
        if (!parseWordValue(tokens[i + 2], parsed_value)) {
            Serial.println("funcheck direct failed: verifyw values must fit in 16 bits");
            return FuncheckResult::Fail;
        }
        values[i] = nextFuncheckWordValue();
    }
    printFuncheckWordValues("funcheck randomized_values=", values, static_cast<size_t>(value_count));
    if (!connectPlc(false)) {
        Serial.println("funcheck direct failed: plc not connected");
        return FuncheckResult::Fail;
    }

    uint16_t before[kMaxWordPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(value_count), before, kMaxWordPoints) != slmp::Error::Ok) {
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyw before read");
        return FuncheckResult::Fail;
    }
    if (plc.writeWords(device, values, static_cast<size_t>(value_count)) != slmp::Error::Ok) {
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyw write");
        return FuncheckResult::Fail;
    }
    uint16_t readback[kMaxWordPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(value_count), readback, kMaxWordPoints) != slmp::Error::Ok) {
        clearWordsSilently(device, static_cast<size_t>(value_count));
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyw readback");
        return FuncheckResult::Fail;
    }

    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::WordWrite;
    verification.device = device;
    verification.points = static_cast<uint16_t>(value_count);
    verification.started_ms = millis();
    memcpy(verification.before_words, before, sizeof(uint16_t) * static_cast<size_t>(value_count));
    memcpy(verification.written_words, values, sizeof(uint16_t) * static_cast<size_t>(value_count));
    memcpy(verification.readback_words, readback, sizeof(uint16_t) * static_cast<size_t>(value_count));
    verification.readback_match = wordArraysEqual(values, readback, static_cast<uint16_t>(value_count));
    verification.judged = true;
    verification.pass = verification.readback_match;
    printVerificationSummary();
    const FuncheckResult result = verification.readback_match ? FuncheckResult::Ok : FuncheckResult::Fail;
    clearVerificationTargetSilently();
    resetVerificationRecord();
    return result;
}

FuncheckResult runFuncheckVerifyBit(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println("funcheck direct failed: verifyb command is incomplete");
        return FuncheckResult::Fail;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println("funcheck direct failed: verifyb device parse failed");
        return FuncheckResult::Fail;
    }
    bool value = false;
    if (!parseBoolValue(tokens[2], value)) {
        Serial.println("funcheck direct failed: verifyb value must be 0/1");
        return FuncheckResult::Fail;
    }
    if (!connectPlc(false)) {
        Serial.println("funcheck direct failed: plc not connected");
        return FuncheckResult::Fail;
    }

    bool before = false;
    if (plc.readOneBit(device, before) != slmp::Error::Ok) {
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyb before read");
        return FuncheckResult::Fail;
    }
    if (plc.writeOneBit(device, value) != slmp::Error::Ok) {
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyb write");
        return FuncheckResult::Fail;
    }
    bool readback = false;
    if (plc.readOneBit(device, readback) != slmp::Error::Ok) {
        clearBitsSilently(device, 1);
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        printLastPlcError("funcheck verifyb readback");
        return FuncheckResult::Fail;
    }

    resetVerificationRecord();
    verification.active = true;
    verification.kind = VerificationKind::BitWrite;
    verification.device = device;
    verification.points = 1;
    verification.started_ms = millis();
    verification.before_bit = before;
    verification.written_bit = value;
    verification.readback_bit = readback;
    verification.readback_match = value == readback;
    verification.judged = true;
    verification.pass = verification.readback_match;
    printVerificationSummary();
    const FuncheckResult result = verification.readback_match ? FuncheckResult::Ok : FuncheckResult::Fail;
    clearVerificationTargetSilently();
    resetVerificationRecord();
    return result;
}

FuncheckResult runAutomaticDirectStep(const example_config::ManualCheckStep& step) {
    char command_line[kSerialLineCapacity] = {};
    strncpy(command_line, step.command, sizeof(command_line) - 1U);
    command_line[sizeof(command_line) - 1U] = '\0';

    char* tokens[kMaxTokens] = {};
    const int token_count = splitTokens(command_line, tokens, static_cast<int>(kMaxTokens));
    if (token_count == 0) {
        Serial.println("funcheck direct failed: empty command");
        return FuncheckResult::Fail;
    }
    uppercaseInPlace(tokens[0]);
    if (strcmp(tokens[0], "VERIFYW") == 0 || strcmp(tokens[0], "VW") == 0) {
        return runFuncheckVerifyWords(tokens, token_count);
    }
    if (strcmp(tokens[0], "VERIFYB") == 0 || strcmp(tokens[0], "VB") == 0) {
        return runFuncheckVerifyBit(tokens, token_count);
    }
    Serial.print("funcheck direct failed: unsupported step command ");
    Serial.println(tokens[0]);
    return FuncheckResult::Fail;
}

FuncheckSummary runFuncheckDirectSuite() {
    FuncheckSummary summary = {};
    Serial.println("funcheck suite=direct");
    if (manualCheckConfigLooksUnset()) {
        Serial.println("funcheck direct failed: check config empty; edit config.h");
        summary.fail = 1;
        return summary;
    }

    size_t display_index = 0;
    for (size_t i = 0; i < kManualCheckStepCount; ++i) {
        const example_config::ManualCheckStep& step = example_config::kManualCheckSteps[i];
        if (manualCheckStepIsEmpty(step)) {
            continue;
        }
        ++display_index;
        Serial.println("-------------------");
        Serial.print("funcheck direct step ");
        Serial.print(display_index);
        Serial.print("/");
        Serial.println(kManualCheckStepCount);
        if (step.guide != nullptr && step.guide[0] != '\0') {
            Serial.print("funcheck guide: ");
            Serial.println(step.guide);
        }
        if (step.preview != nullptr && step.preview[0] != '\0') {
            Serial.print("funcheck target: ");
            Serial.println(step.preview);
        }
        Serial.print("funcheck command: ");
        Serial.println(step.command);

        const FuncheckResult result = runAutomaticDirectStep(step);
        recordFuncheckResult(summary, result);
        Serial.print("funcheck result=");
        Serial.println(funcheckResultText(result));
        if (result == FuncheckResult::Skip) {
            Serial.print("funcheck skip_reason=");
            Serial.println(slmp::errorString(plc.lastError()));
        }
    }

    Serial.print("funcheck direct summary: ok=");
    Serial.print(summary.ok);
    Serial.print(" skip=");
    Serial.print(summary.skip);
    Serial.print(" fail=");
    Serial.println(summary.fail);
    return summary;
}

void runManualCheckStep() {
    if (manualCheckConfigLooksUnset()) {
        Serial.println("check config empty; edit config.h");
        return;
    }
    if (!advanceManualCheckCursorIfReady()) {
        return;
    }
    while (manual_check.next_step_index < kManualCheckStepCount) {
        const example_config::ManualCheckStep& step = example_config::kManualCheckSteps[manual_check.next_step_index];
        if (manualCheckStepIsEmpty(step)) {
            ++manual_check.next_step_index;
            continue;
        }
        manual_check.active = true;
        manual_check.last_guide[0] = '\0';
        manual_check.last_preview[0] = '\0';
        if (step.guide != nullptr) {
            strncpy(manual_check.last_guide, step.guide, sizeof(manual_check.last_guide) - 1U);
            manual_check.last_guide[sizeof(manual_check.last_guide) - 1U] = '\0';
        }
        if (step.preview != nullptr) {
            strncpy(manual_check.last_preview, step.preview, sizeof(manual_check.last_preview) - 1U);
            manual_check.last_preview[sizeof(manual_check.last_preview) - 1U] = '\0';
        }
        strncpy(manual_check.last_command, step.command, sizeof(manual_check.last_command) - 1U);
        manual_check.last_command[sizeof(manual_check.last_command) - 1U] = '\0';

        Serial.println("-------------------");
        Serial.print("check step ");
        Serial.print(manual_check.next_step_index + 1);
        Serial.print("/");
        Serial.println(kManualCheckStepCount);
        if (manual_check.last_guide[0] != '\0') {
            Serial.print("check guide: ");
            Serial.println(manual_check.last_guide);
        }
        if (manual_check.last_preview[0] != '\0') {
            Serial.print("check target: ");
            Serial.println(manual_check.last_preview);
        }
        Serial.print("check command: ");
        Serial.println(manual_check.last_command);

        resetVerificationRecord();
        char command_line[kSerialLineCapacity] = {};
        strncpy(command_line, manual_check.last_command, sizeof(command_line) - 1U);
        command_line[sizeof(command_line) - 1U] = '\0';
        executeCommandLine(command_line, false);

        if (!verification.active) {
            if (manualCheckFailureShouldAutoSkip()) {
                Serial.print("check auto-skip: ");
                Serial.println(slmp::errorString(plc.lastError()));
                ++manual_check.next_step_index;
                if (manual_check.next_step_index >= kManualCheckStepCount) {
                    manual_check.active = false;
                    Serial.println("check complete");
                    return;
                }
                continue;
            }
            manual_check.waiting_for_judge = false;
            Serial.println("check step failed before judgement");
            Serial.println("fix the cause, then enter: check next");
            return;
        }

        if (verification.active && !verification.judged) {
            manual_check.waiting_for_judge = true;
            Serial.println("check judgement required");
            return;
        }

        ++manual_check.next_step_index;
        if (manual_check.next_step_index < kManualCheckStepCount) {
            Serial.println("check step complete; starting next step is available with: check next");
        } else {
            manual_check.active = false;
            Serial.println("check complete");
        }
        return;
    }

    manual_check.active = false;
    Serial.println("check complete");
}

bool setManualCheckJudgement(bool pass, const char* note) {
    if (!verification.active) {
        Serial.println("check judge failed: no active verification");
        return false;
    }
    verification.pass = pass;
    verification.judged = true;
    verification.note[0] = '\0';
    if (note != nullptr && note[0] != '\0') {
        strncpy(verification.note, note, sizeof(verification.note) - 1U);
        verification.note[sizeof(verification.note) - 1U] = '\0';
    }
    printVerificationSummary();
    return true;
}

void clearAcceptedVerificationSilently() {
    if (!verification.active || !verification.pass) {
        return;
    }
    clearVerificationTargetSilently();
}

void completeManualCheckJudgement(bool pass, const char* note) {
    if (!setManualCheckJudgement(pass, note)) {
        return;
    }
    if (pass) {
        clearAcceptedVerificationSilently();
    }
    if (!manual_check.waiting_for_judge) {
        return;
    }
    manual_check.waiting_for_judge = false;
    ++manual_check.next_step_index;
    if (manual_check.next_step_index >= kManualCheckStepCount) {
        manual_check.active = false;
        Serial.println("check complete");
        return;
    }
    Serial.println("check accepted; moving to next step");
    runManualCheckStep();
}

void manualCheckNgCommand(char* tokens[], int token_count) {
    if (!manual_check.waiting_for_judge) {
        Serial.println("ng failed: check is not waiting for judgement");
        return;
    }
    char note[kVerificationNoteCapacity] = {};
    joinTokens(tokens, 1, token_count, note, sizeof(note));
    completeManualCheckJudgement(false, note);
}

void manualCheckOkCommand(char* tokens[], int token_count) {
    if (!manual_check.waiting_for_judge) {
        Serial.println("ok failed: check is not waiting for judgement");
        return;
    }
    char note[kVerificationNoteCapacity] = {};
    joinTokens(tokens, 1, token_count, note, sizeof(note));
    completeManualCheckJudgement(true, note);
}

void manualCheckSkipCommand(char* tokens[], int token_count) {
    if (!manual_check.active) {
        Serial.println("skip failed: check is not active");
        return;
    }
    char note[kVerificationNoteCapacity] = {};
    joinTokens(tokens, 1, token_count, note, sizeof(note));
    if (note[0] != '\0') {
        Serial.print("check skipped: ");
        Serial.println(note);
    } else {
        Serial.println("check skipped");
    }
    manual_check.waiting_for_judge = false;
    resetVerificationRecord();
    ++manual_check.next_step_index;
    if (manual_check.next_step_index >= kManualCheckStepCount) {
        manual_check.active = false;
        Serial.println("check complete");
        return;
    }
    runManualCheckStep();
}

void manualCheckCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        stopDemoForManualLedControl();
        resetManualCheckSession();
        resetVerificationRecord();
        runManualCheckStep();
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        stopDemoForManualLedControl();
        resetManualCheckSession();
        resetVerificationRecord();
        runManualCheckStep();
        return;
    }
    if (strcmp(tokens[1], "NEXT") == 0 || strcmp(tokens[1], "CONT") == 0 || strcmp(tokens[1], "RESUME") == 0) {
        runManualCheckStep();
        return;
    }
    if (strcmp(tokens[1], "STATUS") == 0) {
        printManualCheckStatus();
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printManualCheckList();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0 || strcmp(tokens[1], "RESET") == 0) {
        resetManualCheckSession();
        resetVerificationRecord();
        Serial.println("check_mode=off");
        return;
    }

    Serial.println("check usage: check [start|next|status|list|stop]");
}

FuncheckResult runFuncheckApiTypeAndFrames() {
    slmp::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp::Error::Ok) {
        printLastPlcError("funcheck readTypeName");
        return FuncheckResult::Fail;
    }
    if (plc.lastRequestFrameLength() == 0 || plc.lastResponseFrameLength() == 0) {
        Serial.println("funcheck readTypeName failed: dump frames are empty");
        return FuncheckResult::Fail;
    }
    Serial.print("funcheck model=");
    Serial.println(type_name.model);
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiTargetHeader() {
    const slmp::TargetAddress current_target = plc.target();
    plc.setTarget(current_target);
    slmp::TypeNameInfo type_name = {};
    if (plc.readTypeName(type_name) != slmp::Error::Ok) {
        printLastPlcError("funcheck target header");
        return FuncheckResult::Fail;
    }
    const uint8_t* frame = plc.lastRequestFrame();
    if (plc.lastRequestFrameLength() < 15 || frame == nullptr) {
        Serial.println("funcheck target header failed: last request too short");
        return FuncheckResult::Fail;
    }
    if (frame[6] != current_target.network || frame[7] != current_target.station ||
        readFrameLe16(frame + 8) != current_target.module_io || frame[10] != current_target.multidrop) {
        Serial.println("funcheck target header failed: request header mismatch");
        return FuncheckResult::Fail;
    }
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiMonitorAndTimeout() {
    const uint16_t original_monitor = plc.monitoringTimer();
    const uint32_t original_timeout = plc.timeoutMs();
    const uint16_t temporary_monitor = original_monitor == 16U ? 17U : 16U;
    const uint32_t temporary_timeout = original_timeout == 3210U ? 3211U : 3210U;
    plc.setMonitoringTimer(temporary_monitor);
    plc.setTimeoutMs(temporary_timeout);

    slmp::TypeNameInfo type_name = {};
    const slmp::Error error = plc.readTypeName(type_name);
    plc.setMonitoringTimer(original_monitor);
    plc.setTimeoutMs(original_timeout);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck monitor/timeout");
        return FuncheckResult::Fail;
    }
    const uint8_t* frame = plc.lastRequestFrame();
    if (plc.lastRequestFrameLength() < 15 || frame == nullptr) {
        Serial.println("funcheck monitor/timeout failed: last request too short");
        return FuncheckResult::Fail;
    }
    if (readFrameLe16(frame + 13) != temporary_monitor || plc.timeoutMs() != original_timeout) {
        Serial.println("funcheck monitor/timeout failed: state mismatch");
        return FuncheckResult::Fail;
    }
    return FuncheckResult::Ok;
}

FuncheckResult runFuncheckApiOneWord() {
    const uint16_t expected = nextFuncheckWordValue();
    Serial.print("funcheck randomized_value=");
    Serial.println(expected);
    if (plc.writeOneWord(kFuncheckOneWordDevice, expected) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneWord");
        return FuncheckResult::Fail;
    }
    uint16_t readback = 0;
    const slmp::Error error = plc.readOneWord(kFuncheckOneWordDevice, readback);
    clearWordsSilently(kFuncheckOneWordDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneWord");
        return FuncheckResult::Fail;
    }
    return readback == expected ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiWords() {
    const uint16_t expected[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_values=", expected, 2);
    if (plc.writeWords(kFuncheckWordArrayDevice, expected, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeWords");
        return FuncheckResult::Fail;
    }
    uint16_t readback[2] = {};
    const slmp::Error error = plc.readWords(kFuncheckWordArrayDevice, 2, readback, 2);
    clearWordsSilently(kFuncheckWordArrayDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readWords");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected, readback, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiOneBit() {
    if (plc.writeOneBit(kFuncheckOneBitDevice, true) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneBit");
        return FuncheckResult::Fail;
    }
    bool readback = false;
    const slmp::Error error = plc.readOneBit(kFuncheckOneBitDevice, readback);
    clearBitsSilently(kFuncheckOneBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneBit");
        return FuncheckResult::Fail;
    }
    return readback ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiBits() {
    const bool expected[] = {true, false, true, false};
    if (plc.writeBits(kFuncheckBitArrayDevice, expected, 4) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBits");
        return FuncheckResult::Fail;
    }
    bool readback[4] = {};
    const slmp::Error error = plc.readBits(kFuncheckBitArrayDevice, 4, readback, 4);
    clearBitsSilently(kFuncheckBitArrayDevice, 4);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBits");
        return FuncheckResult::Fail;
    }
    return bitArraysEqual(expected, readback, 4) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiOneDWord() {
    const uint32_t expected = nextFuncheckDWordValue();
    Serial.print("funcheck randomized_value=");
    Serial.println(expected);
    if (plc.writeOneDWord(kFuncheckOneDWordDevice, expected) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeOneDWord");
        return FuncheckResult::Fail;
    }
    uint32_t readback = 0;
    const slmp::Error error = plc.readOneDWord(kFuncheckOneDWordDevice, readback);
    clearDWordsSilently(kFuncheckOneDWordDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readOneDWord");
        return FuncheckResult::Fail;
    }
    return readback == expected ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiDWords() {
    const uint32_t expected[] = {nextFuncheckDWordValue(), nextFuncheckDWordValue()};
    printFuncheckDWordValues("funcheck randomized_values=", expected, 2);
    if (plc.writeDWords(kFuncheckDWordArrayDevice, expected, 2) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeDWords");
        return FuncheckResult::Fail;
    }
    uint32_t readback[2] = {};
    const slmp::Error error = plc.readDWords(kFuncheckDWordArrayDevice, 2, readback, 2);
    clearDWordsSilently(kFuncheckDWordArrayDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readDWords");
        return FuncheckResult::Fail;
    }
    return dwordArraysEqual(expected, readback, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiRandomWords() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const uint32_t expected_dwords[] = {nextFuncheckDWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    printFuncheckDWordValues("funcheck randomized_dword_values=", expected_dwords, 1);
    if (plc.writeRandomWords(
            kFuncheckRandomWordDevices,
            expected_words,
            sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
            kFuncheckRandomDWordDevices,
            expected_dwords,
            sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0])
        ) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeRandomWords");
        return FuncheckResult::Fail;
    }

    uint16_t readback_words[2] = {};
    uint32_t readback_dwords[1] = {};
    const slmp::Error error = plc.readRandom(
        kFuncheckRandomWordDevices,
        sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
        readback_words,
        2,
        kFuncheckRandomDWordDevices,
        sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0]),
        readback_dwords,
        1
    );
    clearRandomWordsSilently(
        kFuncheckRandomWordDevices,
        sizeof(kFuncheckRandomWordDevices) / sizeof(kFuncheckRandomWordDevices[0]),
        kFuncheckRandomDWordDevices,
        sizeof(kFuncheckRandomDWordDevices) / sizeof(kFuncheckRandomDWordDevices[0])
    );
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandom");
        return FuncheckResult::Fail;
    }
    if (!wordArraysEqual(expected_words, readback_words, 2)) {
        return FuncheckResult::Fail;
    }
    return dwordArraysEqual(expected_dwords, readback_dwords, 1) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiRandomBits() {
    const bool expected[] = {true, false};
    if (plc.writeRandomBits(
            kFuncheckRandomBitDevices,
            expected,
            sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0])
        ) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeRandomBits");
        return FuncheckResult::Fail;
    }
    bool first = false;
    bool second = false;
    const slmp::Error first_error = plc.readOneBit(kFuncheckRandomBitDevices[0], first);
    const slmp::Error second_error = plc.readOneBit(kFuncheckRandomBitDevices[1], second);
    clearRandomBitsSilently(
        kFuncheckRandomBitDevices,
        sizeof(kFuncheckRandomBitDevices) / sizeof(kFuncheckRandomBitDevices[0])
    );
    if (first_error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandomBits first");
        return FuncheckResult::Fail;
    }
    if (second_error != slmp::Error::Ok) {
        printLastPlcError("funcheck readRandomBits second");
        return FuncheckResult::Fail;
    }
    return (first == expected[0] && second == expected[1]) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiWordBlock() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    const slmp::DeviceBlockWrite word_blocks[] = {
        slmp::dev::blockWrite(kFuncheckBlockWordDevice, expected_words, 2),
    };
    if (plc.writeBlock(word_blocks, 1, nullptr, 0) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock words");
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_word_blocks[] = {
        slmp::dev::blockRead(kFuncheckBlockWordDevice, 2),
    };
    uint16_t readback_words[2] = {};
    const slmp::Error error = plc.readBlock(read_word_blocks, 1, nullptr, 0, readback_words, 2, nullptr, 0);
    clearWordsSilently(kFuncheckBlockWordDevice, 2);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock words");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_words, readback_words, 2) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiBitBlock() {
    const uint16_t expected_bits[] = {nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_packed_bits=", expected_bits, 1);
    const slmp::DeviceBlockWrite bit_blocks[] = {
        slmp::dev::blockWrite(kFuncheckBlockBitDevice, expected_bits, 1),
    };
    if (plc.writeBlock(nullptr, 0, bit_blocks, 1) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock bits");
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_bit_blocks[] = {
        slmp::dev::blockRead(kFuncheckBlockBitDevice, 1),
    };
    uint16_t readback_bits[1] = {};
    const slmp::Error error = plc.readBlock(nullptr, 0, read_bit_blocks, 1, nullptr, 0, readback_bits, 1);
    clearPackedBitWordsSilently(kFuncheckBlockBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock bits");
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_bits, readback_bits, 1) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckResult runFuncheckApiMixedBlock() {
    const uint16_t expected_words[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const uint16_t expected_bits[] = {nextFuncheckWordValue()};
    printFuncheckWordValues("funcheck randomized_word_values=", expected_words, 2);
    printFuncheckWordValues("funcheck randomized_packed_bits=", expected_bits, 1);
    const slmp::DeviceBlockWrite word_blocks[] = {
        slmp::dev::blockWrite(kFuncheckBlockWordDevice, expected_words, 2),
    };
    const slmp::DeviceBlockWrite bit_blocks[] = {
        slmp::dev::blockWrite(kFuncheckBlockBitDevice, expected_bits, 1),
    };
    if (plc.writeBlock(word_blocks, 1, bit_blocks, 1) != slmp::Error::Ok) {
        printLastPlcError("funcheck writeBlock mixed");
        clearBlockSilently(kFuncheckBlockWordDevice, 2, kFuncheckBlockBitDevice, 1);
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        return FuncheckResult::Fail;
    }

    const slmp::DeviceBlockRead read_word_blocks[] = {
        slmp::dev::blockRead(kFuncheckBlockWordDevice, 2),
    };
    const slmp::DeviceBlockRead read_bit_blocks[] = {
        slmp::dev::blockRead(kFuncheckBlockBitDevice, 1),
    };
    uint16_t readback_words[2] = {};
    uint16_t readback_bits[1] = {};
    const slmp::Error error = plc.readBlock(read_word_blocks, 1, read_bit_blocks, 1, readback_words, 2, readback_bits, 1);
    clearBlockSilently(kFuncheckBlockWordDevice, 2, kFuncheckBlockBitDevice, 1);
    if (error != slmp::Error::Ok) {
        printLastPlcError("funcheck readBlock mixed");
        if (manualCheckFailureShouldAutoSkip()) {
            return FuncheckResult::Skip;
        }
        return FuncheckResult::Fail;
    }
    if (!wordArraysEqual(expected_words, readback_words, 2)) {
        return FuncheckResult::Fail;
    }
    return wordArraysEqual(expected_bits, readback_bits, 1) ? FuncheckResult::Ok : FuncheckResult::Fail;
}

FuncheckSummary runFuncheckApiSuite() {
    FuncheckSummary summary = {};
    Serial.println("funcheck suite=api");
    if (!connectPlc(false)) {
        Serial.println("funcheck api failed: plc not connected");
        summary.fail = 1;
        return summary;
    }

    struct ApiCase {
        const char* name;
        FuncheckResult (*run)();
    };

    const ApiCase cases[] = {
        {"readTypeName + dump", runFuncheckApiTypeAndFrames},
        {"target header", runFuncheckApiTargetHeader},
        {"monitor + timeout", runFuncheckApiMonitorAndTimeout},
        {"readOneWord/writeOneWord", runFuncheckApiOneWord},
        {"readWords/writeWords", runFuncheckApiWords},
        {"readOneBit/writeOneBit", runFuncheckApiOneBit},
        {"readBits/writeBits", runFuncheckApiBits},
        {"readOneDWord/writeOneDWord", runFuncheckApiOneDWord},
        {"readDWords/writeDWords", runFuncheckApiDWords},
        {"readRandom/writeRandomWords", runFuncheckApiRandomWords},
        {"writeRandomBits", runFuncheckApiRandomBits},
        {"readBlock/writeBlock words", runFuncheckApiWordBlock},
        {"readBlock/writeBlock bits", runFuncheckApiBitBlock},
        {"readBlock/writeBlock mixed", runFuncheckApiMixedBlock},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Serial.println("-------------------");
        Serial.print("funcheck api step ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(sizeof(cases) / sizeof(cases[0]));
        Serial.print("funcheck api: ");
        Serial.println(cases[i].name);
        const FuncheckResult result = cases[i].run();
        recordFuncheckResult(summary, result);
        Serial.print("funcheck result=");
        Serial.println(funcheckResultText(result));
        if (result == FuncheckResult::Skip) {
            Serial.print("funcheck skip_reason=");
            Serial.println(slmp::errorString(plc.lastError()));
        }
    }

    Serial.print("funcheck api summary: ok=");
    Serial.print(summary.ok);
    Serial.print(" skip=");
    Serial.print(summary.skip);
    Serial.print(" fail=");
    Serial.println(summary.fail);
    return summary;
}

void printFuncheckSummary(const char* label, const FuncheckSummary& summary) {
    Serial.print("funcheck ");
    Serial.print(label);
    Serial.print(" summary: ok=");
    Serial.print(summary.ok);
    Serial.print(" skip=");
    Serial.print(summary.skip);
    Serial.print(" fail=");
    Serial.println(summary.fail);
}

void runFuncheckAll() {
    stopDemoForManualLedControl();
    resetManualCheckSession();
    resetVerificationRecord();
    const FuncheckSummary direct_summary = runFuncheckDirectSuite();
    const FuncheckSummary api_summary = runFuncheckApiSuite();
    FuncheckSummary total = {};
    total.ok = direct_summary.ok + api_summary.ok;
    total.skip = direct_summary.skip + api_summary.skip;
    total.fail = direct_summary.fail + api_summary.fail;
    printFuncheckSummary("all", total);
}

void runFuncheckDirectOnly() {
    stopDemoForManualLedControl();
    resetManualCheckSession();
    resetVerificationRecord();
    (void)runFuncheckDirectSuite();
}

void runFuncheckApiOnly() {
    stopDemoForManualLedControl();
    resetManualCheckSession();
    resetVerificationRecord();
    (void)runFuncheckApiSuite();
}

void funcheckCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        runFuncheckAll();
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "ALL") == 0 || strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        runFuncheckAll();
        return;
    }
    if (strcmp(tokens[1], "DIRECT") == 0 || strcmp(tokens[1], "DEVICES") == 0) {
        runFuncheckDirectOnly();
        return;
    }
    if (strcmp(tokens[1], "API") == 0 || strcmp(tokens[1], "FUNCTIONS") == 0) {
        runFuncheckApiOnly();
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printFuncheckList();
        return;
    }

    Serial.println("funcheck usage: funcheck [all|direct|api|list]");
}

void resetEnduranceSession() {
    endurance = EnduranceSession();
}

void resetReconnectSession() {
    reconnect = ReconnectSession();
}

void stopReconnect(bool print_summary) {
    if (!reconnect.active) {
        return;
    }
    reconnect.active = false;
    renderDemoInactive();
    if (print_summary) {
        printReconnectSummary("reconnect summary:");
    }
}

void stopEndurance(bool print_summary, bool failed) {
    if (!endurance.active) {
        return;
    }
    endurance.active = false;
    clearEnduranceTargetsSilently();
    if (failed) {
        renderEnduranceFailure();
    } else {
        renderDemoInactive();
    }
    if (print_summary) {
        printEnduranceSummary("endurance summary:");
    }
}

bool failEnduranceCycle(const char* step, const char* issue, const char* plc_label, bool use_plc_error) {
    copyText(endurance.last_step, sizeof(endurance.last_step), step);
    copyText(endurance.last_issue, sizeof(endurance.last_issue), issue);
    endurance.last_error = use_plc_error ? plc.lastError() : slmp::Error::Ok;
    endurance.last_end_code = use_plc_error ? plc.lastEndCode() : 0;
    ++endurance.attempts;
    ++endurance.fail;
    Serial.print("endurance failed at ");
    Serial.print(step);
    Serial.print(": ");
    Serial.println(issue);
    if (use_plc_error && plc_label != nullptr) {
        printLastPlcError(plc_label);
    }
    stopEndurance(true, true);
    return false;
}

bool recordEnduranceCycleTiming(uint32_t started_ms) {
    endurance.last_cycle_ms = millis() - started_ms;
    if (endurance.attempts == 0 || endurance.last_cycle_ms < endurance.min_cycle_ms) {
        endurance.min_cycle_ms = endurance.last_cycle_ms;
    }
    if (endurance.last_cycle_ms > endurance.max_cycle_ms) {
        endurance.max_cycle_ms = endurance.last_cycle_ms;
    }
    endurance.total_cycle_ms += endurance.last_cycle_ms;
    renderEnduranceStatusBar();
    if (millis() - endurance.last_report_ms >= kEnduranceReportIntervalMs) {
        printEnduranceSummary("endurance progress:");
        endurance.last_report_ms = millis();
    }
    return true;
}

bool runEnduranceCycle() {
    if (!connectPlc(false)) {
        return failEnduranceCycle("connect", "plc not connected", nullptr, false);
    }

    const uint32_t started_ms = millis();
    copyText(endurance.last_issue, sizeof(endurance.last_issue), "");
    endurance.last_error = slmp::Error::Ok;
    endurance.last_end_code = 0;

    const uint16_t one_word_value = nextFuncheckWordValue();
    uint16_t one_word_readback = 0;
    if (plc.writeOneWord(kEnduranceOneWordDevice, one_word_value) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneWord", "plc write failed", "endurance writeOneWord", true);
    }
    if (plc.readOneWord(kEnduranceOneWordDevice, one_word_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneWord", "plc read failed", "endurance readOneWord", true);
    }
    if (one_word_readback != one_word_value) {
        return failEnduranceCycle("readOneWord/writeOneWord", "readback mismatch", nullptr, false);
    }
    clearWordsSilently(kEnduranceOneWordDevice, 1);

    const uint16_t word_values[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    uint16_t word_readback[2] = {};
    if (plc.writeWords(kEnduranceWordArrayDevice, word_values, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("writeWords", "plc write failed", "endurance writeWords", true);
    }
    if (plc.readWords(kEnduranceWordArrayDevice, 2, word_readback, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("readWords", "plc read failed", "endurance readWords", true);
    }
    if (!wordArraysEqual(word_values, word_readback, 2)) {
        return failEnduranceCycle("readWords/writeWords", "readback mismatch", nullptr, false);
    }
    clearWordsSilently(kEnduranceWordArrayDevice, 2);

    bool one_bit_readback = false;
    if (plc.writeOneBit(kEnduranceOneBitDevice, true) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneBit", "plc write failed", "endurance writeOneBit", true);
    }
    if (plc.readOneBit(kEnduranceOneBitDevice, one_bit_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneBit", "plc read failed", "endurance readOneBit", true);
    }
    if (!one_bit_readback) {
        return failEnduranceCycle("readOneBit/writeOneBit", "readback mismatch", nullptr, false);
    }
    clearBitsSilently(kEnduranceOneBitDevice, 1);

    bool bit_values[4] = {
        (esp_random() & 0x01U) != 0U,
        (esp_random() & 0x01U) != 0U,
        (esp_random() & 0x01U) != 0U,
        (esp_random() & 0x01U) != 0U,
    };
    if (!bit_values[0] && !bit_values[1] && !bit_values[2] && !bit_values[3]) {
        bit_values[0] = true;
    }
    bool bit_readback[4] = {};
    if (plc.writeBits(kEnduranceBitArrayDevice, bit_values, 4) != slmp::Error::Ok) {
        return failEnduranceCycle("writeBits", "plc write failed", "endurance writeBits", true);
    }
    if (plc.readBits(kEnduranceBitArrayDevice, 4, bit_readback, 4) != slmp::Error::Ok) {
        return failEnduranceCycle("readBits", "plc read failed", "endurance readBits", true);
    }
    if (!bitArraysEqual(bit_values, bit_readback, 4)) {
        return failEnduranceCycle("readBits/writeBits", "readback mismatch", nullptr, false);
    }
    clearBitsSilently(kEnduranceBitArrayDevice, 4);

    const uint32_t one_dword_value = nextFuncheckDWordValue();
    uint32_t one_dword_readback = 0;
    if (plc.writeOneDWord(kEnduranceOneDWordDevice, one_dword_value) != slmp::Error::Ok) {
        return failEnduranceCycle("writeOneDWord", "plc write failed", "endurance writeOneDWord", true);
    }
    if (plc.readOneDWord(kEnduranceOneDWordDevice, one_dword_readback) != slmp::Error::Ok) {
        return failEnduranceCycle("readOneDWord", "plc read failed", "endurance readOneDWord", true);
    }
    if (one_dword_readback != one_dword_value) {
        return failEnduranceCycle("readOneDWord/writeOneDWord", "readback mismatch", nullptr, false);
    }
    clearDWordsSilently(kEnduranceOneDWordDevice, 1);

    const uint32_t dword_values[] = {nextFuncheckDWordValue(), nextFuncheckDWordValue()};
    uint32_t dword_readback[2] = {};
    if (plc.writeDWords(kEnduranceDWordArrayDevice, dword_values, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("writeDWords", "plc write failed", "endurance writeDWords", true);
    }
    if (plc.readDWords(kEnduranceDWordArrayDevice, 2, dword_readback, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("readDWords", "plc read failed", "endurance readDWords", true);
    }
    if (!dwordArraysEqual(dword_values, dword_readback, 2)) {
        return failEnduranceCycle("readDWords/writeDWords", "readback mismatch", nullptr, false);
    }
    clearDWordsSilently(kEnduranceDWordArrayDevice, 2);

    const uint16_t random_word_values[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const uint32_t random_dword_values[] = {nextFuncheckDWordValue()};
    uint16_t random_word_readback[2] = {};
    uint32_t random_dword_readback[1] = {};
    if (plc.writeRandomWords(
            kEnduranceRandomWordDevices,
            random_word_values,
            2,
            kEnduranceRandomDWordDevices,
            random_dword_values,
            1) != slmp::Error::Ok) {
        return failEnduranceCycle("writeRandomWords", "plc write failed", "endurance writeRandomWords", true);
    }
    if (plc.readRandom(
            kEnduranceRandomWordDevices,
            2,
            random_word_readback,
            2,
            kEnduranceRandomDWordDevices,
            1,
            random_dword_readback,
            1) != slmp::Error::Ok) {
        return failEnduranceCycle("readRandom", "plc read failed", "endurance readRandom", true);
    }
    if (!wordArraysEqual(random_word_values, random_word_readback, 2) ||
        !dwordArraysEqual(random_dword_values, random_dword_readback, 1)) {
        return failEnduranceCycle("readRandom/writeRandomWords", "readback mismatch", nullptr, false);
    }
    clearRandomWordsSilently(kEnduranceRandomWordDevices, 2, kEnduranceRandomDWordDevices, 1);

    bool random_bit_values[] = {
        (esp_random() & 0x01U) != 0U,
        (esp_random() & 0x01U) != 0U,
    };
    if (!random_bit_values[0] && !random_bit_values[1]) {
        random_bit_values[0] = true;
    }
    bool random_bit_readback[2] = {};
    if (plc.writeRandomBits(kEnduranceRandomBitDevices, random_bit_values, 2) != slmp::Error::Ok) {
        return failEnduranceCycle("writeRandomBits", "plc write failed", "endurance writeRandomBits", true);
    }
    if (plc.readBits(kEnduranceRandomBitDevices[0], 1, random_bit_readback, 1) != slmp::Error::Ok) {
        return failEnduranceCycle("readRandomBits first", "plc read failed", "endurance readRandomBits first", true);
    }
    if (plc.readBits(kEnduranceRandomBitDevices[1], 1, random_bit_readback + 1, 1) != slmp::Error::Ok) {
        return failEnduranceCycle("readRandomBits second", "plc read failed", "endurance readRandomBits second", true);
    }
    if (!bitArraysEqual(random_bit_values, random_bit_readback, 2)) {
        return failEnduranceCycle("readRandomBits/writeRandomBits", "readback mismatch", nullptr, false);
    }
    clearRandomBitsSilently(kEnduranceRandomBitDevices, 2);

    const uint16_t block_word_values[] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
    const slmp::DeviceBlockWrite word_block = {kEnduranceBlockWordDevice, block_word_values, 2};
    const slmp::DeviceBlockRead read_word_block = {kEnduranceBlockWordDevice, 2};
    uint16_t block_word_readback[2] = {};
    if (plc.writeBlock(&word_block, 1, nullptr, 0) != slmp::Error::Ok) {
        return failEnduranceCycle("writeBlock words", "plc write failed", "endurance writeBlock words", true);
    }
    if (plc.readBlock(&read_word_block, 1, nullptr, 0, block_word_readback, 2, nullptr, 0) != slmp::Error::Ok) {
        return failEnduranceCycle("readBlock words", "plc read failed", "endurance readBlock words", true);
    }
    if (!wordArraysEqual(block_word_values, block_word_readback, 2)) {
        return failEnduranceCycle("readBlock/writeBlock words", "readback mismatch", nullptr, false);
    }
    clearWordsSilently(kEnduranceBlockWordDevice, 2);

    const uint16_t block_bit_values[] = {nextFuncheckWordValue()};
    const slmp::DeviceBlockWrite bit_block = {kEnduranceBlockBitDevice, block_bit_values, 1};
    const slmp::DeviceBlockRead read_bit_block = {kEnduranceBlockBitDevice, 1};
    uint16_t block_bit_readback[1] = {};
    if (plc.writeBlock(nullptr, 0, &bit_block, 1) != slmp::Error::Ok) {
        return failEnduranceCycle("writeBlock bits", "plc write failed", "endurance writeBlock bits", true);
    }
    if (plc.readBlock(nullptr, 0, &read_bit_block, 1, nullptr, 0, block_bit_readback, 1) != slmp::Error::Ok) {
        return failEnduranceCycle("readBlock bits", "plc read failed", "endurance readBlock bits", true);
    }
    if (!wordArraysEqual(block_bit_values, block_bit_readback, 1)) {
        return failEnduranceCycle("readBlock/writeBlock bits", "readback mismatch", nullptr, false);
    }
    clearPackedBitWordsSilently(kEnduranceBlockBitDevice, 1);

    ++endurance.attempts;
    ++endurance.ok;
    copyText(endurance.last_step, sizeof(endurance.last_step), "cycle_ok");
    recordEnduranceCycleTiming(started_ms);
    return true;
}

void startEndurance(uint32_t cycle_limit) {
    stopDemoForManualLedControl();
    resetManualCheckSession();
    resetVerificationRecord();
    clearEnduranceTargetsSilently();
    resetEnduranceSession();
    endurance.active = true;
    endurance.started_ms = millis();
    endurance.next_cycle_due_ms = endurance.started_ms;
    endurance.last_report_ms = endurance.started_ms;
    endurance.cycle_limit = cycle_limit;
    copyText(endurance.last_step, sizeof(endurance.last_step), "starting");
    copyText(endurance.last_issue, sizeof(endurance.last_issue), "none");
    renderSolidMatrix(0, 0, 8);
    Serial.println("endurance=on");
    Serial.print("endurance_cycle_limit=");
    Serial.println(cycle_limit);
    printEnduranceList();
}

void enduranceCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        startEndurance(0);
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STATUS") == 0) {
        printEnduranceStatus();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
        Serial.println("endurance=off");
        stopEndurance(true, false);
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printEnduranceList();
        return;
    }
    if (strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        if (token_count >= 3) {
            unsigned long parsed_limit = 0;
            if (!parseUnsignedValue(tokens[2], parsed_limit, 10)) {
                Serial.println("endurance usage: endurance [start [cycles]|status|stop|list]");
                return;
            }
            startEndurance(static_cast<uint32_t>(parsed_limit));
            return;
        }
        startEndurance(0);
        return;
    }

    unsigned long cycle_limit = 0;
    if (parseUnsignedValue(tokens[1], cycle_limit, 10)) {
        startEndurance(static_cast<uint32_t>(cycle_limit));
        return;
    }

    Serial.println("endurance usage: endurance [start [cycles]|status|stop|list]");
}

void pollEnduranceTest() {
    if (!endurance.active) {
        return;
    }
    if (millis() < endurance.next_cycle_due_ms) {
        return;
    }
    if (!runEnduranceCycle()) {
        printPrompt();
        return;
    }
    if (endurance.cycle_limit > 0 && endurance.ok >= endurance.cycle_limit) {
        Serial.println("endurance limit reached");
        stopEndurance(true, false);
        printPrompt();
        return;
    }
    endurance.next_cycle_due_ms = millis() + kEnduranceCycleGapMs;
}

void finishReconnectCycleTiming(uint32_t started_ms) {
    reconnect.last_cycle_ms = millis() - started_ms;
    if (reconnect.attempts == 0 || reconnect.last_cycle_ms < reconnect.min_cycle_ms) {
        reconnect.min_cycle_ms = reconnect.last_cycle_ms;
    }
    if (reconnect.last_cycle_ms > reconnect.max_cycle_ms) {
        reconnect.max_cycle_ms = reconnect.last_cycle_ms;
    }
    reconnect.total_cycle_ms += reconnect.last_cycle_ms;
    renderReconnectStatusBar();
    if (millis() - reconnect.last_report_ms >= kReconnectReportIntervalMs) {
        printReconnectSummary("reconnect progress:");
        reconnect.last_report_ms = millis();
    }
}

bool runReconnectCycle() {
    const uint32_t started_ms = millis();
    const size_t failure_streak_before = reconnect.consecutive_failures;
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), "none");
    reconnect.last_error = slmp::Error::Ok;
    reconnect.last_end_code = 0;

    uint16_t probe_value = 0;
    bool ok = false;
    const char* step = "connect";
    const char* issue = "plc not connected";
    bool use_plc_error = false;

    if (connectPlc(false)) {
        step = "readOneWord";
        issue = "plc read failed";
        use_plc_error = true;
        if (plc.readOneWord(kEnduranceOneWordDevice, probe_value) == slmp::Error::Ok) {
            ok = true;
        }
    }

    ++reconnect.attempts;
    copyText(reconnect.last_step, sizeof(reconnect.last_step), step);

    if (ok) {
        ++reconnect.ok;
        reconnect.consecutive_failures = 0;
        if (failure_streak_before > 0) {
            ++reconnect.recoveries;
            Serial.print("reconnect recovered after ");
            Serial.print(failure_streak_before);
            Serial.print(" failed attempts; probe=");
            printDeviceAddress(kEnduranceOneWordDevice);
            Serial.print(" value=");
            Serial.println(probe_value);
        }
        finishReconnectCycleTiming(started_ms);
        return true;
    }

    ++reconnect.fail;
    ++reconnect.consecutive_failures;
    if (reconnect.consecutive_failures > reconnect.max_consecutive_failures) {
        reconnect.max_consecutive_failures = reconnect.consecutive_failures;
    }
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), issue);
    reconnect.last_error = use_plc_error ? plc.lastError() : slmp::Error::Ok;
    reconnect.last_end_code = use_plc_error ? plc.lastEndCode() : 0;
    plc.close();
    if (reconnect.consecutive_failures == 1) {
        Serial.print("reconnect lost at ");
        Serial.print(step);
        Serial.print(": ");
        Serial.println(issue);
        if (use_plc_error) {
            Serial.print("reconnect last_error=");
            Serial.print(slmp::errorString(reconnect.last_error));
            Serial.print(" end=0x");
            Serial.println(reconnect.last_end_code, HEX);
        }
    }
    finishReconnectCycleTiming(started_ms);
    return true;
}

void startReconnect(uint32_t cycle_limit) {
    stopDemoForManualLedControl();
    stopBenchmark(false, false);
    stopEndurance(false, false);
    resetManualCheckSession();
    resetVerificationRecord();
    resetReconnectSession();
    reconnect.active = true;
    reconnect.started_ms = millis();
    reconnect.next_cycle_due_ms = reconnect.started_ms;
    reconnect.last_report_ms = reconnect.started_ms;
    reconnect.cycle_limit = cycle_limit;
    copyText(reconnect.last_step, sizeof(reconnect.last_step), "starting");
    copyText(reconnect.last_issue, sizeof(reconnect.last_issue), "none");
    renderSolidMatrix(0, 6, 16);
    Serial.println("reconnect=on");
    Serial.print("reconnect_cycle_limit=");
    Serial.println(cycle_limit);
    printReconnectList();
}

void reconnectCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        startReconnect(0);
        return;
    }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STATUS") == 0) {
        printReconnectStatus();
        return;
    }
    if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
        Serial.println("reconnect=off");
        stopReconnect(true);
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        printReconnectList();
        return;
    }
    if (strcmp(tokens[1], "START") == 0 || strcmp(tokens[1], "ON") == 0) {
        if (token_count >= 3) {
            unsigned long parsed_limit = 0;
            if (!parseUnsignedValue(tokens[2], parsed_limit, 10)) {
                Serial.println("reconnect usage: reconnect [start [cycles]|status|stop|list]");
                return;
            }
            startReconnect(static_cast<uint32_t>(parsed_limit));
            return;
        }
        startReconnect(0);
        return;
    }

    unsigned long cycle_limit = 0;
    if (parseUnsignedValue(tokens[1], cycle_limit, 10)) {
        startReconnect(static_cast<uint32_t>(cycle_limit));
        return;
    }

    Serial.println("reconnect usage: reconnect [start [cycles]|status|stop|list]");
}

void pollReconnectTest() {
    if (!reconnect.active) {
        return;
    }
    if (millis() < reconnect.next_cycle_due_ms) {
        return;
    }
    (void)runReconnectCycle();
    if (reconnect.cycle_limit > 0 && reconnect.attempts >= reconnect.cycle_limit) {
        Serial.println("reconnect limit reached");
        stopReconnect(true);
        printPrompt();
        return;
    }
    reconnect.next_cycle_due_ms = millis() + reconnectRetryGapMs();
}

bool bringUpWiFiWithStartupProgress() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    if (wifiConfigLooksUnset()) {
        Serial.println("wifi config looks unset; edit config.h");
    }
    WiFi.begin(example_config::kWifiSsid, example_config::kWifiPassword);

    const uint32_t started_ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
        const uint32_t elapsed_ms = millis() - started_ms;
        const uint32_t timeout_ms = example_config::kWifiConnectTimeoutMs;
        size_t lit_count = 10;
        if (timeout_ms > 0) {
            lit_count += static_cast<size_t>((static_cast<uint64_t>(elapsed_ms) * 8U) / timeout_ms);
        }
        if (lit_count > 18) {
            lit_count = 18;
        }
        renderStartupProgress(lit_count, 0, 10, 20);
        if (elapsed_ms >= timeout_ms) {
            wifi_ready = false;
            printWifiSummary();
            Serial.println("wifi connect timeout");
            return false;
        }
        delay(120);
    }

    wifi_ready = true;
    showStartupPhase(18, 0, 20, 8);
    printWifiSummary();
    return true;
}

bool runStartupDemoWithProgress() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("startup read skipped: wifi not connected");
        showStartupPhase(25, 18, 8, 0);
        return false;
    }

    runStartupDemo();
    if (plc.connected()) {
        showStartupPhase(25, 12, 12, 12);
        return true;
    }

    showStartupPhase(25, 18, 8, 0);
    return false;
}

void setupConsole() {
    showStartupSplash();

    Serial.begin(115200);
    const uint32_t serial_wait_started_ms = millis();
    while (!Serial) {
        if (SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS == 0 ||
            millis() - serial_wait_started_ms >= SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS) {
            break;
        }
        delay(10);
    }

    showStartupPhase(5, 8, 8, 8);
    Serial.println(SLMP_WIFI_SERIAL_CONSOLE_BANNER);
    plc.setTimeoutMs(2000);
    showStartupPhase(10, 0, 16, 8);
    (void)bringUpWiFiWithStartupProgress();
    showStartupPhase(20, 0, 18, 6);
    printHelp();
    showStartupPhase(22, 8, 18, 0);
    (void)runStartupDemoWithProgress();
    delay(kStartupCompleteHoldMs);
}

void setDemoMode(bool enabled) {
    demo_mode_enabled = enabled;
    demo_last_poll_ms = 0;
    if (!enabled) {
        Serial.println("demo_mode=off");
        renderDemoInactive();
        return;
    }
    Serial.println("demo_mode=on");
    Serial.print("demo_button_device=");
    printDeviceAddress(kButtonIncrementDevice);
    Serial.println();
    Serial.print("demo_led_bits=");
    printDeviceAddress(kDemoBitDevice);
    Serial.print("..");
    printDeviceAddress(kDemoBitDevice, static_cast<uint32_t>(kMatrixLedCount - 1U));
    Serial.println();
    renderDemoWaitingForWiFi();
}

void demoCommand(char* tokens[], int token_count) {
    if (token_count == 1) {
        setDemoMode(true);
        return;
    }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "OFF") == 0 || strcmp(tokens[1], "0") == 0 || strcmp(tokens[1], "STOP") == 0) {
        setDemoMode(false);
        return;
    }
    if (strcmp(tokens[1], "ON") == 0 || strcmp(tokens[1], "1") == 0 || strcmp(tokens[1], "START") == 0) {
        setDemoMode(true);
        return;
    }
    Serial.println("demo usage: demo | demo off");
}

void ledColorCommand(char* color_token) {
    uppercaseInPlace(color_token);
    stopDemoForManualLedControl();
    if (strcmp(color_token, "OFF") == 0 || strcmp(color_token, "BLACK") == 0 || strcmp(color_token, "0") == 0) {
        renderSolidMatrix(0, 0, 0);
        Serial.println("led=off");
        return;
    }
    if (strcmp(color_token, "RED") == 0) {
        renderSolidMatrix(24, 0, 0);
        Serial.println("led=red");
        return;
    }
    if (strcmp(color_token, "GREEN") == 0) {
        renderSolidMatrix(0, 24, 0);
        Serial.println("led=green");
        return;
    }
    if (strcmp(color_token, "BLUE") == 0) {
        renderSolidMatrix(0, 0, 24);
        Serial.println("led=blue");
        return;
    }
    if (strcmp(color_token, "WHITE") == 0) {
        renderSolidMatrix(16, 16, 16);
        Serial.println("led=white");
        return;
    }
    Serial.println("led usage: led <off|red|green|blue|white>");
}

void ledTestCommand() {
    stopDemoForManualLedControl();
    Serial.println("ledtest: red");
    renderSolidMatrix(24, 0, 0);
    delay(300);
    Serial.println("ledtest: green");
    renderSolidMatrix(0, 24, 0);
    delay(300);
    Serial.println("ledtest: blue");
    renderSolidMatrix(0, 0, 24);
    delay(300);
    Serial.println("ledtest: white");
    renderSolidMatrix(16, 16, 16);
    delay(300);
    renderSolidMatrix(0, 0, 0);
    Serial.println("ledtest: off");
}

bool handleAtomCustomCommand(char* tokens[], int token_count) {
    if (strcmp(tokens[0], "DEMO") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        demoCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "CHECK") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        manualCheckCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "FUNCHECK") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        funcheckCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "RECONNECT") == 0 || strcmp(tokens[0], "RETRY") == 0) {
        reconnectCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "TXLIMIT") == 0 || strcmp(tokens[0], "TXBUF") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        txlimitCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "BENCH") == 0 || strcmp(tokens[0], "PERF") == 0) {
        stopEndurance(false, false);
        stopReconnect(false);
        benchCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "ENDURANCE") == 0 || strcmp(tokens[0], "SOAK") == 0) {
        stopBenchmark(false, false);
        stopReconnect(false);
        enduranceCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "NG") == 0) {
        manualCheckNgCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "SKIP") == 0) {
        manualCheckSkipCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "OK") == 0) {
        manualCheckOkCommand(tokens, token_count);
        return true;
    }
    if (strcmp(tokens[0], "LED") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        if (token_count < 2) {
            Serial.println("led usage: led <off|red|green|blue|white>");
            return true;
        }
        ledColorCommand(tokens[1]);
        return true;
    }
    if (strcmp(tokens[0], "LEDTEST") == 0) {
        stopBenchmark(false, false);
        stopEndurance(false, false);
        stopReconnect(false);
        ledTestCommand();
        return true;
    }
    return false;
}

void setupButtonIncrement() {
    pinMode(kButtonPin, INPUT);
    button_raw_pressed = isIncrementButtonPressed();
    button_stable_pressed = button_raw_pressed;
    button_last_change_ms = millis();
    button_press_started_ms = button_stable_pressed ? button_last_change_ms : 0;
    button_long_press_handled = false;
    (void)ensureMatrixReady();
    renderDemoInactive();
    Serial.print("demo_button_device=");
    printDeviceAddress(kButtonIncrementDevice);
    Serial.println();
    Serial.print("demo_led_device_range=");
    printDeviceAddress(kDemoBitDevice);
    Serial.print("..");
    printDeviceAddress(kDemoBitDevice, static_cast<uint32_t>(kMatrixLedCount - 1U));
    Serial.println();
    Serial.println("type 'demo' to enable button+matrix mode");
    Serial.println("type 'check' to start guided manual verification");
    Serial.println("type 'funcheck' to run automatic device + api tests");
    Serial.println("type 'endurance' to start continuous durability testing");
    Serial.println("type 'reconnect' to keep retrying after communication loss");
    Serial.println("during reconnect/endurance: long-press button=stop");
    Serial.println("type 'bench' to compare response speed across microcontrollers");
    Serial.println("during check: button=yes, console 'ng [note]'=no");
    Serial.println("type 'ledtest' to verify matrix LEDs");
    printPrompt();
}

void incrementButtonDevice() {
    if (!demo_mode_enabled) {
        return;
    }
    if (!connectPlc(false)) {
        Serial.println();
        Serial.println("button increment skipped: plc not connected");
        printPrompt();
        return;
    }

    uint16_t current_value = 0;
    if (plc.readOneWord(kButtonIncrementDevice, current_value) != slmp::Error::Ok) {
        Serial.println();
        printLastPlcError("button readOneWord");
        printPrompt();
        return;
    }

    const uint16_t next_value = static_cast<uint16_t>(current_value + 1U);
    if (plc.writeOneWord(kButtonIncrementDevice, next_value) != slmp::Error::Ok) {
        Serial.println();
        printLastPlcError("button writeOneWord");
        printPrompt();
        return;
    }

    Serial.println();
    Serial.print("button increment ");
    printDeviceAddress(kButtonIncrementDevice);
    Serial.print(": ");
    Serial.print(current_value);
    Serial.print(" -> ");
    Serial.println(next_value);
    printPrompt();
}

void acceptManualCheckButtonOk() {
    if (!manual_check.waiting_for_judge) {
        return;
    }
    Serial.println();
    Serial.println("button=yes");
    completeManualCheckJudgement(true, "button");
    printPrompt();
}

void stopLongRunningModeByButtonLongPress() {
    if ((!reconnect.active && !endurance.active) || !button_stable_pressed || button_long_press_handled) {
        return;
    }
    if (button_press_started_ms == 0 || millis() - button_press_started_ms < kButtonLongPressMs) {
        return;
    }
    button_long_press_handled = true;
    Serial.println();
    if (reconnect.active) {
        Serial.println("reconnect=off");
        Serial.println("reconnect stop: button long press");
        stopReconnect(true);
    } else if (endurance.active) {
        Serial.println("endurance=off");
        Serial.println("endurance stop: button long press");
        stopEndurance(true, false);
    }
    printPrompt();
}

void pollButtonIncrement() {
    const bool raw_pressed = isIncrementButtonPressed();
    if (raw_pressed != button_raw_pressed) {
        button_raw_pressed = raw_pressed;
        button_last_change_ms = millis();
        return;
    }

    if (millis() - button_last_change_ms < kButtonDebounceMs) {
        return;
    }

    if (button_stable_pressed == button_raw_pressed) {
        if (button_stable_pressed) {
            stopLongRunningModeByButtonLongPress();
        }
        return;
    }

    button_stable_pressed = button_raw_pressed;
    if (button_stable_pressed) {
        button_press_started_ms = millis();
        button_long_press_handled = false;
        if (manual_check.waiting_for_judge) {
            acceptManualCheckButtonOk();
            return;
        }
        incrementButtonDevice();
        return;
    }

    button_press_started_ms = 0;
    button_long_press_handled = false;
}

void pollDemoDisplay() {
    if (!demo_mode_enabled) {
        return;
    }
    if (millis() - demo_last_poll_ms < kDemoPollIntervalMs) {
        return;
    }
    demo_last_poll_ms = millis();

    if (WiFi.status() != WL_CONNECTED) {
        renderDemoWaitingForWiFi();
        return;
    }
    if (!connectPlc(false)) {
        renderDemoWaitingForPlc();
        return;
    }

    bool values[kMatrixLedCount] = {};
    if (plc.readBits(kDemoBitDevice, static_cast<uint16_t>(kMatrixLedCount), values, kMatrixLedCount) != slmp::Error::Ok) {
        renderDemoReadError();
        return;
    }
    renderDemoBits(values);
}

}  // namespace atom_matrix_serial_console

#undef SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_HELP_HANDLER
#undef SLMP_WIFI_SERIAL_CONSOLE_CUSTOM_COMMAND_HANDLER
#undef SLMP_WIFI_SERIAL_CONSOLE_EXTRA_HELP
#undef SLMP_WIFI_SERIAL_CONSOLE_PRINT_PENDING_JUDGE_HINT
#undef SLMP_WIFI_SERIAL_CONSOLE_SERIAL_WAIT_MS
#undef SLMP_WIFI_SERIAL_CONSOLE_BANNER
#undef SLMP_WIFI_SERIAL_CONSOLE_NAMESPACE
#undef SLMP_WIFI_SERIAL_CONSOLE_CONFIG_HEADER
