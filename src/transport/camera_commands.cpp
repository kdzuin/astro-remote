#include "transport/camera_commands.h"

#include <Arduino.h>

#include "transport/ble_device.h"

namespace CameraCommands {
// Static variables
static uint8_t focusStatus = Status::FOCUS_LOST;
static uint8_t shutterStatus = Status::SHUTTER_READY;
static uint8_t recordingStatus = Status::RECORD_STOPPED;
static FocusMode currentMode = FocusMode::AUTO_FOCUS;
static bool focusHeld = false;
static uint32_t lastMessageTime = 0;
static uint32_t lastCheck = 0;
static bool statusNotificationEnabled = false;

// Command sending implementation
bool sendCommand16(uint16_t cmd) {
    if (!BLEDeviceManager::isConnected()) {
        LOG_PERIPHERAL("[Camera] Not connected");
        return false;
    }

    BLERemoteCharacteristic* pChar = BLEDeviceManager::getControlCharacteristic();
    if (!pChar) {
        LOG_PERIPHERAL("[Camera] Control characteristic not available");
        return false;
    }

    // Simple 2-byte format: [CMD_MSB CMD_LSB]
    uint8_t cmdBuffer[] = {
        (uint8_t)(cmd >> 8),   // Command MSB
        (uint8_t)(cmd & 0xFF)  // Command LSB
    };

    // Debug output
    char logText[64];
    snprintf(logText, sizeof(logText), "[Camera] Sending command 0x%04X: [", cmd);
    for (size_t i = 0; i < sizeof(cmdBuffer); i++) {
        snprintf(logText + strlen(logText), sizeof(logText) - strlen(logText), "%02X%s",
                 cmdBuffer[i], i < sizeof(cmdBuffer) - 1 ? " " : "");
    }
    strcat(logText, "]");
    LOG_PERIPHERAL("%s", logText);

    // Try writing with response
    try {
        pChar->writeValue(cmdBuffer, sizeof(cmdBuffer), true);
        LOG_PERIPHERAL("[Camera] Command 0x%04X sent successfully", cmd);
        return true;
    } catch (const std::exception& e) {
        LOG_PERIPHERAL("[Camera] Failed to send command: %s", e.what());
        return false;
    }
}

bool sendCommand24(uint16_t cmd, uint8_t param) {
    if (!BLEDeviceManager::isConnected()) {
        LOG_PERIPHERAL("[Camera] Not connected");
        return false;
    }

    BLERemoteCharacteristic* pChar = BLEDeviceManager::getControlCharacteristic();
    if (!pChar) {
        LOG_PERIPHERAL("[Camera] Control characteristic not available");
        return false;
    }

    uint8_t cmdBuffer[] = {
        (uint8_t)(cmd >> 8),    // Command MSB
        (uint8_t)(cmd & 0xFF),  // Command LSB
        param                   // Parameter
    };

    // Debug output
    char logText[128];
    snprintf(logText, sizeof(logText), "[Camera] Sending command [0x%02X 0x%02X 0x%02X]",
             cmdBuffer[0], cmdBuffer[1], cmdBuffer[2]);
    LOG_PERIPHERAL("%s", logText);

    try {
        pChar->writeValue(cmdBuffer, sizeof(cmdBuffer), true);
    } catch (const std::exception& e) {
        LOG_PERIPHERAL("[Camera] Error sending command: %s", e.what());
        return false;
    }

    return true;
}

// State change handlers
void handleFocusStateChange(uint8_t prevState, uint8_t newState) {
    if (newState == Status::FOCUS_ACQUIRED) {
        // Focus was just acquired - could trigger LED or display update
        // M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
        // M5.Display.drawString("Focus OK", 10, 200);
    } else {
        // Focus was lost
        // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
        // M5.Display.drawString("No Focus", 10, 200);
    }
}

void handleShutterStateChange(uint8_t prevState, uint8_t newState) {
    if (newState == Status::SHUTTER_ACTIVE) {
        // Shutter just activated - could play sound or flash LED
        // M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
        // M5.Display.drawString("*CLICK*", 10, 220);
    }
}

void handleRecordingStateChange(uint8_t prevState, uint8_t newState) {
    if (newState == Status::RECORD_STARTED) {
        LOG_PERIPHERAL("[Camera] Recording started");
        recordingStatus = Status::RECORD_STARTED;
    } else {
        LOG_PERIPHERAL("[Camera] Recording stopped");
        recordingStatus = Status::RECORD_STOPPED;
    }
}

bool takePhoto() {
    LOG_PERIPHERAL("[Camera] Take photo");

    // Step 1: Press shutter
    if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN)) {
        LOG_PERIPHERAL("[Camera] Failed to press shutter");
        return false;
    }

    // Step 2: Wait for shutter active notification
    while (!isShutterActive()) {
        delay(10);
    }

    // Step 3: Wait for shutter ready notification
    while (isShutterActive()) {
        delay(10);
    }

    // Step 4: Release shutter
    if (!sendCommand16(Cmd::SHUTTER_FULL_UP)) {
        LOG_PERIPHERAL("[Camera] Failed to release shutter");
        return false;
    }

    LOG_PERIPHERAL("[Camera] Photo taken successfully");
    return true;
};

