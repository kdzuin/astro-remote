#include "camera_commands.h"
#include "ble_device.h"
#include <Arduino.h>

namespace CameraCommands
{
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
    bool sendCommand16(uint16_t cmd)
    {
        if (!BLEDeviceManager::isConnected())
        {
            Serial.println("[Camera] Not connected");
            return false;
        }

        BLERemoteCharacteristic *pChar = BLEDeviceManager::getControlCharacteristic();
        if (!pChar)
        {
            Serial.println("[Camera] Control characteristic not available");
            return false;
        }

        // Simple 2-byte format: [CMD_MSB CMD_LSB]
        uint8_t cmdBuffer[] = {
            (uint8_t)(cmd >> 8),  // Command MSB
            (uint8_t)(cmd & 0xFF) // Command LSB
        };

        // Debug output
        Serial.print("[Camera] Sending command bytes: [");
        for (size_t i = 0; i < sizeof(cmdBuffer); i++)
        {
            Serial.printf("%02X%s", cmdBuffer[i], i < sizeof(cmdBuffer) - 1 ? " " : "");
        }
        Serial.println("]");

        // Try writing with response
        try
        {
            pChar->writeValue(cmdBuffer, sizeof(cmdBuffer), true);
            Serial.printf("[Camera] Command 0x%04X sent successfully\n", cmd);
            return true;
        }
        catch (const std::exception &e)
        {
            Serial.printf("[Camera] Failed to send command: %s\n", e.what());
            return false;
        }
    }

    // State change handlers
    void handleFocusStateChange(uint8_t prevState, uint8_t newState)
    {
        if (newState == Status::FOCUS_ACQUIRED)
        {
            // Focus was just acquired - could trigger LED or display update
            // M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
            // M5.Display.drawString("Focus OK", 10, 200);
        }
        else
        {
            // Focus was lost
            // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
            // M5.Display.drawString("No Focus", 10, 200);
        }
    }

    void handleShutterStateChange(uint8_t prevState, uint8_t newState)
    {
        if (newState == Status::SHUTTER_ACTIVE)
        {
            // Shutter just activated - could play sound or flash LED
            // M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
            // M5.Display.drawString("*CLICK*", 10, 220);
        }
    }

    void handleRecordingStateChange(uint8_t prevState, uint8_t newState)
    {
        if (newState == Status::RECORD_STARTED)
        {
            Serial.println("[Camera] Recording started");
            recordingStatus = Status::RECORD_STARTED;
        }
        else
        {
            Serial.println("[Camera] Recording stopped");
            recordingStatus = Status::RECORD_STOPPED;
        }
    }

    bool takePhoto()
    {
        Serial.println("[Camera] Take photo");

        // Step 1: Press shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN))
        {
            Serial.println("[Camera] Failed to press shutter");
            return false;
        }

        // Step 2: Wait for shutter active notification
        while (!isShutterActive())
        {
            delay(10);
        }

        // Step 3: Wait for shutter ready notification
        while (isShutterActive())
        {
            delay(10);
        }

        // Step 4: Release shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_UP))
        {
            Serial.println("[Camera] Failed to release shutter");
            return false;
        }

        Serial.println("[Camera] Photo taken successfully");
        return true;
    };

    bool takeBulb()
    {
        Serial.println("[Camera] Take bulb photo");

        // Step 1: Press shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN))
        {
            Serial.println("[Camera] Failed to press shutter");
            return false;
        }

        // Step 2: Wait for shutter active notification
        while (!isShutterActive())
        {
            delay(10);
        }

        // Step 4: Release shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_UP))
        {
            Serial.println("[Camera] Failed to release shutter");
            return false;
        }

        Serial.println("[Camera] Bulb Start/Stop command successful");
        return true;
    };

