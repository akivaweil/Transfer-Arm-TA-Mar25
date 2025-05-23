#include <Arduino.h>
#include <AccelStepper.h>
#include "../include/Utils.h"

// Move a stepper to a position and return true when motion is complete
bool moveToPosition(AccelStepper &stepper, long position) {
  static long lastTarget = -999999;  // Track the last target position
  static bool motionStarted = false; // Track if motion has started
  
  // If this is a new target position or motion hasn't started yet
  if (position != lastTarget || !motionStarted) {
    Serial.print("Setting new target position: ");
    Serial.print(position);
    Serial.print(", Current pos: ");
    Serial.println(stepper.currentPosition());
    
    // Set the target position
    stepper.moveTo(position);
    lastTarget = position;
    motionStarted = true;
    return false; // Motion is not complete
  }
  
  // Return true if the motor has reached the target position
  if (stepper.distanceToGo() == 0) {
    motionStarted = false; // Reset for next movement
    return true;  // Motion is complete
  }
  
  return false;  // Motion is still in progress
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