#include <Arduino.h>
#include <AccelStepper.h>
#include <ESP32Servo.h>
#include <Bounce2.h>
#include "../include/Settings.h"
#include "../include/PickCycle.h"
#include "../include/Utils.h"
#include "../include/Homing.h"

/*
Pick and Place Cycle Steps (Values refer to constants in Settings.h):
1. Move to Pickup Position: X-axis to X_PICKUP_POS, Servo to SERVO_PICKUP_POS.
2. Lower Z-axis for Pickup (Continuous, Activate Vacuum during descent):
  - Z-axis moves towards Z_PICKUP_POS.
  - Vacuum (SOLENOID_RELAY_PIN HIGH) activates when Z passes Z_SUCTION_START_POS.
3. Wait at Pickup: Hold for PICKUP_HOLD_TIME ms.
4. Raise Z-axis with Object: Z-axis to Z_UP_POS.
5. Rotate Servo to Travel Position: Servo to SERVO_TRAVEL_POS.
6. Move to Dropoff Overshoot Position: X-axis to X_DROPOFF_OVERSHOOT_POS (4 inches past dropoff).
7. Rotate Servo to Dropoff Position: Servo to SERVO_DROPOFF_POS.
8. Wait for Servo Rotation: Hold for SERVO_ROTATION_WAIT_TIME ms (500ms).
9. Return to Dropoff Position: X-axis to X_DROPOFF_POS.
10. Lower Z-axis for Dropoff: Z-axis to Z_DROPOFF_POS.
11. Release Object: Vacuum (SOLENOID_RELAY_PIN LOW).
12. Wait After Release: Hold for DROPOFF_HOLD_TIME ms.
13. Raise Z-axis After Dropoff: Z-axis to Z_UP_POS.
14. Signal Stage 2: Pulse STAGE2_SIGNAL_PIN HIGH then LOW.
15. Return to Pickup Position (X-axis to X_PICKUP_POS, Servo to SERVO_PICKUP_POS).
16. Home X-axis.
17. Final Move to Pickup Position (X-axis to X_PICKUP_POS). Cycle ends, enters WAITING state.
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
  
  // Initialize Z-axis to normal speed and acceleration
  zStepper.setMaxSpeed(Z_MAX_SPEED);
  zStepper.setAcceleration(Z_ACCELERATION);
  
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
  // State machine for pick cycle
  switch (currentState) {
    case WAITING:
      // Check for pick cycle trigger
      if (startButton.read() == HIGH || stage1Signal.read() == HIGH) {
        Serial.println("Pick Cycle Triggered");
        enableXMotor();  // Enable X-axis motor for pick cycle
        //! Step 1: Move to Pickup Position
        currentState = MOVE_TO_PICKUP;
        stateTimer = 0;
      }
      break;
      
    case MOVE_TO_PICKUP:
      // Move from waiting position (X+3) to pickup position (X+0)
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        Serial.println("At X pickup position. Z already at pickup height from waiting.");
        Serial.print("Target Z: "); Serial.print(Z_PICKUP_POS);
        Serial.print(", Suction Start Z: "); Serial.println(Z_SUCTION_START_POS);
        gripperServo.write(SERVO_PICKUP_POS);  // Ensure servo is at pickup position
        vacuumActivatedDuringDescent = false; // Reset flag
        // Since Z is already at pickup height, activate vacuum and go directly to wait
        digitalWrite(SOLENOID_RELAY_PIN, HIGH);
        vacuumActivatedDuringDescent = true;
        Serial.println("Z already at pickup position, vacuum activated, starting pickup wait");
        stateTimer = 0; // Reset timer for the wait state
        //! Step 3: Wait at Pickup (Z already positioned, vacuum on)
        currentState = WAIT_AT_PICKUP;
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
        Serial.println("Z-axis raised, rotating servo to travel position");
        midpointServoRotated = false; // Reset for upcoming sequence
        //! Step 5: Rotate Servo to Travel Position
        currentState = ROTATE_SERVO_AFTER_PICKUP;
      }
      break;

    case ROTATE_SERVO_AFTER_PICKUP:
      // Rotate servo to travel position after pickup
      gripperServo.write(SERVO_TRAVEL_POS);
      // Assuming servo rotation is quick, directly move to next state.
      // If servo needs time, a timer or check would be needed here.
      Serial.println("Servo rotated to travel position, moving to dropoff overshoot");
      //! Step 6: Move to Dropoff Overshoot Position
      currentState = MOVE_TO_DROPOFF_OVERSHOOT;
      break;
      
    case MOVE_TO_DROPOFF_OVERSHOOT:
      // Move X axis to overshoot position (4 inches past dropoff)
      if (moveToPosition(xStepper, X_DROPOFF_OVERSHOOT_POS)) {
        Serial.println("At dropoff overshoot position, rotating servo to dropoff position");
        gripperServo.write(SERVO_DROPOFF_POS);
        stateTimer = 0;
        //! Step 7: Wait for Servo Rotation
        currentState = WAIT_FOR_SERVO_ROTATION;
      }
      break;
      
    case WAIT_FOR_SERVO_ROTATION:
      // Wait for servo to complete rotation at overshoot position
      if (Wait(SERVO_ROTATION_WAIT_TIME, &stateTimer)) {
        Serial.println("Servo rotation complete, returning to dropoff position");
        //! Step 8: Return to Dropoff Position
        currentState = RETURN_TO_DROPOFF;
      }
      break;
      
    case RETURN_TO_DROPOFF:
      // Move X axis back to normal dropoff position
      if (moveToPosition(xStepper, X_DROPOFF_POS)) {
        Serial.println("At dropoff X position, lowering Z-axis");
        //! Step 9: Lower Z-axis for Dropoff
        currentState = LOWER_Z_FOR_DROPOFF;
      }
      break;
      
    case LOWER_Z_FOR_DROPOFF:
      // Lower Z axis for dropoff at a slower speed
      zStepper.setMaxSpeed(Z_DROPOFF_MAX_SPEED);       // Set slower speed for dropoff movement
      zStepper.setAcceleration(Z_DROPOFF_ACCELERATION); // Set slower acceleration for dropoff movement
      zStepper.moveTo(Z_DROPOFF_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis lowered for dropoff, releasing object");
        //! Step 10: Release Object
        currentState = RELEASE_OBJECT;
      }
      break;
      
    case RELEASE_OBJECT:
      // Turn off the vacuum solenoid
      digitalWrite(SOLENOID_RELAY_PIN, LOW);
      Serial.println("Object released, waiting briefly");
      stateTimer = 0;
      //! Step 11: Wait After Release
      currentState = WAIT_AFTER_RELEASE;
      break;
      
    case WAIT_AFTER_RELEASE:
      // Wait briefly after release
      if (Wait(DROPOFF_HOLD_TIME, &stateTimer)) {
        Serial.println("Wait complete, raising Z-axis");
        // Restore normal Z-axis speed and acceleration for upward movement
        zStepper.setMaxSpeed(Z_MAX_SPEED);
        zStepper.setAcceleration(Z_ACCELERATION);
        //! Step 12: Raise Z-axis After Dropoff
        currentState = RAISE_Z_AFTER_DROPOFF;
      }
      break;
      
    case RAISE_Z_AFTER_DROPOFF:
      // Raise Z axis after dropoff
      zStepper.moveTo(Z_UP_POS);
      if (zStepper.distanceToGo() == 0) {
        Serial.println("Z-axis raised, signaling Stage 2");
        //! Step 13: Signal Stage 2
        currentState = SIGNAL_STAGE2;
      }
      break;
      
    case SIGNAL_STAGE2:
      // Send signal to Stage 2 machine
      digitalWrite(STAGE2_SIGNAL_PIN, HIGH);
      delay(100);  // Brief pulse
      digitalWrite(STAGE2_SIGNAL_PIN, LOW);
      Serial.println("Stage 2 signaled, returning to pickup position (pre-homing)");
      //! Step 14: Return to Pickup Position (pre-homing)
      currentState = RETURN_TO_PICKUP;
      break;
      
    case RETURN_TO_PICKUP: // This state now occurs BEFORE homing
      // Return to pickup position to prepare for homing
      if (moveToPosition(xStepper, X_PICKUP_POS)) {
        gripperServo.write(SERVO_PICKUP_POS);  // Reset servo to pickup position
        Serial.println("Returned to pickup position (pre-homing), initiating X-axis homing");
        //! Step 15: Home X-axis
        currentState = HOME_X_AXIS;
      }
      break;
      
    case HOME_X_AXIS:
      // Home the X-axis
      homeXAxis(); // This is a blocking call
      Serial.println("X-axis homed, moving to pickup position (post-homing)");
      //! Step 16: Final Move to Pickup Position (post-homing)
      currentState = FINAL_MOVE_TO_PICKUP;
      break;
      
    case FINAL_MOVE_TO_PICKUP: // New state for post-homing move to waiting position
      // Restore normal Z-axis speed and acceleration first
      zStepper.setMaxSpeed(Z_MAX_SPEED);
      zStepper.setAcceleration(Z_ACCELERATION);
      
      // Move to waiting position after homing (pickup + 3 inches)
      if (moveToPosition(xStepper, X_WAIT_POS)) {
        // Check if Z is already at pickup position
        if (zStepper.currentPosition() == Z_PICKUP_POS) {
          // Z is already at pickup position, complete the cycle
          gripperServo.write(SERVO_PICKUP_POS);
          Serial.println("At waiting position (pickup+3, Z at pickup height), cycle complete");
          disableXMotor();  // Disable X-axis motor after cycle completion
          //! Cycle Complete: Waiting for next trigger
          currentState = WAITING;
        } else {
          // Z needs to move to pickup position - no serial logs for this movement
          zStepper.moveTo(Z_PICKUP_POS);
          // Note: Will check completion on next loop iteration
        }
      }
      break;
  }
} 