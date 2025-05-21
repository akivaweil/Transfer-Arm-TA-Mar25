/*
Instructional Guideline for PickCycle States:

WAITING:
  - The system is idle, awaiting a trigger to start the pick cycle.
  - Trigger can be the limit switch (LIMIT_SWITCH_PIN) or a signal from Stage 1 (STAGE1_SIGNAL_PIN).

MOVE_TO_PICKUP:
  - The X-axis stepper motor moves the arm to the designated pickup position (X_PICKUP_POS).
  - Once at the position, the gripper servo is set to the pickup angle (SERVO_PICKUP_POS).

LOWER_Z_FOR_PICKUP:
  - The Z-axis stepper motor lowers the arm towards the object to be picked up.
  - Initially lowers to the Z_SUCTION_START_POS.

ACTIVATE_VACUUM:
  - The vacuum solenoid (SOLENOID_RELAY_PIN) is activated to create suction.

CONTINUE_LOWERING_Z:
  - The Z-axis continues to lower to the full pickup depth (Z_PICKUP_POS).

WAIT_AT_PICKUP:
  - The arm pauses at the pickup position for a defined duration (PICKUP_HOLD_TIME) to ensure a secure grip.

RAISE_Z_WITH_OBJECT:
  - The Z-axis stepper motor raises the arm, now holding the object, to the safe Z up position (Z_UP_POS).

MOVE_TO_DROPOFF:
  - The X-axis stepper motor moves the arm towards the designated drop-off position (X_DROPOFF_POS).
  - During this movement, if the X-axis passes X_MIDPOINT_POS, the gripper servo rotates to SERVO_DROPOFF_POS.

ROTATE_SERVO_MIDPOINT:
  - This state is conceptual and handled within the MOVE_TO_DROPOFF logic.
  - It signifies the point where the servo should rotate while the X-axis is in transit.

LOWER_Z_FOR_DROPOFF:
  - The Z-axis stepper motor lowers the arm to the drop-off height (Z_DROPOFF_POS).

RELEASE_OBJECT:
  - The vacuum solenoid is deactivated, releasing the object.

WAIT_AFTER_RELEASE:
  - The arm pauses briefly at the drop-off position (DROPOFF_HOLD_TIME) after releasing the object.

RAISE_Z_AFTER_DROPOFF:
  - The Z-axis stepper motor raises the arm back to the safe Z up position (Z_UP_POS).

SIGNAL_STAGE2:
  - A signal is sent to the Stage 2 machine (via STAGE2_SIGNAL_PIN) to indicate the completion of the drop-off.
  - The signal is a brief HIGH pulse.

RETURN_TO_PICKUP:
  - The X-axis stepper motor moves the arm back to the pickup position (X_PICKUP_POS).
  - The gripper servo is reset to the pickup angle (SERVO_PICKUP_POS).
  - The cycle is complete, and the system returns to the WAITING state.
*/
#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include "../include/Constants.h"
#include "../include/PickCycle.h"
#include "../include/Utils.h"

// External references 
extern AccelStepper xStepper;
extern AccelStepper zStepper;
extern Servo gripperServo;

// State variables
PickCycleState currentState = WAITING;
unsigned long stateTimer = 0;
bool midpointServoRotated = false;  // Track if servo has been rotated at midpoint

// Initialize the pick cycle state machine
void initializePickCycle() {
  currentState = WAITING;
  stateTimer = 0;
  midpointServoRotated = false;
  
  // Configure Stage 2 signal pin as output
  pinMode(STAGE2_SIGNAL_PIN, OUTPUT);
  digitalWrite(STAGE2_SIGNAL_PIN, LOW);  // Initialize as LOW
}

// Get the current state
PickCycleState getCurrentState() {
  return currentState;
}

