#ifndef PICK_CYCLE_H
#define PICK_CYCLE_H

#include "Settings.h"
#include <Bounce2.h>

//* ************************************************************************
//* ************************* PICK CYCLE LOGIC ***************************
//* ************************************************************************
// This file contains the function declarations and state definitions for the pick-and-place cycle.

// External Bounce objects for switch debouncing
extern Bounce startButton;
extern Bounce stage1Signal;

// Functions for the pick cycle state machine
void initializePickCycle();
void updatePickCycle();
PickCycleState getCurrentState();

#endif // PICK_CYCLE_H 