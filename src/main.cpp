#include <Arduino.h>

#include "../include/TransferArm.h"

// Global variable definitions
const char* BOARD_ID = "TRANSFER_ARM_001";
const char* BOARD_DESCRIPTION = "Transfer Arm Controller";

//* ************************************************************************
//* *************************** MAIN PROGRAM *****************************
//* ************************************************************************
// This is the main entry point for the Transfer Arm system.
// It initializes the TransferArm class which handles all hardware,
// state machines, and web server functionality.

// Arduino setup function - runs once at startup
void setup() {
  // Initialize the Transfer Arm system
  transferArm.begin();
}

// Arduino loop function - runs repeatedly
void loop() {
  // Update the Transfer Arm system
  transferArm.update();
}
