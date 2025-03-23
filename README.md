# Transfer Arm Control System

ESP32-based control system for a robotic transfer arm that picks up objects from one location and places them in another.

## Basic Operation Flow
1. **Initialization** - System power-on
2. **Wait for Signal** - From limit switch (active high) or Stage 1 machine
3. **Pick Cycle** - Execute the pick and place sequence
4. **Return Home** - Return to starting position

## Pin Layout
Using Freenove ESP32 breakout board:
- Left side (top to bottom): 34, 35, 32, 33, 25, 26, 27, 14, 12, 13
- Right side (top to bottom): 23, 22, 21, 19, 18, 5, 4, 0, 2, 15 