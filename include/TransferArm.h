#ifndef TRANSFER_ARM_H
#define TRANSFER_ARM_H

#include <AccelStepper.h>
#include <Bounce2.h>
#include <ESP32Servo.h>

// Include our config files
#include "../src/Config/Config.h"
#include "../src/Config/Pins_Definitions.h"

//* ************************************************************************
//* ************************ TRANSFER ARM CLASS *************************
//* ************************************************************************
// This class encapsulates all Transfer Arm functionality including
// hardware initialization, homing, and pick-and-place cycle operations.
// No webserver functionality - pure hardware control.

class TransferArm {
 private:
  // Hardware instances
  AccelStepper xStepper;
  AccelStepper zStepper;
  Servo gripperServo;
  float currentServoPosition;  // Track servo position since ESP32Servo doesn't have read()

  // Bounce objects for debouncing
  Bounce xHomeSwitch;
  Bounce zHomeSwitch;
  Bounce startButton;
  Bounce stage1Signal;
  Bounce stopSignalStage2;

  // Private hardware configuration methods
  void configurePins();
  void configureDebouncers();
  void configureSteppers();
  void configureServo();

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
  Bounce& getStopSignalStage2() { return stopSignalStage2; }

  // Servo control methods
  void setServoPosition(float position);
  float getServoPosition() const { return currentServoPosition; }

  // Status methods
  bool isXMoving() { return xStepper.isRunning(); }
  bool isZMoving() { return zStepper.isRunning(); }
  bool isAnyMotorMoving() { return xStepper.isRunning() || zStepper.isRunning(); }

  // Safety methods
  bool isStage2SafeForZLowering();

  // Motor enable/disable methods
  void enableXMotor();
  void disableXMotor();

  // Communication methods
  void handleSerialCommand(const String& command);
  void sendSerialMessage(const String& message);
  void sendBurstRequest();
};

// Global instance declaration
extern TransferArm transferArm;

#endif  // TRANSFER_ARM_H