// Update the pick cycle state machine
void updatePickCycle() {
  // Check for X-axis reaching midpoint during travel to dropoff
  if (currentState == MOVE_TO_DROPOFF && !midpointServoRotated) {
    if (xStepper.currentPosition() >= X_MIDPOINT_POS) {
      gripperServo.write(SERVO_DROPOFF_POS);
      midpointServoRotated = true;
    }
  }

  // State machine for pick cycle
  switch (currentState) {
    case WAITING:
      // Check for pick cycle trigger
      if (digitalRead(START_BUTTON_PIN) == HIGH || digitalRead(STAGE1_SIGNAL_PIN) == HIGH) {
        Serial.println("Pick Cycle Triggered");
        currentState = MOVE_TO_PICKUP;
        stateTimer = 0;
      }
      break;
      
    case MOVE_TO_PICKUP:
      // Ensure we're at the pickup position on X-axis
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        Serial.println("At pickup position, preparing to lower Z-axis");
        gripperServo.write(SERVO_PICKUP_POS);  // Set servo to pickup position
        currentState = LOWER_Z_FOR_PICKUP;
      }
      break;
      
    case LOWER_Z_FOR_PICKUP:
      // Lower Z axis for pickup, initially to suction start position
      zStepper.moveTo(Z_SUCTION_START_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z at suction start position, activating vacuum");
        currentState = ACTIVATE_VACUUM;
      }
      break;
      
    case ACTIVATE_VACUUM:
      // Turn on the vacuum solenoid
      digitalWrite(SOLENOID_RELAY_PIN, HIGH);
      Serial.println("Vacuum activated, continuing to lower Z-axis");
      currentState = CONTINUE_LOWERING_Z;
      break;
      
    case CONTINUE_LOWERING_Z:
      // Continue lowering to full pickup position
      zStepper.moveTo(Z_PICKUP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z fully lowered for pickup, waiting");
        stateTimer = 0;
        currentState = WAIT_AT_PICKUP;
      }
      break;
      
    case WAIT_AT_PICKUP:
      // Wait for hold time at pickup position
      if (Wait(PICKUP_HOLD_TIME, &stateTimer)) {
        Serial.println("Pickup wait complete, raising Z-axis with object");
        currentState = RAISE_Z_WITH_OBJECT;
      }
      break;
      
    case RAISE_Z_WITH_OBJECT:
      // Raise Z axis with object
      zStepper.moveTo(Z_UP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis raised, moving to dropoff position");
        midpointServoRotated = false;  // Reset midpoint flag
        currentState = MOVE_TO_DROPOFF;
      }
      break;
      
    case MOVE_TO_DROPOFF:
      // Move X axis to dropoff position
      // Note: Servo rotation at midpoint is handled in the main loop
      if (moveToPosition(xStepper, X_DROPOFF_POS)) {
        Serial.println("At dropoff position, lowering Z-axis");
        currentState = LOWER_Z_FOR_DROPOFF;
      }
      break;
      
    case ROTATE_SERVO_MIDPOINT:
      // This state is handled automatically in the main loop
      // It's included here for clarity in the state machine
      break;
      
    case LOWER_Z_FOR_DROPOFF:
      // Lower Z axis for dropoff
      zStepper.moveTo(Z_DROPOFF_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis lowered for dropoff, releasing object");
        currentState = RELEASE_OBJECT;
      }
      break;
      
    case RELEASE_OBJECT:
      // Turn off the vacuum solenoid
      digitalWrite(SOLENOID_RELAY_PIN, LOW);
      Serial.println("Object released, waiting briefly");
      stateTimer = 0;
      currentState = WAIT_AFTER_RELEASE;
      break;
      
    case WAIT_AFTER_RELEASE:
      // Wait briefly after release
      if (Wait(DROPOFF_HOLD_TIME, &stateTimer)) {
        Serial.println("Wait complete, raising Z-axis");
        currentState = RAISE_Z_AFTER_DROPOFF;
      }
      break;
      
    case RAISE_Z_AFTER_DROPOFF:
      // Raise Z axis after dropoff
      zStepper.moveTo(Z_UP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis raised, signaling Stage 2");
        currentState = SIGNAL_STAGE2;
      }
      break;
      
    case SIGNAL_STAGE2:
      // Send signal to Stage 2 machine
      digitalWrite(STAGE2_SIGNAL_PIN, HIGH);
      delay(100);  // Brief pulse
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);
      Serial.println("Stage 2 signaled, returning to pickup position");
      currentState = RETURN_TO_PICKUP;
      break;
      
    case RETURN_TO_PICKUP:
      // Return to pickup position to prepare for next cycle
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        gripperServo.write(SERVO_PICKUP_POS);  // Reset servo to pickup position
        Serial.println("Returned to pickup position, cycle complete");
        currentState = WAITING;
      }
      break;
  }
} 