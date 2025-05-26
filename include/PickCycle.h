#ifndef PICK_CYCLE_H
#define PICK_CYCLE_H

#include <Bounce2.h>

#include "Settings.h"

//* ************************************************************************
//* ************************* PICK CYCLE LOGIC ***************************
//* ************************************************************************
// This file contains the function declarations and state definitions for the
// pick-and-place cycle.

// Forward declaration
class TransferArm;

// Functions for the pick cycle state machine
void initializePickCycle();
void updatePickCycle();
PickCycleState getCurrentState();

#endif  // PICK_CYCLE_H