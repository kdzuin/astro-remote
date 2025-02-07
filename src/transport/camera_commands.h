#pragma once

#include "ble_device.h"

namespace CameraCommands {
    // Command codes (2-byte format)
    namespace Cmd {
        // Shutter control
        constexpr uint16_t SHUTTER_HALF_UP = 0x0106;    // Release half-press
        constexpr uint16_t SHUTTER_HALF_DOWN = 0x0107;  // Half-press shutter
        constexpr uint16_t SHUTTER_FULL_UP = 0x0108;    // Release full-press
        constexpr uint16_t SHUTTER_FULL_DOWN = 0x0109;  // Full-press shutter

        // Recording control
        constexpr uint16_t RECORD_UP = 0x010E;    // Stop recording
        constexpr uint16_t RECORD_DOWN = 0x010F;  // Start recording

        // Autofocus control
        constexpr uint16_t AF_ON_UP = 0x0114;    // Release AF
        constexpr uint16_t AF_ON_DOWN = 0x0115;  // Start AF

        // Custom button
        constexpr uint16_t C1_UP = 0x0120;    // Release C1
        constexpr uint16_t C1_DOWN = 0x0121;  // Press C1

        // Zoom control (add parameter 0x00-0x0F for release, 0x10-0x8F for press)
        constexpr uint16_t ZOOM_TELE_RELEASE = 0x0244;  // Release zoom tele
        constexpr uint16_t ZOOM_TELE_PRESS = 0x0245;    // Press zoom tele
        constexpr uint16_t ZOOM_WIDE_RELEASE = 0x0246;  // Release zoom wide
        constexpr uint16_t ZOOM_WIDE_PRESS = 0x0247;    // Press zoom wide

        // Manual focus control (add parameter 0x00-0x0F for release, 0x10-0x7F for press)
        constexpr uint16_t FOCUS_IN_RELEASE = 0x026A;   // Release focus in
        constexpr uint16_t FOCUS_IN_PRESS = 0x026B;     // Press focus in
        constexpr uint16_t FOCUS_OUT_RELEASE = 0x026C;  // Release focus out
        constexpr uint16_t FOCUS_OUT_PRESS = 0x026D;    // Press focus out
    }

    // Status codes from camera (0xFF02)
    namespace Status {
        // Focus status
        constexpr uint8_t FOCUS_TYPE = 0x3F;      // Focus status type
        constexpr uint8_t FOCUS_LOST = 0x00;      // 0x02 3F 00
        constexpr uint8_t FOCUS_ACQUIRED = 0x20;  // 0x02 3F 20

        // Shutter status
        constexpr uint8_t SHUTTER_TYPE = 0xA0;    // Shutter status type
        constexpr uint8_t SHUTTER_READY = 0x00;   // 0x02 A0 00
        constexpr uint8_t SHUTTER_ACTIVE = 0x20;  // 0x02 A0 20

        // Recording status
        constexpr uint8_t RECORD_TYPE = 0xD5;     // Recording status type
        constexpr uint8_t RECORD_STOPPED = 0x00;  // 0x02 D5 00
        constexpr uint8_t RECORD_STARTED = 0x20;  // 0x02 D5 20
    }

    // Focus modes
    enum class FocusMode {
        AUTO_FOCUS,
        MANUAL_FOCUS
    };

    // Interface functions
    void init();
    void update();
    bool testConnection();
    bool startFocus();
    bool stopFocus();
    bool pressShutter();
    bool releaseShutter();
    bool shutterPress(uint32_t focusTimeout = 1000);
    bool recordStart();
    bool recordStop();
    bool isConnected();
    bool isFocusAcquired();
    bool isShutterActive();
    bool isRecording();

    // Status notification handler
    void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

    // Internal functions
    bool sendCommand16(uint16_t cmd);
    void handleFocusStateChange(uint8_t prevState, uint8_t newState);
    void handleShutterStateChange(uint8_t prevState, uint8_t newState);
    void handleRecordingStateChange(uint8_t prevState, uint8_t newState);
};
