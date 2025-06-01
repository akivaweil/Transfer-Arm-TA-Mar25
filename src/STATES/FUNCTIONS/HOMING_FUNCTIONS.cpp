#include "../../Config/Config.h"
#include "../../Config/Pins_Definitions.h"
#include "../../../include/TransferArm.h"
#include "../../../include/Utils.h"
#include <AccelStepper.h>
#include <Arduino.h>
#include <Bounce2.h>

//* ************************************************************************
//* ************************ HOMING FUNCTIONS ***************************
//* ************************************************************************
// This file contains all the individual functions needed for the homing sequence.
// Each axis has its own dedicated homing function with proper limit switch handling.

// Home the Z axis
void homeZAxis() {
  smartLog("Homing Z axis...");

  // Move towards home switch
  transferArm.getZStepper().setSpeed(-Z_HOME_SPEED);  // Slow speed in negative direction

  // Keep stepping until home switch is triggered (active HIGH)
  while (transferArm.getZHomeSwitch().read() == LOW) {
    transferArm.getZStepper().runSpeed();
    transferArm.getZHomeSwitch().update();
    yield();  // Allow ESP32 to handle background tasks
  }

  // Stop the motor
  transferArm.getZStepper().stop();

  // Set current position as home
  transferArm.getZStepper().setCurrentPosition(Z_HOME_POS);

  smartLog("Z axis homed");
}

// Home the X axis  
void homeXAxis() {
  smartLog("Homing X axis...");
  smartLog("Initial home switch state: " + String(transferArm.getXHomeSwitch().read() ? "HIGH" : "LOW"));

  // Check if X home switch is already activated
  transferArm.getXHomeSwitch().update();
  if (transferArm.getXHomeSwitch().read() == HIGH) {
    smartLog("X home switch already triggered. Setting position as home.");
    transferArm.getXStepper().stop();
    transferArm.getXStepper().setCurrentPosition(X_HOME_POS);

    // Move away from the switch a small amount to prevent future issues
    smartLog("Moving away from the switch slightly...");
    transferArm.getXStepper().setSpeed(X_HOME_SPEED);  // Positive direction (away from home)

    // Step until switch is released or max steps reached
    float safetyCounter = 0.0;
    transferArm.getXHomeSwitch().update();
    while (transferArm.getXHomeSwitch().read() == HIGH && safetyCounter < 200.0) {
      transferArm.getXStepper().runSpeed();
      transferArm.getXHomeSwitch().update();
      safetyCounter += 1.0;
      yield();
    }

    // Stop and set position to a small positive value
    transferArm.getXStepper().stop();
    transferArm.getXStepper().setCurrentPosition(50.0);  // Small offset from home
    smartLog("Backed off from switch by " + String(safetyCounter) + " steps");

    smartLog("X axis homed");
    return;
  }

  // Move towards home switch
  transferArm.getXStepper().setSpeed(-X_HOME_SPEED);  // Slow speed in negative direction

  // Keep stepping until home switch is triggered (active HIGH)
  while (transferArm.getXHomeSwitch().read() == LOW) {
    transferArm.getXStepper().runSpeed();
    transferArm.getXHomeSwitch().update();
    yield();  // Allow ESP32 to handle background tasks
  }

  // Stop the motor
  transferArm.getXStepper().stop();

  // Set current position as home
  transferArm.getXStepper().setCurrentPosition(X_HOME_POS);

  // Move away from the switch a small amount
  smartLog("Moving away from the switch slightly...");
  transferArm.getXStepper().setSpeed(X_HOME_SPEED);  // Positive direction (away from home)

  // Step until switch is released or max steps reached
  float safetyCounter = 0.0;
  transferArm.getXHomeSwitch().update();
  while (transferArm.getXHomeSwitch().read() == HIGH && safetyCounter < 200.0) {
    transferArm.getXStepper().runSpeed();
    transferArm.getXHomeSwitch().update();
    safetyCounter += 1.0;
    yield();
  }

  // Stop and set position to a small positive value
  transferArm.getXStepper().stop();
  transferArm.getXStepper().setCurrentPosition(50.0);  // Small offset from home
  smartLog("Backed off from switch by " + String(safetyCounter) + " steps");

  smartLog("X axis homed");
}