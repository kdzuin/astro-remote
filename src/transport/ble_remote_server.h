#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <functional>
#include <map>

// Service and Characteristic UUIDs
#define REMOTE_SERVICE_UUID "180F1000-1234-5678-90AB-CDEF12345678"
#define CONTROL_CHAR_UUID "180F1001-1234-5678-90AB-CDEF12345678"
#define FEEDBACK_CHAR_UUID "180F1002-1234-5678-90AB-CDEF12345678"

// Button IDs
enum class ButtonId : uint8_t
{
    UP = 0x01,
    DOWN = 0x02,
    LEFT = 0x03,
    RIGHT = 0x04,
    CONFIRM = 0x05,
    BACK = 0x06
};

// Control commands
enum class RemoteCommand : uint8_t
{
    // Button states (0x01-0x0F)
    BUTTON_DOWN = 0x01,
    BUTTON_UP = 0x02,

    // Parameter commands (0x10-0x2F reserved for parameters)
    SET_SUBEXPOSURE_COUNT = 0x10,

    NONE = 0xFF
};

// Feedback status
enum class CommandStatus
{
    SUCCESS,
    FAILURE,
    BUSY,
    INVALID,
    BUTTON_STATE_ERROR // New status for invalid button state transitions
};

class BLERemoteServer
{
public:
    // Update callback to include parameters
    using CommandCallback = std::function<CommandStatus(RemoteCommand, const uint8_t *, size_t)>;

    static void init(const char *deviceName = "M5Remote");
    static void setCommandCallback(CommandCallback callback);
    static void sendFeedback(CommandStatus status);
    static bool isConnected();
    static void stop();

private:
    static BLEServer *pServer;
    static BLECharacteristic *pControlChar;
    static BLECharacteristic *pFeedbackChar;
    static bool deviceConnected;
    static CommandCallback onCommandReceived;

    // Track button states
    static std::map<ButtonId, bool> buttonStates;

    static class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
    } serverCallbacks;

    static class ControlCharCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic) override;
    } controlCharCallbacks;

    // Validate button state transitions
    static bool validateButtonTransition(RemoteCommand cmd, ButtonId button);
};
