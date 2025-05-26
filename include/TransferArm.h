#ifndef TRANSFER_ARM_H
#define TRANSFER_ARM_H

#include <AccelStepper.h>
#include <Arduino.h>
#include <Bounce2.h>
#include <ESP32Servo.h>

#include "Settings.h"

//* ************************************************************************
//* ************************ TRANSFER ARM CLASS *************************
//* ************************************************************************
// This class encapsulates all the Transfer Arm functionality including
// hardware initialization, homing, and the pick-and-place cycle.

class TransferArm {
 private:
  // Hardware instances
  AccelStepper xStepper;
  AccelStepper zStepper;
  Servo gripperServo;

  // Bounce objects for debouncing
  Bounce xHomeSwitch;
  Bounce zHomeSwitch;
  Bounce startButton;
  Bounce stage1Signal;

  // Private methods
  void initializeHardware();
  void configurePins();
  void configureSteppers();
  void configureServo();
  void configureDebouncers();

 public:
  // Constructor
  TransferArm();

  // Main interface methods
  void begin();
  void update();

  // Getter methods for accessing hardware (needed by other modules)
  AccelStepper& getXStepper() { return xStepper; }
  AccelStepper& getZStepper() { return zStepper; }
  Servo& getGripperServo() { return gripperServo; }
  Bounce& getXHomeSwitch() { return xHomeSwitch; }
  Bounce& getZHomeSwitch() { return zHomeSwitch; }
  Bounce& getStartButton() { return startButton; }
  Bounce& getStage1Signal() { return stage1Signal; }
};

// Global instance declaration
extern TransferArm transferArm;

#endif  // TRANSFER_ARM_H