bool takeBulb() {
    LOG_PERIPHERAL("[Camera] Take bulb photo");

    // Step 1: Press shutter
    if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN)) {
        LOG_PERIPHERAL("[Camera] Failed to press shutter");
        return false;
    }

    // Step 2: Wait for shutter active notification
    while (!isShutterActive()) {
        delay(10);
    }

    // Step 4: Release shutter
    if (!sendCommand16(Cmd::SHUTTER_FULL_UP)) {
        LOG_PERIPHERAL("[Camera] Failed to release shutter");
        return false;
    }

    LOG_PERIPHERAL("[Camera] Bulb Start/Stop command successful");
    return true;
};

bool recordStart() {
    LOG_PERIPHERAL("[Camera] Starting recording");
    // Press record button
    if (!sendCommand16(Cmd::RECORD_DOWN)) {
        LOG_PERIPHERAL("[Camera] Failed to start recording - DOWN failed");
        return false;
    }
    delay(100);  // Small delay between down and up
    // Release record button
    if (!sendCommand16(Cmd::RECORD_UP)) {
        LOG_PERIPHERAL("[Camera] Failed to start recording - UP failed");
        return false;
    }
    return true;
}

bool recordStop() {
    LOG_PERIPHERAL("[Camera] Stopping recording");
    // Press record button again
    if (!sendCommand16(Cmd::RECORD_DOWN)) {
        LOG_PERIPHERAL("[Camera] Failed to stop recording - DOWN failed");
        return false;
    }
    delay(100);  // Small delay between down and up
    // Release record button
    if (!sendCommand16(Cmd::RECORD_UP)) {
        LOG_PERIPHERAL("[Camera] Failed to stop recording - UP failed");
        return false;
    }
    return true;
}

bool zoomOut(uint8_t sensitivity) {
    // Send zoom out press command
    if (!sendCommand24(Cmd::ZOOM_WIDE_PRESS, sensitivity)) {
        LOG_PERIPHERAL("[Camera] Failed to send zoom out press command");
        return false;
    }

    delay(30);  // This delay determines the length of the change with the specified sensitivity

    // Send zoom out release command
    if (!sendCommand24(Cmd::ZOOM_WIDE_RELEASE, 0x00)) {
        LOG_PERIPHERAL("[Camera] Failed to send zoom out release command");
        return false;
    }

    return true;
}

bool zoomIn(uint8_t sensitivity) {
    // Send zoom in press command
    if (!sendCommand24(Cmd::ZOOM_TELE_PRESS, sensitivity)) {
        LOG_PERIPHERAL("[Camera] Failed to send zoom in press command");
        return false;
    }

    delay(30);  // This delay determines the length of the change with the specified sensitivity

    // Send zoom in release command
    if (!sendCommand24(Cmd::ZOOM_TELE_RELEASE, 0x00)) {
        LOG_PERIPHERAL("[Camera] Failed to send zoom in release command");
        return false;
    }

    return true;
}

bool focusIn(uint8_t sensitivity) {
    LOG_PERIPHERAL("[Camera] Sending focus in command");

    if (!sendCommand24(Cmd::FOCUS_IN_PRESS, sensitivity)) {
        LOG_PERIPHERAL("[Camera] Failed to send focus in press command");
        return false;
    }

    delay(30);  // This delay determines the length of the change with the specified sensitivity

    if (!sendCommand24(Cmd::FOCUS_IN_RELEASE, 0x00)) {
        LOG_PERIPHERAL("[Camera] Failed to send focus in release command");
        return false;
    }

    return true;
}

bool focusOut(uint8_t sensitivity) {
    LOG_PERIPHERAL("[Camera] Sending focus out command");

    if (!sendCommand24(Cmd::FOCUS_OUT_PRESS, sensitivity)) {
        LOG_PERIPHERAL("[Camera] Failed to send focus out press command");
        return false;
    }

    delay(30);  // This delay determines the length of the change with the specified sensitivity

    if (!sendCommand24(Cmd::FOCUS_OUT_RELEASE, 0x00)) {
        LOG_PERIPHERAL("[Camera] Failed to send focus out release command");
        return false;
    }

    return true;
}

// Emergency stop for zoom/focus
bool emergencyStop() {
    LOG_PERIPHERAL("[Camera] Emergency stop - sending all possible release commands");

    // Send all possible release commands
    const uint16_t RELEASE_COMMANDS[] = {Cmd::SHUTTER_HALF_UP,
                                         Cmd::SHUTTER_FULL_UP,
                                         Cmd::RECORD_UP,
                                         Cmd::AF_ON_UP,
                                         Cmd::C1_UP,
                                         Cmd::ZOOM_TELE_RELEASE,
                                         Cmd::ZOOM_WIDE_RELEASE,
                                         Cmd::FOCUS_IN_RELEASE,
                                         Cmd::FOCUS_OUT_RELEASE};

    for (uint16_t cmd : RELEASE_COMMANDS) {
        LOG_PERIPHERAL("[Camera] Sending release command 0x%04X", cmd);
        if (!sendCommand24(cmd, 0x00)) {
            LOG_PERIPHERAL("[Camera] Failed to send release command");
        }
        delay(100);
    }

    return true;
}

