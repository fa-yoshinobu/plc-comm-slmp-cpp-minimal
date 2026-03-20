/**
 * @example atom_matrix_serial_console.ino
 * @brief SLMP Serial Console example for M5Stack Atom Matrix.
 * 
 * This example demonstrates:
 * - Connecting to a PLC via WiFi.
 * - Providing a command-line interface over Serial to read/write devices.
 * - Visualizing communication status on the 5x5 LED matrix.
 * - Performing endurance and benchmark tests.
 * 
 * Target Hardware: M5Stack Atom Matrix (ESP32)
 */

#include "atom_matrix_serial_console.h"

void setup() {
    atom_matrix_serial_console::setupConsole();
    atom_matrix_serial_console::setupButtonIncrement();
}

void loop() {
    atom_matrix_serial_console::loop();
    atom_matrix_serial_console::pollButtonIncrement();
    atom_matrix_serial_console::pollDemoDisplay();
    atom_matrix_serial_console::pollEnduranceTest();
    atom_matrix_serial_console::pollReconnectTest();
    atom_matrix_serial_console::pollBenchmark();
}
