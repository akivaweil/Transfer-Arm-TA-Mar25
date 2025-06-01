#include "Config/Pins_Definitions.h"

//* ************************************************************************
//* ************************ PIN DEFINITIONS ******************************
//* ************************************************************************

// Pin Definitions
// Inputs
const float START_BUTTON_PIN = 2.0;    // Start button input (active high)
const float STAGE1_SIGNAL_PIN = 23.0;  // Stage 1 machine signal input (active high)
const float X_HOME_SWITCH_PIN = 15.0;  // X-axis home limit switch (active high)
const float Z_HOME_SWITCH_PIN = 13.0;  // Z-axis home limit switch (active high)

// Outputs
const float X_STEP_PIN = 27.0;          // X-axis stepper motor step pin
const float X_DIR_PIN = 14.0;           // X-axis stepper motor direction pin
const float Z_STEP_PIN = 19.0;          // Z-axis stepper motor step pin
const float Z_DIR_PIN = 18.0;           // Z-axis stepper motor direction pin
const float SERVO_PIN = 26.0;           // Servo control pin
const float SOLENOID_RELAY_PIN = 33.0;  // Solenoid relay control pin
const float STAGE2_SIGNAL_PIN = 25.0;   // Signal output to Stage 2 machine (active high) 