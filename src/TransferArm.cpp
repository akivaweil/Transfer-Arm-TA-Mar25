#include <AccelStepper.h>
#include <Arduino.h>
#include <Bounce2.h>
#include <ESP32Servo.h>

// Include our custom headers
#include "../include/Homing.h"
#include "../include/PickCycle.h"
#include "../include/Settings.h"
#include "../include/TransferArm.h"
#include "../include/Utils.h"

//* ************************************************************************
//* ************************ TRANSFER ARM CLASS *************************
//* ************************************************************************
// This file contains the implementation of the TransferArm class that
// encapsulates all hardware initialization, homing, and pick-and-place cycle
// logic.

// Global instance definition
TransferArm transferArm;

// Constructor - Initialize hardware with proper pin configurations
TransferArm::TransferArm()
    : xStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN),
      zStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN) {
  // Hardware instances are initialized in the member initializer list
}

// Main initialization method - replaces the old setup() function
void TransferArm::begin() {
  // Initialize serial communication
  Serial.begin(115200);

  // Configure all hardware components
  configurePins();
  configureDebouncers();
  configureSteppers();
  configureServo();

  // Initialize pick cycle state machine
  initializePickCycle();

  // Home the system (automatic on startup - no user input required)
  homeSystem();

  Serial.println("Transfer Arm Initialized Successfully");
}

// Main update method - replaces the old loop() function
void TransferArm::update() {
  // Update debouncers
  xHomeSwitch.update();
  zHomeSwitch.update();
  startButton.update();
  stage1Signal.update();

  // Update steppers
  xStepper.run();
  zStepper.run();

  // Update the pick cycle state machine
  updatePickCycle();
}

// Configure input and output pins
void TransferArm::configurePins() {
  // Configure input pins
  pinMode(X_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(Z_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(START_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(STAGE1_SIGNAL_PIN, INPUT_PULLDOWN);

  // Configure output pins
  pinMode(SOLENOID_RELAY_PIN, OUTPUT);
  digitalWrite(SOLENOID_RELAY_PIN, LOW);  // Ensure solenoid is off
}

// Configure debouncer objects
void TransferArm::configureDebouncers() {
  xHomeSwitch.attach(X_HOME_SWITCH_PIN);
  xHomeSwitch.interval(2);  // 2ms debounce

  zHomeSwitch.attach(Z_HOME_SWITCH_PIN);
  zHomeSwitch.interval(2);  // 2ms debounce

  startButton.attach(START_BUTTON_PIN);
  startButton.interval(10);  // 10ms debounce

  stage1Signal.attach(STAGE1_SIGNAL_PIN);
  stage1Signal.interval(10);  // 10ms debounce
}

// Configure stepper motor settings
void TransferArm::configureSteppers() {
  xStepper.setMaxSpeed(X_MAX_SPEED);
  xStepper.setAcceleration(X_ACCELERATION);
  xStepper.setMinPulseWidth(3);

  zStepper.setMaxSpeed(Z_MAX_SPEED);
  zStepper.setAcceleration(Z_ACCELERATION);
  zStepper.setMinPulseWidth(3);
}

// Configure servo motor
void TransferArm::configureServo() {
  gripperServo.attach(SERVO_PIN);
  gripperServo.write(SERVO_HOME_POS);
}