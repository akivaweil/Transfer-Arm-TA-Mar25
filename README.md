# Transfer Arm Control System

ESP32-based control system for a robotic transfer arm that picks up objects from one location and places them in another.

## Hardware Requirements
- ESP32 Development Board
- Stepper motors for arm movement
- Vacuum gripper or mechanical gripper system
- Limit switches for homing
- Sensors for object detection/grip verification

## Libraries Used
- AccelStepper: For controlling stepper motors
- Bounce2: For debouncing button/switch inputs

## Project Structure
- `src/`: Contains the main code
- `include/`: Header files
- `flowchart.dot`: DOT file for the process flowchart
- `transfer_arm_flowchart.png`: Visual representation of the control flow

## Operation Flow
The system operates following these main steps:
1. Initialization and homing
2. Waiting for pickup signal
3. Moving to pickup position
4. Gripping the object
5. Transferring to target position
6. Releasing the object
7. Returning to home position

See the flowchart for a detailed visual representation of the process.

## Pin Layout
Using Freenove ESP32 breakout board:
- Left side (top to bottom): 34, 35, 32, 33, 25, 26, 27, 14, 12, 13
- Right side (top to bottom): 23, 22, 21, 19, 18, 5, 4, 0, 2, 15

## Development Status
- [x] Created process flowchart
- [ ] Implement basic motion control
- [ ] Add gripping mechanism control
- [ ] Implement sensor feedback
- [ ] Add error handling and recovery
- [ ] Fine-tune motion parameters
- [ ] Full system testing 