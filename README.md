# Smart Housekeeper System
This is an AIoT project implemented on Arduino. The system consists of two independent agents, Smart Janitor and Smart Housekeeper, which communicate via Bluetooth to provide home security, energy savings, and automated management. For detailed system design, please refer to *Introduction.png* and *Workflow.png* in this repository.

## System Architecture
### Agent 1: Smart Janitor
Manages the smart door and tracks the movement of people entering and exiting the house.

Face Recognition: Uses a camera to identify users and control the door lock.

Direction Tracking: Utilizes pressure sensors inside and outside the door to detect movement direction. Combined with face recognition, it accurately records the current headcount inside the house.

User Interface: A monitor on the door displays customized greetings or status messages based on the identification results and system status.

### Agent 2: Smart Housekeeper
Manages indoor furniture and environmental controls.

LCD Status Display: A wall-mounted LCD screen shows the current usage status of connected furniture and devices.

IR Sensor Control: Users can control different devices based on the duration of hand gestures over an infrared sensor:

SHORT: Toggles the light ON/OFF.

MEDIUM: Opens or closes the windows.

LONG: Adjusts the light brightness.

## Bluetooth Integration Features
The two agents trigger advanced smart home scenarios via Bluetooth communication:

### 1. Intruder Alarm System
When the Smart Janitor records that no one is home, any attempt by a stranger to open the door triggers the security alarm.

The Smart Janitor sends an alert signal to the Smart Housekeeper.

The Smart Housekeeper then displays a warning message on the LCD, locks the windows, turns on the warning LED, and activates the buzzer.

### 2. Energy Saving & Auto-Shutdown
The moment the last family member leaves the house, the Smart Janitor requests the Smart Housekeeper to check if all lights and windows are closed.

If any devices are left on, the Smart Housekeeper automatically closes them for energy conservation and security.

After the check and shutdown are complete, the Smart Housekeeper sends a feedback message back to the Smart Janitor.

The Smart Janitor then displays an appropriate farewell greeting on the door's interface.
