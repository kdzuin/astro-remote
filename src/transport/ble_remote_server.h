#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <functional>
#include <map>

#include "button_id.h"
#include "remote_control_manager.h"

// Service and Characteristic UUIDs
#define REMOTE_SERVICE_UUID "180F1000-1234-5678-90AB-CDEF12345678"
#define CONTROL_CHAR_UUID "180F1001-1234-5678-90AB-CDEF12345678"
#define FEEDBACK_CHAR_UUID "180F1002-1234-5678-90AB-CDEF12345678"
#define ASTRO_STATUS_CHAR_UUID "180F1003-1234-5678-90AB-CDEF12345678"
#define ASTRO_CONTROL_CHAR_UUID "180F1004-1234-5678-90AB-CDEF12345678"

// Control commands
enum class RemoteCommand : uint8_t {
    // Button states (0x01-0x0F)
    BUTTON_DOWN = 0x01,
    BUTTON_UP = 0x02,

    // Parameter commands (0x10-0x2F reserved for parameters)
    SET_SUBEXPOSURE_COUNT = 0x10,

    // Astro commands (0x30-0x3F)
    ASTRO_START = 0x30,
    ASTRO_PAUSE = 0x31,
    ASTRO_STOP = 0x32,
    ASTRO_RESET = 0x33,
    ASTRO_SET_PARAMS = 0x34,

    NONE = 0xFF
};

// Feedback status
enum class CommandStatus {
    SUCCESS,
    FAILURE,
    BUSY,
    INVALID,
    BUTTON_STATE_ERROR,  // New status for invalid button state transitions
    ASTRO_ERROR         // New status for astro-specific errors
};

// Astro parameter packet structure (sent with ASTRO_SET_PARAMS)
struct __attribute__((packed)) AstroParamPacket {
    uint16_t initialDelaySec;
    uint16_t exposureSec;
    uint16_t subframeCount;
    uint16_t intervalSec;
};

// Astro status packet structure (sent via notification)
struct __attribute__((packed)) AstroStatusPacket {
    uint8_t state;             // Maps to AstroProcess::State
    uint16_t completedFrames;
    uint32_t sequenceStartTime;
    uint32_t currentFrameStartTime;
    uint32_t elapsedSec;
    uint32_t remainingSec;
    uint8_t isCameraConnected;
    uint8_t errorCode;
};

class BLERemoteServer {
public:
    // Update callback to include parameters
    using CommandCallback =
        std::function<void(RemoteCommand cmd, const uint8_t* params, size_t paramCount)>;
    using AstroCommandCallback = 
        std::function<void(RemoteCommand cmd, const AstroParamPacket* params)>;

    static void init(const char* deviceName = "M5Remote");
    static void setCommandCallback(CommandCallback callback);
    static void setAstroCommandCallback(AstroCommandCallback callback);
    static void sendFeedback(CommandStatus status);
    static void sendAstroStatus(const AstroStatusPacket& status);
    static bool isConnected();
    static void stop();

private:
    static BLEServer* pServer;
    static BLEService* pService;
    static BLECharacteristic* pControlChar;
    static BLECharacteristic* pFeedbackChar;
    static BLECharacteristic* pAstroStatusChar;
    static BLECharacteristic* pAstroControlChar;
    static CommandCallback commandCallback;
    static AstroCommandCallback astroCommandCallback;
    static bool deviceConnected;
    static std::map<ButtonId, bool> buttonStates;

    // BLE callbacks
    class ServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };

    class ControlCharCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };

    class AstroControlCharCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };

    static void handleAstroCommand(RemoteCommand cmd, const uint8_t* data, size_t length);
    static bool validateButtonTransition(RemoteCommand cmd, ButtonId button);

    static ServerCallbacks serverCallbacks;
    static ControlCharCallbacks controlCharCallbacks;
};
