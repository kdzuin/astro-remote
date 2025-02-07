#include "camera_commands.h"
#include "ble_device.h"
#include <M5Unified.h>
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
            M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
            M5.Display.drawString("Focus OK", 10, 200);
        }
        else
        {
            // Focus was lost
            M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
            M5.Display.drawString("No Focus", 10, 200);
        }
    }

    void handleShutterStateChange(uint8_t prevState, uint8_t newState)
    {
        if (newState == Status::SHUTTER_ACTIVE)
        {
            // Shutter just activated - could play sound or flash LED
            M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
            M5.Display.drawString("*CLICK*", 10, 220);
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

    // Basic camera control functions
    bool startFocus()
    {
        Serial.println("[Camera] Starting focus (half-press)...");
        return sendCommand16(Cmd::SHUTTER_HALF_DOWN);
    }

    bool stopFocus()
    {
        Serial.println("[Camera] Stopping focus...");
        return sendCommand16(Cmd::SHUTTER_HALF_UP);
    }

    bool pressShutter()
    {
        Serial.println("[Camera] Pressing shutter...");
        return sendCommand16(Cmd::SHUTTER_FULL_DOWN);
    }

    bool releaseShutter()
    {
        Serial.println("[Camera] Releasing shutter...");
        return sendCommand16(Cmd::SHUTTER_FULL_UP);
    }

    bool recordStart()
    {
        Serial.println("[Camera] Starting recording");
        // Press record button
        if (!sendCommand16(Cmd::RECORD_DOWN)) {
            Serial.println("[Camera] Failed to start recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP)) {
            Serial.println("[Camera] Failed to start recording - UP failed");
            return false;
        }
        return true;
    }

    bool recordStop()
    {
        Serial.println("[Camera] Stopping recording");
        // Press record button again
        if (!sendCommand16(Cmd::RECORD_DOWN)) {
            Serial.println("[Camera] Failed to stop recording - DOWN failed");
            return false;
        }
        delay(100); // Small delay between down and up
        // Release record button
        if (!sendCommand16(Cmd::RECORD_UP)) {
            Serial.println("[Camera] Failed to stop recording - UP failed");
            return false;
        }
        return true;
    }

    // Connection testing
    bool testConnection()
    {
        Serial.println("[Camera] Testing connection...");

        // First, make sure notifications are enabled
        BLERemoteCharacteristic *pStatusChar = BLEDeviceManager::getStatusCharacteristic();
        if (pStatusChar && pStatusChar->canNotify())
        {
            Serial.println("[Camera] Re-enabled notifications");
            pStatusChar->registerForNotify(onStatusNotification);
        }

        // Read current status
        BLERemoteCharacteristic *pStatusReadChar = BLEDeviceManager::getService()->getCharacteristic(SONY_REMOTE_STATUS_READ_CHARACTERISTIC_UUID);
        if (pStatusReadChar && pStatusReadChar->canRead())
        {
            std::string value = pStatusReadChar->readValue();
            Serial.print("[Camera] Current status (cc05): ");
            for (size_t i = 0; i < value.length(); i++)
            {
                Serial.printf("%02X ", (uint8_t)value[i]);
            }
            Serial.println();
        }

        // Test sequence:
        // 1. Half-press (focus)
        Serial.println("\n[Camera] Testing half-press (focus)...");
        if (!startFocus())
        {
            Serial.println("[Camera] Focus start failed");
            return false;
        }
        delay(1000); // Wait for focus

        // Check if we got any response
        if (millis() - lastMessageTime > 1000)
        {
            Serial.println("[Camera] No response to focus command");
            stopFocus();
            return false;
        }

        // Release half-press
        if (!stopFocus())
        {
            Serial.println("[Camera] Focus stop failed");
            return false;
        }
        delay(500);

        // 2. Full-press (shutter)
        Serial.println("\n[Camera] Testing full-press (shutter)...");
        if (!pressShutter())
        {
            Serial.println("[Camera] Shutter press failed");
            return false;
        }
        delay(500);

        // Release shutter
        if (!releaseShutter())
        {
            Serial.println("[Camera] Shutter release failed");
            return false;
        }

        Serial.println("[Camera] Test sequence completed");
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

    bool shutterPress(uint32_t focusTimeout)
    {
        Serial.println("[Camera] Starting shutter sequence");
        
        // 1. Focus down (half-press)
        if (!sendCommand16(Cmd::SHUTTER_HALF_DOWN)) {  // 0x0107
            Serial.println("[Camera] Failed to half-press");
            return false;
        }
        delay(100);

        // 2. Full press
        if (!sendCommand16(Cmd::SHUTTER_FULL_DOWN)) {  // 0x0109
            Serial.println("[Camera] Failed to full-press");
            return false;
        }
        delay(100);

        // 3. Release shutter
        if (!sendCommand16(Cmd::SHUTTER_FULL_UP)) {    // 0x0108
            Serial.println("[Camera] Failed to release shutter");
            return false;
        }
        delay(100);

        // 4. Release focus
        if (!sendCommand16(Cmd::SHUTTER_HALF_UP)) {    // 0x0106
            Serial.println("[Camera] Failed to release focus");
            return false;
        }

        return true;
    }

    bool shutterRelease()
    {
        // Hold focus first
        if (!stopFocus())
            return false;
        delay(10);

        // Then release shutter
        return releaseShutter();
    }

    bool focusPress()
    {
        return startFocus();
    }

    bool focusRelease()
    {
        if (!stopFocus())
            return false;
        delay(10);
        return releaseShutter();
    }

    void setFocusMode(FocusMode mode)
    {
        currentMode = mode;
        focusHeld = false;
    }

    void holdFocus(bool hold)
    {
        if (currentMode == FocusMode::MANUAL_FOCUS)
        {
            focusHeld = hold;
            if (hold)
            {
                startFocus();
            }
            else
            {
                stopFocus();
                delay(10);
                releaseShutter();
            }
        }
    }

    bool isFocusHeld()
    {
        return focusHeld;
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
                    M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    M5.Display.drawString("No Focus", 10, 200);
                }
                else if (statusValue == Status::FOCUS_ACQUIRED)
                {
                    Serial.println("[Camera] Focus acquired");
                    focusStatus = Status::FOCUS_ACQUIRED;
                    M5.Display.setTextColor(M5.Display.color565(0, 255, 0)); // Green
                    M5.Display.drawString("Focus OK", 10, 200);
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
                    M5.Display.setTextColor(M5.Display.color565(255, 255, 0)); // Yellow
                    M5.Display.drawString("*CLICK*", 10, 220);
                }
                break;

            case Status::RECORD_TYPE: // 0xD5
                if (statusValue == Status::RECORD_STOPPED)
                {
                    Serial.println("[Camera] Recording stopped");
                    recordingStatus = Status::RECORD_STOPPED;
                    M5.Display.fillRect(280, 10, 40, 20, M5.Display.color565(0, 0, 0)); // Clear REC indicator
                }
                else if (statusValue == Status::RECORD_STARTED)
                {
                    Serial.println("[Camera] Recording started");
                    recordingStatus = Status::RECORD_STARTED;
                    M5.Display.setTextColor(M5.Display.color565(255, 0, 0)); // Red
                    M5.Display.drawString("REC", 280, 10);
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
