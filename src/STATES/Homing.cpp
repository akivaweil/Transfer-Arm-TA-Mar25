#include "../include/Homing.h"

#include <AccelStepper.h>
#include <Arduino.h>
#include <Bounce2.h>

#include "../include/Settings.h"
#include "../include/TransferArm.h"
#include "../include/Utils.h"

//* ************************************************************************
//* **************************** HOMING LOGIC ******************************
//* ************************************************************************
// This file implements the homing sequence for the X and Z axes of the Transfer
// Arm. It uses limit switches to define the home positions.

// Main homing sequence that coordinates all axes according to the specified
// sequence
void homeSystem() {
  smartLog("Starting homing sequence...");

  // 1. Home Z axis first
  homeZAxis();

  // 2. Move Z axis up 5 inches
  smartLog("Moving Z-axis up 5 inches from home...");
  transferArm.getZStepper().moveTo(Z_UP_POS);

  // Wait for Z movement to complete
  while (transferArm.getZStepper().distanceToGo() != 0) {
    transferArm.getZStepper().run();
    yield();
  }

  // 3. Home X axis
  homeXAxis();

  // 4. Move X axis to pickup position
  smartLog("Moving X-axis to pickup position...");
  transferArm.getXStepper().moveTo(X_PICKUP_POS);

  // Wait for X movement to complete
  while (transferArm.getXStepper().distanceToGo() != 0) {
    transferArm.getXStepper().run();
    yield();
  }

  smartLog("Homing sequence completed");
}

// Home the Z axis
void homeZAxis() {
  smartLog("Homing Z axis...");

  // Move towards home switch
  transferArm.getZStepper().setSpeed(
      -Z_HOME_SPEED);  // Slow speed in negative direction

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
  smartLog("Initial home switch state: " +
           String(transferArm.getXHomeSwitch().read() ? "HIGH" : "LOW"));

  // Check if X home switch is already activated
  transferArm.getXHomeSwitch().update();
  if (transferArm.getXHomeSwitch().read() == HIGH) {
    smartLog("X home switch already triggered. Setting position as home.");
    transferArm.getXStepper().stop();
    transferArm.getXStepper().setCurrentPosition(X_HOME_POS);

    // Move away from the switch a small amount to prevent future issues
    smartLog("Moving away from the switch slightly...");
    transferArm.getXStepper().setSpeed(
        X_HOME_SPEED);  // Positive direction (away from home)

    // Step until switch is released or max steps reached
    int safetyCounter = 0;
    transferArm.getXHomeSwitch().update();
    while (transferArm.getXHomeSwitch().read() == HIGH && safetyCounter < 200) {
      transferArm.getXStepper().runSpeed();
      transferArm.getXHomeSwitch().update();
      safetyCounter++;
      yield();
    }

    // Stop and set position to a small positive value
    transferArm.getXStepper().stop();
    transferArm.getXStepper().setCurrentPosition(50);  // Small offset from home
    smartLog("Backed off from switch by " + String(safetyCounter) + " steps");

    smartLog("X axis homed");
    return;
  }

  // Move towards home switch
  transferArm.getXStepper().setSpeed(
      -X_HOME_SPEED);  // Slow speed in negative direction

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
  transferArm.getXStepper().setSpeed(
      X_HOME_SPEED);  // Positive direction (away from home)

  // Step until switch is released or max steps reached
  int safetyCounter = 0;
  transferArm.getXHomeSwitch().update();
  while (transferArm.getXHomeSwitch().read() == HIGH && safetyCounter < 200) {
    transferArm.getXStepper().runSpeed();
    transferArm.getXHomeSwitch().update();
    safetyCounter++;
    yield();
  }

  // Stop and set position to a small positive value
  transferArm.getXStepper().stop();
  transferArm.getXStepper().setCurrentPosition(50);  // Small offset from home
  smartLog("Backed off from switch by " + String(safetyCounter) + " steps");

  smartLog("X axis homed");
}