#include "camera_commands.h"

namespace {
    uint8_t focusStatus = CameraCommands::Status::FOCUS_LOST;
    uint8_t shutterStatus = CameraCommands::Status::SHUTTER_READY;
    uint8_t recordingStatus = CameraCommands::Status::RECORDING_STOPPED;
    bool statusNotificationEnabled = false;

    // Helper function to send command
    bool sendCommand(uint8_t cmd) {
        if (!BLEDeviceManager::isConnected()) {
            Serial.println("[Camera] Not connected");
            return false;
        }

        BLERemoteCharacteristic* pChar = BLEDeviceManager::getControlCharacteristic();
        if (!pChar) {
            Serial.println("[Camera] Control characteristic not available");
            return false;
        }

        uint8_t cmdBuffer[] = {0x01, cmd}; // Length (1) + Command
        pChar->writeValue(cmdBuffer, sizeof(cmdBuffer));
        Serial.printf("[Camera] Sent command: 0x%02X\n", cmd);
        return true;
    }

    // Helper function to send command with parameter
    bool sendCommandWithParam(uint8_t cmd, uint8_t param) {
        if (!BLEDeviceManager::isConnected()) {
            Serial.println("[Camera] Not connected");
            return false;
        }

        BLERemoteCharacteristic* pChar = BLEDeviceManager::getControlCharacteristic();
        if (!pChar) {
            Serial.println("[Camera] Control characteristic not available");
            return false;
        }

        uint8_t cmdBuffer[] = {0x02, cmd, param}; // Length (2) + Command + Parameter
        pChar->writeValue(cmdBuffer, sizeof(cmdBuffer));
        Serial.printf("[Camera] Sent command: 0x%02X with param: 0x%02X\n", cmd, param);
        return true;
    }
}

namespace CameraCommands {
    void init() {
        // Register for status notifications
        BLERemoteCharacteristic* pChar = BLEDeviceManager::getStatusCharacteristic();
        if (pChar) {
            pChar->registerForNotify(onStatusNotification);
            statusNotificationEnabled = true;
            Serial.println("[Camera] Status notifications enabled");
        }
    }

    void update() {
        // Nothing to do here yet
    }

    bool shutterHalfPress() {
        return sendCommand(Cmd::SHUTTER_HALF_DOWN);
    }

    bool shutterHalfRelease() {
        return sendCommand(Cmd::SHUTTER_HALF_UP);
    }

    bool shutterFullPress() {
        return sendCommand(Cmd::SHUTTER_FULL_DOWN);
    }

    bool shutterFullRelease() {
        return sendCommand(Cmd::SHUTTER_FULL_UP);
    }

    bool shutterPress() {
        if (!shutterHalfPress()) return false;
        delay(100); // Give camera time to focus
        if (!shutterFullPress()) return false;
        delay(100); // Give camera time to take photo
        if (!shutterFullRelease()) return false;
        if (!shutterHalfRelease()) return false;
        return true;
    }

    bool recordStart() {
        return sendCommand(Cmd::RECORD_DOWN);
    }

    bool recordStop() {
        return sendCommand(Cmd::RECORD_UP);
    }

    bool afPress() {
        return sendCommand(Cmd::AF_ON_DOWN);
    }

    bool afRelease() {
        return sendCommand(Cmd::AF_ON_UP);
    }

    bool c1Press() {
        return sendCommand(Cmd::C1_DOWN);
    }

    bool c1Release() {
        return sendCommand(Cmd::C1_UP);
    }

    bool zoomTelePress(uint8_t strength) {
        if (strength < 0x10) strength = 0x10;
        if (strength > 0x8F) strength = 0x8F;
        return sendCommandWithParam(Cmd::ZOOM_TELE_PRESS, strength);
    }

    bool zoomTeleRelease() {
        return sendCommandWithParam(Cmd::ZOOM_TELE_RELEASE, 0x00);
    }

    bool zoomWidePress(uint8_t strength) {
        if (strength < 0x10) strength = 0x10;
        if (strength > 0x8F) strength = 0x8F;
        return sendCommandWithParam(Cmd::ZOOM_WIDE_PRESS, strength);
    }

    bool zoomWideRelease() {
        return sendCommandWithParam(Cmd::ZOOM_WIDE_RELEASE, 0x00);
    }

    bool focusInPress(uint8_t strength) {
        if (strength > 0x7F) strength = 0x7F;
        return sendCommandWithParam(Cmd::FOCUS_IN_PRESS, strength);
    }

    bool focusInRelease() {
        return sendCommandWithParam(Cmd::FOCUS_IN_RELEASE, 0x00);
    }

    bool focusOutPress(uint8_t strength) {
        if (strength > 0x7F) strength = 0x7F;
        return sendCommandWithParam(Cmd::FOCUS_OUT_PRESS, strength);
    }

    bool focusOutRelease() {
        return sendCommandWithParam(Cmd::FOCUS_OUT_RELEASE, 0x00);
    }

    bool isFocusAcquired() {
        return focusStatus == Status::FOCUS_ACQUIRED;
    }

    bool isShutterActive() {
        return shutterStatus == Status::SHUTTER_ACTIVE;
    }

    bool isRecording() {
        return recordingStatus == Status::RECORDING_STARTED;
    }

    void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
        if (length < 3) return;

        // Status format: [0x02, Type, Value]
        uint8_t type = pData[1];
        uint8_t value = pData[2];

        switch (type) {
            case Status::TYPE_FOCUS: // Focus status
                focusStatus = value;
                Serial.printf("[Camera] Focus status: %s\n", 
                    value == Status::FOCUS_ACQUIRED ? "Acquired" : "Lost");
                break;

            case Status::TYPE_SHUTTER: // Shutter status
                shutterStatus = value;
                Serial.printf("[Camera] Shutter status: %s\n", 
                    value == Status::SHUTTER_ACTIVE ? "Active" : "Ready");
                break;

            case Status::TYPE_RECORDING: // Recording status
                recordingStatus = value;
                Serial.printf("[Camera] Recording status: %s\n", 
                    value == Status::RECORDING_STARTED ? "Started" : "Stopped");
                break;

            default:
                Serial.printf("[Camera] Unknown status type: 0x%02X value: 0x%02X\n", type, value);
                break;
        }
    }
}
