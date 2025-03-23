#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>

// Include our custom headers
#include "../include/Constants.h"
#include "../include/Homing.h"
#include "../include/Utils.h"
#include "../include/PickCycle.h"

// Create global instances
AccelStepper xStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper zStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
Servo gripperServo;

// Setup function - runs once at startup
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Configure input pins
  pinMode(LIMIT_SWITCH_PIN, INPUT);
  pinMode(STAGE1_SIGNAL_PIN, INPUT);
  pinMode(X_HOME_SWITCH_PIN, INPUT);  // Active HIGH switch
  pinMode(Z_HOME_SWITCH_PIN, INPUT);  // Active HIGH switch
  
  // Configure output pins
  pinMode(SOLENOID_RELAY_PIN, OUTPUT);
  digitalWrite(SOLENOID_RELAY_PIN, LOW);     // Ensure solenoid is off
  
  // Configure stepper motors
  xStepper.setMaxSpeed(MAX_SPEED);
  xStepper.setAcceleration(ACCELERATION);
  zStepper.setMaxSpeed(MAX_SPEED);
  zStepper.setAcceleration(ACCELERATION);
  
  // Configure servo
  gripperServo.attach(SERVO_PIN);
  gripperServo.write(SERVO_HOME_POS);
  
  // Initialize pick cycle state machine
  initializePickCycle();
  
  // Home the system (automatic on startup - no user input required)
  bool homingSuccess = homeSystem();
  
  if (homingSuccess) {
    Serial.println("Transfer Arm Initialized Successfully");
  } else {
    Serial.println("Homing Failed - Check switches and restart");
    while(1) { 
      // Error state - halt operation
      delay(1000);
    }
  }
}

// Main loop function - runs repeatedly
void loop() {
  // Update steppers
  xStepper.run();
  zStepper.run();
  
  // Update the pick cycle state machine
  updatePickCycle();
} 