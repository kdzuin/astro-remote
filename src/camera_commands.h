#pragma once

#include "ble_device.h"

// Camera command codes
namespace CameraCommands {
    // Command codes
    namespace Cmd {
        // Shutter commands
        constexpr uint8_t SHUTTER_HALF_UP = 0x06;
        constexpr uint8_t SHUTTER_HALF_DOWN = 0x07;
        constexpr uint8_t SHUTTER_FULL_UP = 0x08;
        constexpr uint8_t SHUTTER_FULL_DOWN = 0x09;
        
        // AF commands
        constexpr uint8_t AF_ON_UP = 0x14;
        constexpr uint8_t AF_ON_DOWN = 0x15;

        // Focus commands
        constexpr uint8_t FOCUS_IN_RELEASE = 0x6A;
        constexpr uint8_t FOCUS_IN_PRESS = 0x6B;
        constexpr uint8_t FOCUS_OUT_RELEASE = 0x6C;
        constexpr uint8_t FOCUS_OUT_PRESS = 0x6D;
    }

    // Status codes from camera
    namespace Status {
        constexpr uint8_t FOCUS_LOST = 0x00;
        constexpr uint8_t FOCUS_ACQUIRED = 0x20;
        constexpr uint8_t SHUTTER_READY = 0x00;
        constexpr uint8_t SHUTTER_ACTIVE = 0x20;
    }

    // Command functions
    bool shutterHalfPress();
    bool shutterHalfRelease();
    bool shutterFullPress();
    bool shutterFullRelease();
    bool shutterPress(); // Convenience function for full press-release cycle
    
    bool afPress();
    bool afRelease();
    
    bool focusInPress(uint8_t strength = 0x7F);
    bool focusInRelease();
    bool focusOutPress(uint8_t strength = 0x7F);
    bool focusOutRelease();

    // Status functions
    bool isFocusAcquired();
    bool isShutterActive();

    // Initialization
    void init();
    void update();

    // Notification handling
    void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
}
