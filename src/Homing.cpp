#include <Arduino.h>
#include <AccelStepper.h>
#include "../include/Constants.h"
#include "../include/Homing.h"

// External references to stepper motors defined in main file
extern AccelStepper xStepper;
extern AccelStepper zStepper;

// Main homing sequence that coordinates all axes according to the specified sequence
bool homeSystem() {
  Serial.println("Starting homing sequence...");
  
  // 1. Home Z axis first
  if (!homeZAxis()) {
    return false;
  }
  
  // 2. Move Z axis up 5 inches
  Serial.println("Moving Z-axis up 5 inches from home...");
  zStepper.moveTo(Z_UP_POS);
  
  // Wait for Z movement to complete
  while (zStepper.distanceToGo() != 0) {
    zStepper.run();
    yield();
  }
  
  // 3. Home X axis
  if (!homeXAxis()) {
    return false;
  }
  
  // 4. Move X axis to pickup position
  Serial.println("Moving X-axis to pickup position...");
  xStepper.moveTo(X_PICKUP_POS);
  
  // Wait for X movement to complete
  while (xStepper.distanceToGo() != 0) {
    xStepper.run();
    yield();
  }
  
  Serial.println("Homing sequence completed");
  return true;
}

// Home the Z axis
bool homeZAxis() {
  Serial.println("Homing Z axis...");
  
  // Move towards home switch
  zStepper.setSpeed(-500);  // Slow speed in negative direction
  
  unsigned long startTime = millis();
  
  // Keep stepping until home switch is triggered (active HIGH)
  while (digitalRead(Z_HOME_SWITCH_PIN) == LOW) {
    zStepper.runSpeed();
    yield();  // Allow ESP32 to handle background tasks
    
    // Check for timeout
    if (millis() - startTime > HOMING_TIMEOUT) {
      Serial.println("Z-axis homing timeout!");
      return false;
    }
  }
  
  // Stop the motor
  zStepper.stop();
  
  // Set current position as home
  zStepper.setCurrentPosition(Z_HOME_POS);
  
  Serial.println("Z axis homed");
  return true;
}

// Home the X axis
bool homeXAxis() {
  Serial.println("Homing X axis...");
  
  // Move towards home switch
  xStepper.setSpeed(-500);  // Slow speed in negative direction
  
  unsigned long startTime = millis();
  
  // Keep stepping until home switch is triggered (active HIGH)
  while (digitalRead(X_HOME_SWITCH_PIN) == LOW) {
    xStepper.runSpeed();
    yield();  // Allow ESP32 to handle background tasks
    
    // Check for timeout
    if (millis() - startTime > HOMING_TIMEOUT) {
      Serial.println("X-axis homing timeout!");
      return false;
    }
  }
  
  // Stop the motor
  xStepper.stop();
  
  // Set current position as home
  xStepper.setCurrentPosition(X_HOME_POS);
  
  Serial.println("X axis homed");
  return true;
} 