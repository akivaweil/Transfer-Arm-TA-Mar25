#include <Arduino.h>

#include "../include/TransferArm.h"

//* ************************************************************************
//* *************************** MAIN ENTRY POINT ***********************
//* ************************************************************************
// This file contains the main Arduino setup() and loop() functions.
// It initializes and updates the Transfer Arm system using the TransferArm
// class.

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
