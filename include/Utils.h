#ifndef UTILS_H
#define UTILS_H

#include <AccelStepper.h>

// Function declarations
bool moveToPosition(AccelStepper &stepper, long position);
bool Wait(unsigned long delayTime, unsigned long* startTimePtr);

#endif // UTILS_H 