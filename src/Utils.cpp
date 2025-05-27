#include "../include/Utils.h"

#include <AccelStepper.h>
#include <Arduino.h>

#include "../include/WebServer.h"

// Move a stepper to a position and return true when motion is complete
bool moveToPosition(AccelStepper& stepper, long position) {
  static long lastTarget = -999999;   // Track the last target position
  static bool motionStarted = false;  // Track if motion has started

  // If this is a new target position or motion hasn't started yet
  if (position != lastTarget || !motionStarted) {
    smartLog("Setting new target position: " + String(position) +
             ", Current pos: " + String(stepper.currentPosition()));

    // Set the target position
    stepper.moveTo(position);
    lastTarget = position;
    motionStarted = true;
    return false;  // Motion is not complete
  }

  // Return true if the motor has reached the target position
  if (stepper.distanceToGo() == 0) {
    motionStarted = false;  // Reset for next movement
    return true;            // Motion is complete
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

// Smart logging function - routes to WebSocket if clients connected, otherwise
// to Serial
void smartLog(const String& message) {
  if (webServer.hasConnectedClients()) {
    // Send to web dashboard if clients are connected
    webServer.sendLogMessage(message);
  } else {
    // Fall back to Serial if no web clients are connected
    Serial.println(message);
  }
}