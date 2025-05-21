#ifndef HOMING_H
#define HOMING_H

#include <AccelStepper.h>
#include <Bounce2.h>

//* ************************************************************************
//* **************************** HOMING LOGIC ******************************
//* ************************************************************************
// This file contains the function declarations for the homing sequence of the Transfer Arm.

// External Bounce objects for switch debouncing
extern Bounce xHomeSwitch;
extern Bounce zHomeSwitch;

// Function declarations
void homeSystem();
void homeZAxis();
void homeXAxis();

#endif // HOMING_H 