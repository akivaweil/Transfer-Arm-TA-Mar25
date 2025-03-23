#ifndef PICK_CYCLE_H
#define PICK_CYCLE_H

#include "Constants.h"

// Functions for the pick cycle state machine
void initializePickCycle();
void updatePickCycle();
PickCycleState getCurrentState();

#endif // PICK_CYCLE_H 