#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>
#include "../include/Settings.h"
#include "../include/Homing.h"
#include "../include/Utils.h"

//* ************************************************************************
//* **************************** HOMING LOGIC ******************************
//* ************************************************************************
// This file implements the homing sequence for the X and Z axes of the Transfer Arm.
// It uses limit switches to define the home positions.

// External references to stepper motors defined in main file
extern AccelStepper xStepper;
extern AccelStepper zStepper;

// External reference to servo defined in main file
extern Servo gripperServo;

// External references to bounce objects defined in main file
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;

// Main homing sequence that coordinates all axes according to the specified sequence
void homeSystem() {
  Serial.println("Starting homing sequence...");
  enableXMotor();  // Enable X-axis motor for homing
  
  // 1. Home Z axis first
  homeZAxis();
  
  // 2. Move Z axis up to fully retracted position
  Serial.println("Moving Z-axis up to fully retracted position...");
  zStepper.moveTo(Z_UP_POS);
  
  // Wait for Z movement to complete
  while (zStepper.distanceToGo() != 0) {
    zStepper.run();
    yield();
  }
  
  // 3. Home X axis
  homeXAxis();
  
  // 4. Move X axis to waiting position (pickup + 3 inches)
  Serial.println("Moving X-axis to waiting position (pickup + 3 inches)...");
  xStepper.moveTo(X_WAIT_POS);
  
  // Wait for X movement to complete
  while (xStepper.distanceToGo() != 0) {
    xStepper.run();
    yield();
  }
  
  // 5. Move Z axis to pickup height for waiting
  Serial.println("Moving Z-axis to pickup height for waiting...");
  zStepper.moveTo(Z_PICKUP_POS);
  
  // Wait for Z movement to complete
  while (zStepper.distanceToGo() != 0) {
    zStepper.run();
    yield();
  }
  
  // 6. Set servo to pickup position for waiting
  Serial.println("Setting servo to pickup position...");
  gripperServo.write(SERVO_PICKUP_POS);
  
  disableXMotor();  // Disable X-axis motor after homing
  Serial.println("Homing sequence completed - positioned at waiting location with servo ready");
}

// Home the Z axis
void homeZAxis() {
  Serial.println("Homing Z axis...");
  
  // Move towards home switch
  zStepper.setSpeed(-Z_HOME_SPEED);  // Slow speed in negative direction
  
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
  
  // Restore normal Z-axis speed and acceleration after homing
  zStepper.setMaxSpeed(Z_MAX_SPEED);
  zStepper.setAcceleration(Z_ACCELERATION);
  
  Serial.println("Z axis homed");
}

// Home the X axis
void homeXAxis() {
  Serial.println("Homing X axis...");
  enableXMotor();  // Enable X-axis motor for homing
  Serial.print("Initial home switch state: ");
  Serial.println(xHomeSwitch.read() ? "HIGH" : "LOW");
  
  // Check if X home switch is already activated
  xHomeSwitch.update();
  if (xHomeSwitch.read() == HIGH) {
    Serial.println("X home switch already triggered. Setting position as home.");
    xStepper.stop();
    xStepper.setCurrentPosition(X_HOME_POS);
    
    // Move away from the switch a small amount to prevent future issues
    Serial.println("Moving away from the switch slightly...");
    xStepper.setSpeed(X_HOME_SPEED);  // Positive direction (away from home)
    
    // Step until switch is released or max steps reached
    int safetyCounter = 0;
    xHomeSwitch.update();
    while (xHomeSwitch.read() == HIGH && safetyCounter < 200) {
      xStepper.runSpeed();
      xHomeSwitch.update();
      safetyCounter++;
      yield();
    }
    
    // Stop and set position to a small positive value
    xStepper.stop();
    xStepper.setCurrentPosition(50); // Small offset from home
    
    // Restore normal X-axis speed and acceleration after homing
    xStepper.setMaxSpeed(X_MAX_SPEED);
    xStepper.setAcceleration(X_ACCELERATION);
    
    Serial.print("Backed off from switch by ");
    Serial.print(safetyCounter);
    Serial.println(" steps");
    
    Serial.println("X axis homed");
    return;
  }
  
  // Move towards home switch
  xStepper.setSpeed(-X_HOME_SPEED);  // Slow speed in negative direction
  
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
  
  // Move away from the switch a small amount
  Serial.println("Moving away from the switch slightly...");
  xStepper.setSpeed(X_HOME_SPEED);  // Positive direction (away from home)
  
  // Step until switch is released or max steps reached
  int safetyCounter = 0;
  xHomeSwitch.update();
  while (xHomeSwitch.read() == HIGH && safetyCounter < 200) {
    xStepper.runSpeed();
    xHomeSwitch.update();
    safetyCounter++;
    yield();
  }
  
  // Stop and set position to a small positive value
  xStepper.stop();
  xStepper.setCurrentPosition(50); // Small offset from home
  
  // Restore normal X-axis speed and acceleration after homing
  xStepper.setMaxSpeed(X_MAX_SPEED);
  xStepper.setAcceleration(X_ACCELERATION);
  
  Serial.print("Backed off from switch by ");
  Serial.print(safetyCounter);
  Serial.println(" steps");
  
  Serial.println("X axis homed");
} 