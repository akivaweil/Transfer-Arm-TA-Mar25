#ifndef CONSTANTS_H
#define CONSTANTS_H

// Pin Definitions
// Inputs
const int LIMIT_SWITCH_PIN = 34;     // Limit switch input (active high)
const int STAGE1_SIGNAL_PIN = 35;    // Stage 1 machine signal input (active high)
const int X_HOME_SWITCH_PIN = 32;    // X-axis home limit switch (active high)
const int Z_HOME_SWITCH_PIN = 33;    // Z-axis home limit switch (active high)

// Outputs
const int X_STEP_PIN = 25;           // X-axis stepper motor step pin
const int X_DIR_PIN = 26;            // X-axis stepper motor direction pin
const int Z_STEP_PIN = 27;           // Z-axis stepper motor step pin
const int Z_DIR_PIN = 14;            // Z-axis stepper motor direction pin
const int SERVO_PIN = 12;            // Servo control pin
const int SOLENOID_RELAY_PIN = 13;   // Solenoid relay control pin
const int STAGE2_SIGNAL_PIN = 23;    // Signal output to Stage 2 machine (active high)

// Constants
const int STEPS_PER_REV = 400;       // Steps per revolution for steppers (1.8° with 1/2 microstepping)
const int PULLEY_TEETH = 20;         // Number of teeth on the pulley
const int BELT_PITCH = 2;            // GT2 belt pitch in mm
const float STEPS_PER_MM = STEPS_PER_REV / (PULLEY_TEETH * BELT_PITCH); // Steps per mm of linear movement
const float STEPS_PER_INCH = STEPS_PER_MM * 25.4;  // Convert to steps per inch

// Positions
const int X_HOME_POS = 0;            // X-axis home position (in steps)
const int Z_HOME_POS = 0;            // Z-axis home position (in steps)

// X-axis positions in inches from home
const int X_PICKUP_POS_INCHES = 5;   // X-axis pickup position (5 inches)
const int X_DROPOFF_POS_INCHES = 20; // X-axis dropoff position (20 inches)
const int X_MIDPOINT_INCHES = (X_PICKUP_POS_INCHES + X_DROPOFF_POS_INCHES) / 2; // Midpoint for servo rotation

// Z-axis distances in inches
const int Z_PICKUP_LOWER_INCHES = 5;    // Lower Z-axis by 5 inches for pickup
const int Z_SUCTION_START_INCHES = 4;   // Start suction when Z is 4 inches down
const int Z_DROPOFF_LOWER_INCHES = 3;   // Lower Z-axis by 3 inches for dropoff

// Converted positions to steps
const int X_PICKUP_POS = X_PICKUP_POS_INCHES * STEPS_PER_INCH;
const int X_DROPOFF_POS = X_DROPOFF_POS_INCHES * STEPS_PER_INCH;
const int X_MIDPOINT_POS = X_MIDPOINT_INCHES * STEPS_PER_INCH;

const int Z_UP_POS = 0;                                   // Z-axis fully up position
const int Z_PICKUP_POS = Z_PICKUP_LOWER_INCHES * STEPS_PER_INCH; // Z-axis down position for pickup
const int Z_SUCTION_START_POS = Z_SUCTION_START_INCHES * STEPS_PER_INCH; // Z position to start suction
const int Z_DROPOFF_POS = Z_DROPOFF_LOWER_INCHES * STEPS_PER_INCH; // Z-axis down position for dropoff

// Servo angles
const int SERVO_HOME_POS = 90;       // Servo home position (in degrees)
const int SERVO_PICKUP_POS = 0;      // Servo pickup position (in degrees)
const int SERVO_DROPOFF_POS = 90;    // Servo dropoff position (90 degrees from pickup)

// Timing constants
const unsigned long PICKUP_HOLD_TIME = 300;   // Hold time at pickup position (300ms)
const unsigned long DROPOFF_HOLD_TIME = 100;  // Hold time at dropoff position (100ms)

// Stepper settings
const int MAX_SPEED = 2000;          // Maximum speed in steps per second
const int ACCELERATION = 1000;       // Acceleration in steps per second^2

// State enum for pick cycle
enum PickCycleState {
  WAITING,
  MOVE_TO_PICKUP,
  LOWER_Z_FOR_PICKUP,
  ACTIVATE_VACUUM,
  CONTINUE_LOWERING_Z,
  WAIT_AT_PICKUP,
  RAISE_Z_WITH_OBJECT,
  MOVE_TO_DROPOFF,
  ROTATE_SERVO_MIDPOINT,
  LOWER_Z_FOR_DROPOFF,
  RELEASE_OBJECT,
  WAIT_AFTER_RELEASE,
  RAISE_Z_AFTER_DROPOFF,
  SIGNAL_STAGE2,
  RETURN_TO_PICKUP
};

#endif // CONSTANTS_H 