    bool recordStart()
    {
        Serial.println("[Camera] Starting recording");
        // Press record button
        if (!sendCommand16(Cmd::RECORD_DOWN))
        {
            Serial.println("[Camera] Failed to start recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP))
        {
            Serial.println("[Camera] Failed to start recording - UP failed");
            return false;
        }
        return true;
    }

    bool recordStop()
    {
        Serial.println("[Camera] Stopping recording");
        // Press record button again
        if (!sendCommand16(Cmd::RECORD_DOWN))
        {
            Serial.println("[Camera] Failed to stop recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP))
        {
            Serial.println("[Camera] Failed to stop recording - UP failed");
            return false;
        }
        return true;
    }

    void init()
    {
        // Initialize status variables
        focusStatus = Status::FOCUS_LOST;
        shutterStatus = Status::SHUTTER_READY;
        recordingStatus = Status::RECORD_STOPPED;
        statusNotificationEnabled = false;
        focusHeld = false;
        currentMode = FocusMode::AUTO_FOCUS;
        lastMessageTime = 0;

        Serial.println("[Camera] Initialized");
    }

    void update()
    {
        // Add periodic status check
        if (millis() - lastCheck > 1000)
        { // Check every second
            lastCheck = millis();
            if (BLEDeviceManager::isConnected() && !statusNotificationEnabled)
            {
                Serial.println("[Camera] Attempting to re-enable notifications...");
                // Register for status notifications
                BLERemoteCharacteristic *pChar = BLEDeviceManager::getStatusCharacteristic();
                if (pChar)
                {
                    // Enable notifications explicitly
                    if (pChar->canNotify())
                    {
                        Serial.println("[Camera] Enabling notifications...");

                        // Try to enable notifications
                        if (!pChar->getDescriptor(BLEUUID((uint16_t)0x2902)))
                        {
                            Serial.println("[Camera] Warning: Could not find CCCD descriptor");
                        }

                        // Register callback and enable
                        pChar->registerForNotify(onStatusNotification);
                        try
                        {
                            pChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)"\x01\x00", 2, true);
                            statusNotificationEnabled = true;
                            Serial.println("[Camera] Status notifications enabled successfully");
                        }
                        catch (const std::exception &e)
                        {
                            Serial.printf("[Camera] Failed to enable notifications: %s\n", e.what());
                        }
                    }
                    else
                    {
                        Serial.println("[Camera] Error: Status characteristic does not support notifications!");
                    }
                }
                else
                {
                    Serial.println("[Camera] Error: Status characteristic not available!");
                }
            }
        }
    }

    bool isFocusAcquired()
    {
        return focusStatus == Status::FOCUS_ACQUIRED;
    }

    bool isShutterActive()
    {
        return shutterStatus == Status::SHUTTER_ACTIVE;
    }

    bool isRecording()
    {
        return recordingStatus == Status::RECORD_STARTED;
    }

    uint32_t getLastMessageTime()
    {
        return lastMessageTime;
    }

    void onStatusNotification(BLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify)
    {
        // Debug: Print raw notification data
        Serial.print("[Camera] Raw notification received from ");
        Serial.print(pChar->getUUID().toString().c_str());
        Serial.print(": [");
        for (size_t i = 0; i < length; i++)
        {
            Serial.printf("%02X%s", pData[i], i < length - 1 ? " " : "");
        }
        Serial.println("]");

        // Update last message time
        lastMessageTime = millis();

        // Process RemoteCommand 0xFF02 responses
        if (length >= 3 && pData[0] == 0x02)
        {
            uint8_t statusType = pData[1];
            uint8_t statusValue = pData[2];

            switch (statusType)
            {
            case Status::FOCUS_TYPE: // 0x3F
                if (statusValue == Status::FOCUS_LOST)
                {
                    Serial.println("[Camera] Focus lost");
                    focusStatus = Status::FOCUS_LOST;
                    // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    // M5.Display.drawString("No Focus", 10, 200);
                }
                else if (statusValue == Status::FOCUS_ACQUIRED)
                {
                    Serial.println("[Camera] Focus acquired");
                    focusStatus = Status::FOCUS_ACQUIRED;
                    // M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
                    // M5.Display.drawString("Focus OK", 10, 200);
                }
                break;

            case Status::SHUTTER_TYPE: // 0xA0
                if (statusValue == Status::SHUTTER_READY)
                {
                    Serial.println("[Camera] Shutter ready");
                    shutterStatus = Status::SHUTTER_READY;
                }
                else if (statusValue == Status::SHUTTER_ACTIVE)
                {
                    Serial.println("[Camera] Shutter active");
                    shutterStatus = Status::SHUTTER_ACTIVE;
                    // M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
                    // M5.Display.drawString("*CLICK*", 10, 220);
                }
                break;

            case Status::RECORD_TYPE: // 0xD5
                if (statusValue == Status::RECORD_STOPPED)
                {
                    Serial.println("[Camera] Recording stopped");
                    recordingStatus = Status::RECORD_STOPPED;
                    // M5.Display.fillRect(280, 10, 40, 20, M5.Display.color565(0, 0, 0)); // Clear REC indicator
                }
                else if (statusValue == Status::RECORD_STARTED)
                {
                    Serial.println("[Camera] Recording started");
                    recordingStatus = Status::RECORD_STARTED;
                    // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    // M5.Display.drawString("REC", 280, 10);
                }
                break;

            default:
                Serial.printf("[Camera] Unknown status type: 0x%02X value: 0x%02X\n", statusType, statusValue);
                break;
            }
        }
        else
        {
            // Just log other notifications for debugging
            Serial.printf("[Camera] Other notification from %s, length=%d\n",
                          pChar->getUUID().toString().c_str(), length);
        }
    }
}
