#ifndef PINS_DEFINITIONS_H
#define PINS_DEFINITIONS_H

// Pin Definitions
// Inputs
extern const float START_BUTTON_PIN;    // Start button input (active high)
extern const float STAGE1_SIGNAL_PIN;  // Stage 1 machine signal input (active high)
extern const float X_HOME_SWITCH_PIN;  // X-axis home limit switch (active high)
extern const float Z_HOME_SWITCH_PIN;  // Z-axis home limit switch (active high)

// Outputs
extern const float X_STEP_PIN;          // X-axis stepper motor step pin
extern const float X_DIR_PIN;           // X-axis stepper motor direction pin
extern const float Z_STEP_PIN;          // Z-axis stepper motor step pin
extern const float Z_DIR_PIN;           // Z-axis stepper motor direction pin
extern const float SERVO_PIN;           // Servo control pin
extern const float SOLENOID_RELAY_PIN;  // Solenoid relay control pin
extern const float STAGE2_SIGNAL_PIN;   // Signal output to Stage 2 machine (active high)

#endif  // PINS_DEFINITIONS_H 