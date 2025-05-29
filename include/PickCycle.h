#ifndef PICKCYCLE_H
#define PICKCYCLE_H

// Include config files
#include "../src/config/Config.h"
#include "../src/config/Pins_Definitions.h"

// Forward declarations
class AccelStepper;

// Function declarations for pick cycle operations
void initializePickCycle();
void updatePickCycle();
PickCycleState getCurrentState();
void setCurrentState(PickCycleState newState);
void triggerPickCycleFromWeb();
const char* getStateString(PickCycleState state);

// Movement and utility functions
bool moveToPosition(AccelStepper& stepper, float targetPosition);
bool Wait(unsigned long duration, unsigned long* timer);

#endif  // PICKCYCLE_H