#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>

// Include our custom headers
#include "../include/Settings.h"
#include "../include/Homing.h"
#include "../include/Utils.h"
#include "../include/PickCycle.h"

//* ************************************************************************
//* ************************ MAIN CONTROL LOGIC ****************************
//* ************************************************************************
// This file contains the main setup and loop functions for the Transfer Arm.
// It initializes hardware components, steppers, servo, and the pick-and-place cycle.

// Create global instances
AccelStepper xStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper zStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
Servo gripperServo;

// Create Bounce objects for debouncing
Bounce xHomeSwitch = Bounce();
Bounce zHomeSwitch = Bounce();
Bounce startButton = Bounce();
Bounce stage1Signal = Bounce();

// Setup function - runs once at startup
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Configure input pins with debouncers
  pinMode(X_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  xHomeSwitch.attach(X_HOME_SWITCH_PIN);
  xHomeSwitch.interval(2); // 2ms debounce

  pinMode(Z_HOME_SWITCH_PIN, INPUT_PULLDOWN);
  zHomeSwitch.attach(Z_HOME_SWITCH_PIN);
  zHomeSwitch.interval(2); // 2ms debounce

  pinMode(START_BUTTON_PIN, INPUT_PULLDOWN);
  startButton.attach(START_BUTTON_PIN);
  startButton.interval(10); // 10ms debounce

  pinMode(STAGE1_SIGNAL_PIN, INPUT_PULLDOWN);
  stage1Signal.attach(STAGE1_SIGNAL_PIN);
  stage1Signal.interval(10); // 10ms debounce
  
  // Configure output pins
  pinMode(SOLENOID_RELAY_PIN, OUTPUT);
  digitalWrite(SOLENOID_RELAY_PIN, LOW);     // Ensure solenoid is off
  
  pinMode(X_ENABLE_PIN, OUTPUT);
  digitalWrite(X_ENABLE_PIN, HIGH);          // Start with X-axis motor disabled (active low enable)
  
  // Configure stepper motors
  xStepper.setMaxSpeed(X_MAX_SPEED);
  xStepper.setAcceleration(X_ACCELERATION);
  xStepper.setMinPulseWidth(3);
  zStepper.setMaxSpeed(Z_MAX_SPEED);
  zStepper.setAcceleration(Z_ACCELERATION);
  zStepper.setMinPulseWidth(3);
  
  // Configure servo
  gripperServo.attach(SERVO_PIN);
  gripperServo.write(SERVO_HOME_POS);
  
  // Initialize pick cycle state machine
  initializePickCycle();
  
  // Home the system (automatic on startup - no user input required)
  homeSystem();
  
  Serial.println("Transfer Arm Initialized Successfully");
}

// Main loop function - runs repeatedly
void loop() {
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