# M5StickC Sony Alpha Camera Remote

A Bluetooth remote control for Sony Alpha cameras using M5StickC, specifically designed for astrophotography and long exposure shots.

## Features

- Bluetooth connection to Sony Alpha cameras
- Bulb mode control for exposures longer than 30 seconds
- Adjustable exposure time
- Auto-stop functionality
- Connection status display
- Current exposure time display

## Hardware Requirements

- M5StickC
- Sony Alpha camera with Bluetooth remote capability

## Setup Instructions

1. Install PlatformIO in your IDE (VSCode recommended)
2. Clone this repository
3. Find your camera's Bluetooth MAC address and update it in the code
4. Build and upload to your M5StickC

## Usage

- Button A (M5 button): Start/Stop exposure
- Button B: Adjust exposure time (increments by 30 seconds, up to 5 minutes)
- Display shows connection status and current exposure time

## Development

This is a boilerplate project. You'll need to:

1. Replace the placeholder Bluetooth MAC address with your camera's address
2. Implement the correct command codes for your specific Sony camera model
3. Test and adjust the timing and command sequences

## Notes

- The current implementation uses placeholder commands for the Bluetooth communication
- You'll need to implement the specific Sony camera protocol commands
- The maximum exposure time is set to 5 minutes but can be adjusted in the code
