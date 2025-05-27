#ifndef UTILS_H
#define UTILS_H

#include <AccelStepper.h>
#include <Arduino.h>

// Function declarations
bool moveToPosition(AccelStepper& stepper, long position);
bool Wait(unsigned long delayTime, unsigned long* startTimePtr);

// Smart logging function - routes to WebSocket if clients connected, otherwise
// to Serial
void smartLog(const String& message);

#endif  // UTILS_H