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
        
        // Record commands
        constexpr uint8_t RECORD_UP = 0x0E;
        constexpr uint8_t RECORD_DOWN = 0x0F;
        
        // AF commands
        constexpr uint8_t AF_ON_UP = 0x14;
        constexpr uint8_t AF_ON_DOWN = 0x15;

        // Custom button commands
        constexpr uint8_t C1_UP = 0x20;
        constexpr uint8_t C1_DOWN = 0x21;

        // Zoom commands
        constexpr uint8_t ZOOM_TELE_RELEASE = 0x44;
        constexpr uint8_t ZOOM_TELE_PRESS = 0x45;
        constexpr uint8_t ZOOM_WIDE_RELEASE = 0x46;
        constexpr uint8_t ZOOM_WIDE_PRESS = 0x47;

        // Focus commands
        constexpr uint8_t FOCUS_IN_RELEASE = 0x6A;
        constexpr uint8_t FOCUS_IN_PRESS = 0x6B;
        constexpr uint8_t FOCUS_OUT_RELEASE = 0x6C;
        constexpr uint8_t FOCUS_OUT_PRESS = 0x6D;
    }

    // Status codes from camera
    namespace Status {
        // Focus status (0x3F)
        constexpr uint8_t FOCUS_LOST = 0x00;
        constexpr uint8_t FOCUS_ACQUIRED = 0x20;

        // Shutter status (0xA0)
        constexpr uint8_t SHUTTER_READY = 0x00;
        constexpr uint8_t SHUTTER_ACTIVE = 0x20;

        // Recording status (0xD5)
        constexpr uint8_t RECORDING_STOPPED = 0x00;
        constexpr uint8_t RECORDING_STARTED = 0x20;

        // Status types
        constexpr uint8_t TYPE_FOCUS = 0x3F;
        constexpr uint8_t TYPE_SHUTTER = 0xA0;
        constexpr uint8_t TYPE_RECORDING = 0xD5;
    }

    // Command functions
    bool shutterHalfPress();
    bool shutterHalfRelease();
    bool shutterFullPress();
    bool shutterFullRelease();
    bool shutterPress(); // Convenience function for full press-release cycle
    
    bool recordStart();
    bool recordStop();
    
    bool afPress();
    bool afRelease();
    
    bool c1Press();
    bool c1Release();
    
    bool zoomTelePress(uint8_t strength = 0x50);  // 0x10-0x8F
    bool zoomTeleRelease();
    bool zoomWidePress(uint8_t strength = 0x50);  // 0x10-0x8F
    bool zoomWideRelease();
    
    bool focusInPress(uint8_t strength = 0x40);   // 0x00-0x7F
    bool focusInRelease();
    bool focusOutPress(uint8_t strength = 0x40);  // 0x00-0x7F
    bool focusOutRelease();

    // Status functions
    bool isFocusAcquired();
    bool isShutterActive();
    bool isRecording();

    // Initialization
    void init();
    void update();

    // Notification handling
    void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
}
