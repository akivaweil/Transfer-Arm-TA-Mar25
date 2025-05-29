#include "../../config/Config.h"
#include "../../config/Pins_Definitions.h"
#include "../../../include/TransferArm.h"
#include "../../../include/Utils.h"

// Forward declarations for functions defined in HOMING_FUNCTIONS.cpp
void homeZAxis();
void homeXAxis();

//* ************************************************************************
//* ************************ HOMING ***************************
//* ************************************************************************
// This file contains the main homing state machine logic for the Transfer Arm.
// The homing sequence coordinates X and Z axes to establish reference positions
// using limit switches.

// Main homing sequence that coordinates all axes according to the specified sequence
void homeSystem() {
  smartLog("Starting homing sequence...");

  //! Step 1: Home Z axis first
  homeZAxis();

  //! Step 2: Move Z axis up 5 inches
  smartLog("Moving Z-axis up 5 inches from home...");
  transferArm.getZStepper().moveTo(Z_UP_POS);

  // Wait for Z movement to complete
  while (transferArm.getZStepper().distanceToGo() != 0) {
    transferArm.getZStepper().run();
    yield();
  }

  //! Step 3: Home X axis
  homeXAxis();

  //! Step 4: Move X axis to pickup position - BLOCKING
  smartLog("Moving X-axis to pickup position...");
  transferArm.getXStepper().moveTo(X_PICKUP_POS);
  transferArm.getXStepper().runToPosition();  // Blocking call
  smartLog("X-axis reached pickup position");

  smartLog("Homing sequence completed");
} 