void init() {
    // Initialize status variables
    focusStatus = Status::FOCUS_LOST;
    shutterStatus = Status::SHUTTER_READY;
    recordingStatus = Status::RECORD_STOPPED;
    statusNotificationEnabled = false;
    focusHeld = false;
    currentMode = FocusMode::AUTO_FOCUS;
    lastMessageTime = 0;

    LOG_PERIPHERAL("[Camera] Initialized");
}

void update() {
    // Add periodic status check
    if (millis() - lastCheck > 1000) {  // Check every second
        lastCheck = millis();
        if (BLEDeviceManager::isConnected() && !statusNotificationEnabled) {
            LOG_DEBUG("[Camera] Attempting to re-enable notifications...");
            // Register for status notifications
            BLERemoteCharacteristic* pChar = BLEDeviceManager::getStatusCharacteristic();
            if (pChar) {
                // Enable notifications explicitly
                if (pChar->canNotify()) {
                    LOG_DEBUG("[Camera] Enabling notifications...");

                    // Try to enable notifications
                    if (!pChar->getDescriptor(BLEUUID((uint16_t)0x2902))) {
                        LOG_DEBUG("[Camera] Warning: Could not find CCCD descriptor");
                    }

                    // Register callback and enable
                    pChar->registerForNotify(onStatusNotification);
                    try {
                        pChar->getDescriptor(BLEUUID((uint16_t)0x2902))
                            ->writeValue((uint8_t*)"\x01\x00", 2, true);
                        statusNotificationEnabled = true;
                        LOG_DEBUG("[Camera] Status notifications enabled successfully");
                    } catch (const std::exception& e) {
                        LOG_DEBUG("[Camera] Failed to enable notifications: %s", e.what());
                    }
                } else {
                    LOG_DEBUG(
                        "[Camera] Error: Status characteristic does not support notifications!");
                }
            } else {
                LOG_DEBUG("[Camera] Error: Status characteristic not available!");
            }
        }
    }
}

bool isFocusAcquired() {
    return focusStatus == Status::FOCUS_ACQUIRED;
}

bool isShutterActive() {
    return shutterStatus == Status::SHUTTER_ACTIVE;
}

bool isRecording() {
    return recordingStatus == Status::RECORD_STARTED;
}

uint32_t getLastMessageTime() {
    return lastMessageTime;
}

void onStatusNotification(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length,
                          bool isNotify) {
    // Debug: Print raw notification data
    char logText[128];
    snprintf(logText, sizeof(logText), "[Camera] Raw notification received from %s: [",
             pChar->getUUID().toString().c_str());
    for (size_t i = 0; i < length; i++) {
        snprintf(logText + strlen(logText), sizeof(logText) - strlen(logText), "%02X%s", pData[i],
                 i < length - 1 ? " " : "");
    }
    strcat(logText, "]");
    LOG_DEBUG("%s", logText);

    // Update last message time
    lastMessageTime = millis();

    // Process RemoteCommand 0xFF02 responses
    if (length >= 3 && pData[0] == 0x02) {
        uint8_t statusType = pData[1];
        uint8_t statusValue = pData[2];

        switch (statusType) {
            case Status::FOCUS_TYPE:  // 0x3F
                if (statusValue == Status::FOCUS_LOST) {
                    LOG_PERIPHERAL("[Camera] Focus lost");
                    focusStatus = Status::FOCUS_LOST;
                } else if (statusValue == Status::FOCUS_ACQUIRED) {
                    LOG_PERIPHERAL("[Camera] Focus acquired");
                    focusStatus = Status::FOCUS_ACQUIRED;
                }
                break;

            case Status::SHUTTER_TYPE:  // 0xA0
                if (statusValue == Status::SHUTTER_READY) {
                    LOG_PERIPHERAL("[Camera] Shutter ready");
                    shutterStatus = Status::SHUTTER_READY;
                } else if (statusValue == Status::SHUTTER_ACTIVE) {
                    LOG_PERIPHERAL("[Camera] Shutter active");
                    shutterStatus = Status::SHUTTER_ACTIVE;
                }
                break;

            case Status::RECORD_TYPE:  // 0xD5
                if (statusValue == Status::RECORD_STOPPED) {
                    LOG_PERIPHERAL("[Camera] Recording stopped");
                    recordingStatus = Status::RECORD_STOPPED;
                } else if (statusValue == Status::RECORD_STARTED) {
                    LOG_PERIPHERAL("[Camera] Recording started");
                    recordingStatus = Status::RECORD_STARTED;
                }
                break;

            default:
                LOG_DEBUG("[Camera] Unknown status type: 0x%02X value: 0x%02X", statusType,
                          statusValue);
                break;
        }
    } else {
        // Just log other notifications for debugging
        LOG_DEBUG("[Camera] Other notification from %s, length=%d",
                  pChar->getUUID().toString().c_str(), length);
    }
}
}  // namespace CameraCommands
