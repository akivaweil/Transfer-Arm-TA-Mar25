#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>

// Pin Definitions
// Inputs
const int LIMIT_SWITCH_PIN = 34;     // Limit switch input (active high)
const int STAGE1_SIGNAL_PIN = 35;    // Stage 1 machine signal input
const int X_HOME_SWITCH_PIN = 32;    // X-axis home limit switch
const int Z_HOME_SWITCH_PIN = 33;    // Z-axis home limit switch

// Outputs
const int X_STEP_PIN = 25;           // X-axis stepper motor step pin
const int X_DIR_PIN = 26;            // X-axis stepper motor direction pin
const int Z_STEP_PIN = 27;           // Z-axis stepper motor step pin
const int Z_DIR_PIN = 14;            // Z-axis stepper motor direction pin
const int SERVO_PIN = 12;            // Servo control pin
const int SOLENOID_RELAY_PIN = 13;   // Solenoid relay control pin

// Constants
const int STEPS_PER_REV = 400;       // Steps per revolution for steppers (1.8° with 1/2 microstepping)
const int PULLEY_TEETH = 20;         // Number of teeth on the pulley
const int BELT_PITCH = 2;            // GT2 belt pitch in mm
const float STEPS_PER_MM = STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH); // Steps per mm of linear movement

// Positions
const int X_HOME_POS = 0;            // X-axis home position (in steps)
const int Z_HOME_POS = 0;            // Z-axis home position (in steps)
const int X_PICKUP_POS = 500;        // X-axis pickup position (in steps) - adjust as needed
const int X_PLACE_POS = 1500;        // X-axis place position (in steps) - adjust as needed
const int Z_UP_POS = 0;              // Z-axis up position (in steps)
const int Z_DOWN_POS = 800;          // Z-axis down position (in steps) - adjust as needed

const int SERVO_HOME_POS = 90;       // Servo home position (in degrees)
const int SERVO_PICKUP_POS = 0;      // Servo pickup position (in degrees) - adjust as needed
const int SERVO_PLACE_POS = 180;     // Servo place position (in degrees) - adjust as needed

// Stepper settings
const int MAX_SPEED = 2000;          // Maximum speed in steps per second
const int ACCELERATION = 1000;       // Acceleration in steps per second^2

// Create instances
AccelStepper xStepper(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper zStepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
Servo gripperServo;

// Function prototypes
void homeXAxis();
void homeZAxis();
bool moveToPosition(AccelStepper &stepper, long position);
bool Wait(unsigned long delayTime, unsigned long* startTimePtr);

// State variables
enum PickCycleState {
  WAITING,
  MOVE_TO_PICKUP,
  ACTIVATE_VACUUM,
  LIFT_OBJECT,
  MOVE_TO_PLACE,
  LOWER_OBJECT,
  RELEASE_OBJECT,
  RETURN_HOME
};

PickCycleState currentState = WAITING;
unsigned long stateTimer = 0;

// Setup function - runs once at startup
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Configure input pins
  pinMode(LIMIT_SWITCH_PIN, INPUT);
  pinMode(STAGE1_SIGNAL_PIN, INPUT);
  pinMode(X_HOME_SWITCH_PIN, INPUT_PULLUP);  // Typically active LOW
  pinMode(Z_HOME_SWITCH_PIN, INPUT_PULLUP);  // Typically active LOW
  
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
  
  // Home the axes
  homeXAxis();
  homeZAxis();
  
  Serial.println("Transfer Arm Initialized");
}

// Main loop function - runs repeatedly
void loop() {
  // Update steppers
  xStepper.run();
  zStepper.run();
  
  // State machine for pick cycle
  switch (currentState) {
    case WAITING:
      // Check for pick cycle trigger
      if (digitalRead(LIMIT_SWITCH_PIN) == HIGH || digitalRead(STAGE1_SIGNAL_PIN) == HIGH) {
        Serial.println("Pick Cycle Triggered");
        currentState = MOVE_TO_PICKUP;
        stateTimer = 0;
      }
      break;
      
    case MOVE_TO_PICKUP:
      // TO BE IMPLEMENTED - Move to pickup position
      break;
      
    case ACTIVATE_VACUUM:
      // TO BE IMPLEMENTED - Activate vacuum
      break;
      
    case LIFT_OBJECT:
      // TO BE IMPLEMENTED - Lift object
      break;
      
    case MOVE_TO_PLACE:
      // TO BE IMPLEMENTED - Move to place position
      break;
      
    case LOWER_OBJECT:
      // TO BE IMPLEMENTED - Lower object
      break;
      
    case RELEASE_OBJECT:
      // TO BE IMPLEMENTED - Release object
      break;
      
    case RETURN_HOME:
      // TO BE IMPLEMENTED - Return to home position
      break;
  }
}

// Home the X axis
void homeXAxis() {
  Serial.println("Homing X axis...");
  
  // Move towards home switch
  xStepper.setSpeed(-500);  // Slow speed in negative direction
  
  // Keep stepping until home switch is triggered
  while (digitalRead(X_HOME_SWITCH_PIN) == HIGH) {  // Assuming active LOW
    xStepper.runSpeed();
    yield();  // Allow ESP32 to handle background tasks
  }
  
  // Stop the motor
  xStepper.stop();
  
  // Set current position as home
  xStepper.setCurrentPosition(X_HOME_POS);
  
  Serial.println("X axis homed");
}

// Home the Z axis
void homeZAxis() {
  Serial.println("Homing Z axis...");
  
  // Move towards home switch
  zStepper.setSpeed(-500);  // Slow speed in negative direction
  
  // Keep stepping until home switch is triggered
  while (digitalRead(Z_HOME_SWITCH_PIN) == HIGH) {  // Assuming active LOW
    zStepper.runSpeed();
    yield();  // Allow ESP32 to handle background tasks
  }
  
  // Stop the motor
  zStepper.stop();
  
  // Set current position as home
  zStepper.setCurrentPosition(Z_HOME_POS);
  
  Serial.println("Z axis homed");
}

// Move a stepper to a position and wait for completion
bool moveToPosition(AccelStepper &stepper, long position) {
  // Set the target position
  stepper.moveTo(position);
  
  // Check if we're already at position
  return stepper.distanceToGo() == 0;
}

// Non-blocking delay function
bool Wait(unsigned long delayTime, unsigned long* startTimePtr) {
  // First time entering this function
  if (*startTimePtr == 0) {
    *startTimePtr = millis();
    return false;
  }
  
  // Check if the delay time has elapsed
  if (millis() - *startTimePtr >= delayTime) {
    *startTimePtr = 0;  // Reset for next use
    return true;
  }
  
  return false;
}
