#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>
#include "../include/Settings.h"
#include "../include/PickCycle.h"
#include "../include/Utils.h"
#include "../include/Homing.h"

/*
Pick and Place Cycle Steps (Values refer to constants in Constants.h):
1. Move to Pickup Position: X-axis to X_PICKUP_POS, Servo to SERVO_PICKUP_POS.
2. Lower Z-axis for Pickup (Continuous, Activate Vacuum during descent):
  - Z-axis moves towards Z_PICKUP_POS.
  - Vacuum (SOLENOID_RELAY_PIN HIGH) activates when Z passes Z_SUCTION_START_POS.
3. Wait at Pickup: Hold for PICKUP_HOLD_TIME ms.
4. Raise Z-axis with Object: Z-axis to Z_UP_POS.
5. Move to Dropoff Position (Rotate servo concurrently):
  - X-axis to X_DROPOFF_POS.
  - Servo to SERVO_DROPOFF_POS when X-axis passes X_MIDPOINT_POS.
6. Lower Z-axis for Dropoff: Z-axis to Z_DROPOFF_POS.
7. Release Object: Vacuum (SOLENOID_RELAY_PIN LOW).
8. Wait After Release: Hold for DROPOFF_HOLD_TIME ms.
9. Raise Z-axis After Dropoff: Z-axis to Z_UP_POS.
10. Signal Stage 2: Pulse STAGE2_SIGNAL_PIN HIGH then LOW.
11. Return to Pickup Position (X-axis to X_PICKUP_POS, Servo to SERVO_PICKUP_POS).
12. Home X-axis.
13. Final Move to Pickup Position (X-axis to X_PICKUP_POS). Cycle ends, enters WAITING state.
*/
//* ************************************************************************
//* ************************* PICK CYCLE LOGIC ***************************
//* ************************************************************************
// This file implements the state machine for the pick-and-place cycle of the Transfer Arm.
// It controls the sequence of movements, servo operations, and vacuum activation/deactivation.

// External references 
extern AccelStepper xStepper;
extern AccelStepper zStepper;
extern Servo gripperServo;

// State variables
PickCycleState currentState = WAITING;
unsigned long stateTimer = 0;
bool midpointServoRotated = false;  // Track if servo has been rotated at midpoint
bool vacuumActivatedDuringDescent = false; // Track if vacuum activated during Z descent for pickup

