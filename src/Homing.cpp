#include <Arduino.h>
#include <AccelStepper.h>
#include <Bounce2.h>
#include "../include/Constants.h"
#include "../include/Homing.h"

//* ************************************************************************
//* **************************** HOMING LOGIC ******************************
//* ************************************************************************
// This file implements the homing sequence for the X and Z axes of the Transfer Arm.
// It uses limit switches to define the home positions.

// External references to stepper motors defined in main file
extern AccelStepper xStepper;
extern AccelStepper zStepper;

// Main homing sequence that coordinates all axes according to the specified sequence
void homeSystem() {
  Serial.println("Starting homing sequence...");
  
  // 1. Home Z axis first
  homeZAxis();
  
  // 2. Move Z axis up 5 inches
  Serial.println("Moving Z-axis up 5 inches from home...");
  zStepper.moveTo(Z_UP_POS);
  
  // Wait for Z movement to complete
  while (zStepper.distanceToGo() != 0) {
    zStepper.run();
    yield();
  }
  
  // 3. Home X axis
  homeXAxis();
  
  // 4. Move X axis to pickup position
  Serial.println("Moving X-axis to pickup position...");
  xStepper.moveTo(X_PICKUP_POS);
  
  // Wait for X movement to complete
  while (xStepper.distanceToGo() != 0) {
    xStepper.run();
    yield();
  }
  
  Serial.println("Homing sequence completed");
}

// Home the Z axis
void homeZAxis() {
  Serial.println("Homing Z axis...");
  
  // Move towards home switch
  zStepper.setSpeed(-500);  // Slow speed in negative direction
  
  // Keep stepping until home switch is triggered (active HIGH)
  while (zHomeSwitch.read() == LOW) {
    zStepper.runSpeed();
    zHomeSwitch.update();
    yield();  // Allow ESP32 to handle background tasks
  }
  
  // Stop the motor
  zStepper.stop();
  
  // Set current position as home
  zStepper.setCurrentPosition(Z_HOME_POS);
  
  Serial.println("Z axis homed");
}

// Home the X axis
void homeXAxis() {
  Serial.println("Homing X axis...");
  
  // Move towards home switch
  xStepper.setSpeed(-500);  // Slow speed in negative direction
  
  // Keep stepping until home switch is triggered (active HIGH)
  while (xHomeSwitch.read() == LOW) {
    xStepper.runSpeed();
    xHomeSwitch.update();
    yield();  // Allow ESP32 to handle background tasks
  }
  
  // Stop the motor
  xStepper.stop();
  
  // Set current position as home
  xStepper.setCurrentPosition(X_HOME_POS);
  
  Serial.println("X axis homed");
} 