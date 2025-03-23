#include <Arduino.h>
#include <AccelStepper.h>
#include "../include/Utils.h"

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