// Initialize the pick cycle state machine
void initializePickCycle() {
  currentState = WAITING;
  stateTimer = 0;
  midpointServoRotated = false;
  vacuumActivatedDuringDescent = false;
  
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
  // Check for X-axis reaching servo rotation position during travel to dropoff
  if (currentState == MOVE_TO_DROPOFF && !midpointServoRotated) {
    if (xStepper.currentPosition() >= X_SERVO_ROTATE_POS) {
      gripperServo.write(SERVO_DROPOFF_POS);
      midpointServoRotated = true;
    }
  }

  // State machine for pick cycle
  switch (currentState) {
    case WAITING:
      // Check for pick cycle trigger
      if (startButton.read() == HIGH || stage1Signal.read() == HIGH) {
        Serial.println("Pick Cycle Triggered");
        //! Step 1: Move to Pickup Position
        currentState = MOVE_TO_PICKUP;
        stateTimer = 0;
      }
      break;
      
    case MOVE_TO_PICKUP:
      // Ensure we're at the pickup position on X-axis
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        Serial.println("At X pickup position. Lowering Z to pickup.");
        Serial.print("Target Z: "); Serial.print(Z_PICKUP_POS);
        Serial.print(", Suction Start Z: "); Serial.println(Z_SUCTION_START_POS);
        gripperServo.write(SERVO_PICKUP_POS);  // Set servo to pickup position
        vacuumActivatedDuringDescent = false; // Reset flag
        zStepper.moveTo(Z_PICKUP_POS);      // Command Z to move to final pickup position
        //! Step 2: Lower Z-axis for Pickup & Activate Vacuum
        currentState = LOWER_Z_FOR_PICKUP;
      }
      break;
      
    case LOWER_Z_FOR_PICKUP:
      // Lower Z axis for pickup, activating vacuum mid-way
      // This state is entered once zStepper.moveTo(Z_PICKUP_POS) has been called.

      if (!vacuumActivatedDuringDescent && zStepper.currentPosition() >= Z_SUCTION_START_POS) {
        digitalWrite(SOLENOID_RELAY_PIN, HIGH);
        vacuumActivatedDuringDescent = true;
        Serial.print("Vacuum activated during descent at Z: "); Serial.println(zStepper.currentPosition());
        //! Step 3: Activate Vacuum (during descent)
      }

      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z fully lowered for pickup, waiting");
        stateTimer = 0; // Reset timer for the wait state
        //! Step 4: Wait at Pickup
        currentState = WAIT_AT_PICKUP;
      }
      break;
      
    case WAIT_AT_PICKUP:
      // Wait for hold time at pickup position
      if (Wait(PICKUP_HOLD_TIME, &stateTimer)) {
        Serial.println("Pickup wait complete, raising Z-axis with object");
        //! Step 5: Raise Z-axis with Object
        currentState = RAISE_Z_WITH_OBJECT;
      }
      break;
      
    case RAISE_Z_WITH_OBJECT:
      // Raise Z axis with object
      zStepper.moveTo(Z_UP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis raised, moving to dropoff position");
        midpointServoRotated = false;  // Reset servo rotation flag
        //! Step 6: Move to Dropoff Position
        currentState = MOVE_TO_DROPOFF;
      }
      break;
      
    case MOVE_TO_DROPOFF:
      // Move X axis to dropoff position
      // Note: Servo rotation at midpoint is handled in the main loop
      if (moveToPosition(xStepper, X_DROPOFF_POS)) {
        Serial.println("At dropoff position, lowering Z-axis");
        //! Step 7: Lower Z-axis for Dropoff
        currentState = LOWER_Z_FOR_DROPOFF;
      }
      break;
      
    case ROTATE_SERVO_MIDPOINT:
      // This state is handled automatically in the main loop
      // It's included here for clarity in the state machine
      //! (Handled concurrently with MOVE_TO_DROPOFF)
      break;
      
    case LOWER_Z_FOR_DROPOFF:
      // Lower Z axis for dropoff
      zStepper.moveTo(Z_DROPOFF_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis lowered for dropoff, releasing object");
        //! Step 8: Release Object
        currentState = RELEASE_OBJECT;
      }
      break;
      
    case RELEASE_OBJECT:
      // Turn off the vacuum solenoid
      digitalWrite(SOLENOID_RELAY_PIN, LOW);
      Serial.println("Object released, waiting briefly");
      stateTimer = 0;
      //! Step 9: Wait After Release
      currentState = WAIT_AFTER_RELEASE;
      break;
      
    case WAIT_AFTER_RELEASE:
      // Wait briefly after release
      if (Wait(DROPOFF_HOLD_TIME, &stateTimer)) {
        Serial.println("Wait complete, raising Z-axis");
        //! Step 10: Raise Z-axis After Dropoff
        currentState = RAISE_Z_AFTER_DROPOFF;
      }
      break;
      
    case RAISE_Z_AFTER_DROPOFF:
      // Raise Z axis after dropoff
      zStepper.moveTo(Z_UP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis raised, signaling Stage 2");
        //! Step 11: Signal Stage 2
        currentState = SIGNAL_STAGE2;
      }
      break;
      
    case SIGNAL_STAGE2:
      // Send signal to Stage 2 machine
      digitalWrite(STAGE2_SIGNAL_PIN, HIGH);
      delay(100);  // Brief pulse
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);
      Serial.println("Stage 2 signaled, returning to pickup position (pre-homing)");
      //! Step 11: Return to Pickup Position (pre-homing)
      currentState = RETURN_TO_PICKUP;
      break;
      
    case RETURN_TO_PICKUP: // This state now occurs BEFORE homing
      // Return to pickup position to prepare for homing
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        gripperServo.write(SERVO_PICKUP_POS);  // Reset servo to pickup position
        Serial.println("Returned to pickup position (pre-homing), initiating X-axis homing");
        //! Step 12: Home X-axis
        currentState = HOME_X_AXIS;
      }
      break;
      
    case HOME_X_AXIS:
      // Home the X-axis
      homeXAxis(); // This is a blocking call
      Serial.println("X-axis homed, moving to pickup position (post-homing)");
      //! Step 13: Final Move to Pickup Position (post-homing)
      currentState = FINAL_MOVE_TO_PICKUP;
      break;
      
    case FINAL_MOVE_TO_PICKUP: // New state for post-homing move to pickup
      // Move to pickup position after homing
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        // Servo should already be at SERVO_PICKUP_POS from the RETURN_TO_PICKUP state before homing
        Serial.println("At pickup position (post-homing), cycle complete");
        //! Cycle Complete: Waiting for next trigger
        currentState = WAITING;
      }
      break;
  }
} 