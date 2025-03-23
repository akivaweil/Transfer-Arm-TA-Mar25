#include <Arduino.h>
#include <AccelStepper.h>
#include <Bounce2.h>

// Pin Definitions
// Inputs
const int LIMIT_SWITCH_PIN = 34;     // Limit switch input (active high)
const int STAGE1_SIGNAL_PIN = 35;    // Stage 1 machine signal input

// Setup function - runs once at startup
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Configure input pins
  pinMode(LIMIT_SWITCH_PIN, INPUT);
  pinMode(STAGE1_SIGNAL_PIN, INPUT);
  
  Serial.println("Transfer Arm Initialized");
}

// Main loop function - runs repeatedly
void loop() {
  // Check for pick cycle trigger
  if (digitalRead(LIMIT_SWITCH_PIN) == HIGH || digitalRead(STAGE1_SIGNAL_PIN) == HIGH) {
    Serial.println("Pick Cycle Triggered");
    
    // Execute pick cycle sequence
    // TO BE IMPLEMENTED
    
    // Return to home position
    // TO BE IMPLEMENTED
    
    Serial.println("Pick Cycle Completed");
    
    // Small delay to prevent immediate re-triggering
    delay(500);
  }
}
