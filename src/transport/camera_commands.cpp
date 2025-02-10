#include "camera_commands.h"
#include "ble_device.h"
#include <Arduino.h>
#include "../debug.h"

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
            LOG_PERIPHERAL("[Camera] Not connected");
            return false;
        }

        BLERemoteCharacteristic *pChar = BLEDeviceManager::getControlCharacteristic();
        if (!pChar)
        {
            LOG_PERIPHERAL("[Camera] Control characteristic not available");
            return false;
        }

        // Simple 2-byte format: [CMD_MSB CMD_LSB]
        uint8_t cmdBuffer[] = {
            (uint8_t)(cmd >> 8),  // Command MSB
            (uint8_t)(cmd & 0xFF) // Command LSB
        };

        // Debug output
        char logText[64];
        snprintf(logText, sizeof(logText), "[Camera] Sending command 0x%04X: [", cmd);
        for (size_t i = 0; i < sizeof(cmdBuffer); i++)
        {
            snprintf(logText + strlen(logText), sizeof(logText) - strlen(logText), "%02X%s", cmdBuffer[i], i < sizeof(cmdBuffer) - 1 ? " " : "");
        }
        strcat(logText, "]");
        LOG_PERIPHERAL("%s", logText);

        // Try writing with response
        try
        {
            pChar->writeValue(cmdBuffer, sizeof(cmdBuffer), true);
            LOG_PERIPHERAL("[Camera] Command 0x%04X sent successfully", cmd);
            return true;
        }
        catch (const std::exception &e)
        {
            LOG_PERIPHERAL("[Camera] Failed to send command: %s", e.what());
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
            LOG_PERIPHERAL("[Camera] Recording started");
            recordingStatus = Status::RECORD_STARTED;
        }
        else
        {
            LOG_PERIPHERAL("[Camera] Recording stopped");
            recordingStatus = Status::RECORD_STOPPED;
        }
    }

    bool takePhoto()
    {
        LOG_PERIPHERAL("[Camera] Take photo");

        // Step 1: Press shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN))
        {
            LOG_PERIPHERAL("[Camera] Failed to press shutter");
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
            LOG_PERIPHERAL("[Camera] Failed to release shutter");
            return false;
        }

        LOG_PERIPHERAL("[Camera] Photo taken successfully");
        return true;
    };

    bool takeBulb()
    {
        LOG_PERIPHERAL("[Camera] Take bulb photo");

        // Step 1: Press shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN))
        {
            LOG_PERIPHERAL("[Camera] Failed to press shutter");
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
            LOG_PERIPHERAL("[Camera] Failed to release shutter");
            return false;
        }

        LOG_PERIPHERAL("[Camera] Bulb Start/Stop command successful");
        return true;
    };

    bool recordStart()
    {
        LOG_PERIPHERAL("[Camera] Starting recording");
        // Press record button
        if (!sendCommand16(Cmd::RECORD_DOWN))
        {
            LOG_PERIPHERAL("[Camera] Failed to start recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP))
        {
            LOG_PERIPHERAL("[Camera] Failed to start recording - UP failed");
            return false;
        }
        return true;
    }

    bool recordStop()
    {
        LOG_PERIPHERAL("[Camera] Stopping recording");
        // Press record button again
        if (!sendCommand16(Cmd::RECORD_DOWN))
        {
            LOG_PERIPHERAL("[Camera] Failed to stop recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP))
        {
            LOG_PERIPHERAL("[Camera] Failed to stop recording - UP failed");
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

        LOG_PERIPHERAL("[Camera] Initialized");
    }

    void update()
    {
        // Add periodic status check
        if (millis() - lastCheck > 1000)
        { // Check every second
            lastCheck = millis();
            if (BLEDeviceManager::isConnected() && !statusNotificationEnabled)
            {
                LOG_DEBUG("[Camera] Attempting to re-enable notifications...");
                // Register for status notifications
                BLERemoteCharacteristic *pChar = BLEDeviceManager::getStatusCharacteristic();
                if (pChar)
                {
                    // Enable notifications explicitly
                    if (pChar->canNotify())
                    {
                        LOG_DEBUG("[Camera] Enabling notifications...");

                        // Try to enable notifications
                        if (!pChar->getDescriptor(BLEUUID((uint16_t)0x2902)))
                        {
                            LOG_DEBUG("[Camera] Warning: Could not find CCCD descriptor");
                        }

                        // Register callback and enable
                        pChar->registerForNotify(onStatusNotification);
                        try
                        {
                            pChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)"\x01\x00", 2, true);
                            statusNotificationEnabled = true;
                            LOG_DEBUG("[Camera] Status notifications enabled successfully");
                        }
                        catch (const std::exception &e)
                        {
                            LOG_DEBUG("[Camera] Failed to enable notifications: %s", e.what());
                        }
                    }
                    else
                    {
                        LOG_DEBUG("[Camera] Error: Status characteristic does not support notifications!");
                    }
                }
                else
                {
                    LOG_DEBUG("[Camera] Error: Status characteristic not available!");
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
        char logText[128];
        snprintf(logText, sizeof(logText), "[Camera] Raw notification received from %s: [", pChar->getUUID().toString().c_str());
        for (size_t i = 0; i < length; i++)
        {
            snprintf(logText + strlen(logText), sizeof(logText) - strlen(logText), "%02X%s", pData[i], i < length - 1 ? " " : "");
        }
        strcat(logText, "]");
        LOG_DEBUG("%s", logText);

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
                    LOG_PERIPHERAL("[Camera] Focus lost");
                    focusStatus = Status::FOCUS_LOST;
                    // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    // M5.Display.drawString("No Focus", 10, 200);
                }
                else if (statusValue == Status::FOCUS_ACQUIRED)
                {
                    LOG_PERIPHERAL("[Camera] Focus acquired");
                    focusStatus = Status::FOCUS_ACQUIRED;
                    // M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
                    // M5.Display.drawString("Focus OK", 10, 200);
                }
                break;

            case Status::SHUTTER_TYPE: // 0xA0
                if (statusValue == Status::SHUTTER_READY)
                {
                    LOG_PERIPHERAL("[Camera] Shutter ready");
                    shutterStatus = Status::SHUTTER_READY;
                }
                else if (statusValue == Status::SHUTTER_ACTIVE)
                {
                    LOG_PERIPHERAL("[Camera] Shutter active");
                    shutterStatus = Status::SHUTTER_ACTIVE;
                    // M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
                    // M5.Display.drawString("*CLICK*", 10, 220);
                }
                break;

            case Status::RECORD_TYPE: // 0xD5
                if (statusValue == Status::RECORD_STOPPED)
                {
                    LOG_PERIPHERAL("[Camera] Recording stopped");
                    recordingStatus = Status::RECORD_STOPPED;
                    // M5.Display.fillRect(280, 10, 40, 20, M5.Display.color565(0, 0, 0)); // Clear REC indicator
                }
                else if (statusValue == Status::RECORD_STARTED)
                {
                    LOG_PERIPHERAL("[Camera] Recording started");
                    recordingStatus = Status::RECORD_STARTED;
                    // M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    // M5.Display.drawString("REC", 280, 10);
                }
                break;

            default:
                LOG_DEBUG("[Camera] Unknown status type: 0x%02X value: 0x%02X", statusType, statusValue);
                break;
            }
        }
        else
        {
            // Just log other notifications for debugging
            LOG_DEBUG("[Camera] Other notification from %s, length=%d",
                      pChar->getUUID().toString().c_str(), length);
        }
    }
}
