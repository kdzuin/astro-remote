#include "camera_commands.h"

namespace {
    uint8_t focusStatus = CameraCommands::Status::FOCUS_LOST;
    uint8_t shutterStatus = CameraCommands::Status::SHUTTER_READY;
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
        if (!shutterHalfRelease()) return false;
        if (!shutterFullRelease()) return false;
        return true;
    }

    bool afPress() {
        return sendCommand(Cmd::AF_ON_DOWN);
    }

    bool afRelease() {
        return sendCommand(Cmd::AF_ON_UP);
    }

    bool focusInPress(uint8_t strength) {
        return sendCommandWithParam(Cmd::FOCUS_IN_PRESS, strength);
    }

    bool focusInRelease() {
        return sendCommandWithParam(Cmd::FOCUS_IN_RELEASE, 0x00);
    }

    bool focusOutPress(uint8_t strength) {
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

    void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
        if (length < 3) return;

        // Status format: [0x02, Type, Value]
        uint8_t type = pData[1];
        uint8_t value = pData[2];

        switch (type) {
            case 0x3F: // Focus status
                focusStatus = value;
                Serial.printf("[Camera] Focus status: %s\n", 
                    value == Status::FOCUS_ACQUIRED ? "Acquired" : "Lost");
                break;

            case 0xA0: // Shutter status
                shutterStatus = value;
                Serial.printf("[Camera] Shutter status: %s\n", 
                    value == Status::SHUTTER_ACTIVE ? "Active" : "Ready");
                break;

            default:
                Serial.printf("[Camera] Unknown status type: 0x%02X value: 0x%02X\n", type, value);
                break;
        }
    }
}
