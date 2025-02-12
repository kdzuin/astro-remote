# M5StickC Sony Alpha Camera Remote

A Bluetooth remote control for Sony Alpha cameras using M5StickC, specifically designed for astrophotography and long exposure shots, and some nice benefits of remote controlling your camera.

## Features

- Bluetooth connection to Sony Alpha cameras
- Astro Remote
  - Bulb mode control for exposures longer than 30 seconds
  - Adjustable exposure time
  - Auto-stop functionality
  - Connection status display
  - Current exposure time display
- Photo Remote
  - Ability to use as a remote control
  - Photo Counter
- Video Remote
  - Ability to use as a remote control to start/stop video recording
  - Tally Light with recording time
- Manual Control Section
  - Explicit Camera actions, that are a low-level interface for interaction with the camera
- Settings
  - Control the brightness of remote control device screen
  - Remote control battery level (in case is not connected to a power source)
  - Auto-Connect functionality to a last saved device

## Optional Web-based remote control for your remote control

The project also includes a web-based remote control, that can we hosted anywhere since it's just a static files not requiring any build process, or can be accessed from the GitHub pages for this repository.

## Hardware Requirements

- M5StickC, or any other M5 device supported by M5Unified
- Abstract Hardware layer supports further integration with different boards/devices
- Sony Alpha camera with Bluetooth remote capability

## Project Architecture

### Hardware Abstraction Layer

The project uses a clean hardware abstraction layer to separate device-specific code from the application logic:

- `IDisplay`: Interface for screen operations (M5StickC LCD)
  - Color handling via `getColor(r, g, b)`
  - Text and graphics primitives
- `IInput`: Interface for button handling
  - Supports A, B, and PWR buttons
  - Provides both immediate and event-based states
- `IHardware`: Main hardware interface combining display and input

This abstraction allows for:

- Easy testing with mock hardware
- Potential support for other M5Stack devices, and not only M5 devices
- Clear separation between UI logic and hardware specifics

## Setup Instructions

### PlatformIO Setup

1. Install PlatformIO in VSCode
2. Clone this repository
3. Build and upload to your M5StickC

### VSCode Configuration

To ensure proper code navigation and eliminate false errors:

1. Create/update `.vscode/c_cpp_properties.json`:

   ```json
   {
     "configurations": [
       {
         "name": "PlatformIO",
         "includePath": ["${workspaceFolder}/**", "${workspaceFolder}/src"],
         "forcedInclude": ["${workspaceFolder}/src/debug.h"]
       }
     ],
     "version": 4
   }
   ```

2. Key settings:
   - `includePath`: Ensures proper header resolution
   - `forcedInclude`: Handles global debug.h inclusion
   - PlatformIO will handle all ESP32/Arduino includes automatically

## Usage

- Button A (M5 button): Confirm current action
- Button B: Cycle between options
- Button PWR: Go back to main menu

## Development Notes

### Hardware Layer Usage

```cpp
// Example of hardware abstraction usage
auto& display = hardware->getDisplay();
display.setTextColor(display.getColor(255, 255, 255));  // White text
display.fillScreen(display.getColor(0, 0, 0));         // Black background
```

### Debug Support

The project includes a global debug header that provides logging macros. These are automatically included in all source files via the PlatformIO configuration.
