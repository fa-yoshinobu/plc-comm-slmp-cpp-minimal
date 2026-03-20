/**
 * @example w6300_evb_pico2_serial_console.ino
 * @brief SLMP Serial Console example for W6300-EVB-Pico2.
 * 
 * This example demonstrates:
 * - Connecting to a PLC via Wired Ethernet (W6300).
 * - High-speed serial console for device monitoring and testing.
 * - Hardware-specific initialization for the W6300 shield.
 * 
 * Target Hardware: W6300-EVB-Pico2 (RP2350)
 */

#include "w6300_evb_pico2_serial_console.h"

void setup() {
    w6300_evb_pico2_serial_console::setup();
}

void loop() {
    w6300_evb_pico2_serial_console::loop();
}
