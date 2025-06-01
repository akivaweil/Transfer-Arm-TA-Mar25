#include "../../config/Config.h"
#include "../../config/Pins_Definitions.h"
#include "../../../include/TransferArm.h"
#include "../../../include/Utils.h"

// External variable to track vacuum activation
extern bool vacuumActivatedDuringDescent;

//* ************************************************************************
//* ************************ PICKUP SEQUENCE FUNCTIONS ***************************
//* ************************************************************************
// Functions for pickup sequence operations

// Setup Z-axis for pickup operations
void setupZAxisForPickup() {
  // Initialize Z-axis to normal speed and acceleration
  transferArm.getZStepper().setMaxSpeed(Z_MAX_SPEED);
  transferArm.getZStepper().setAcceleration(Z_ACCELERATION);
  smartLog("Z-axis configured for pickup operations");
  delay(250);
}

// Activate vacuum during Z descent when reaching the suction start position
void activateVacuumDuringDescent() {
  if (!vacuumActivatedDuringDescent &&
      transferArm.getZStepper().currentPosition() >= Z_SUCTION_START_POS) {
    digitalWrite(SOLENOID_RELAY_PIN, HIGH);
    vacuumActivatedDuringDescent = true;
    smartLog("Vacuum activated during descent at Z: " +
             String(transferArm.getZStepper().currentPosition()));
  }
}

// Reset pickup sequence variables
void resetPickupSequence() {
  vacuumActivatedDuringDescent = false;
  smartLog("Pickup sequence variables reset");
} 