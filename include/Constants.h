#ifndef CONSTANTS_H
#define CONSTANTS_H

// Pin Definitions
// Inputs
const int LIMIT_SWITCH_PIN = 34;     // Limit switch input (active high)
const int STAGE1_SIGNAL_PIN = 35;    // Stage 1 machine signal input
const int X_HOME_SWITCH_PIN = 32;    // X-axis home limit switch (active high)
const int Z_HOME_SWITCH_PIN = 33;    // Z-axis home limit switch (active high)

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
const float STEPS_PER_INCH = STEPS_PER_MM * 25.4;  // Convert to steps per inch

// Positions
const int X_HOME_POS = 0;            // X-axis home position (in steps)
const int Z_HOME_POS = 0;            // Z-axis home position (in steps)
const int X_PICKUP_POS = 500;        // X-axis pickup position (in steps) - adjust as needed
const int X_PLACE_POS = 1500;        // X-axis place position (in steps) - adjust as needed
const int Z_UP_POS = int(5 * STEPS_PER_INCH);  // Z-axis up position (5 inches up from home)
const int Z_DOWN_POS = 0;            // Z-axis down position (at home)

const int SERVO_HOME_POS = 90;       // Servo home position (in degrees)
const int SERVO_PICKUP_POS = 0;      // Servo pickup position (in degrees) - adjust as needed
const int SERVO_PLACE_POS = 180;     // Servo place position (in degrees) - adjust as needed

// Stepper settings
const int MAX_SPEED = 2000;          // Maximum speed in steps per second
const int ACCELERATION = 1000;       // Acceleration in steps per second^2
const unsigned long HOMING_TIMEOUT = 30000; // Timeout for homing sequence (30 seconds)

// State enum for pick cycle
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

#endif // CONSTANTS_H 