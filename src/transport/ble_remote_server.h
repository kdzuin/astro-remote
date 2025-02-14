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

// Command format (16-bit base command + optional parameters)
namespace RemoteCmd {
// Command type ranges (high byte)
constexpr uint8_t TYPE_BUTTON = 0x01;  // Button commands
constexpr uint8_t TYPE_ASTRO = 0x02;   // Astro commands
constexpr uint8_t TYPE_SYSTEM = 0x03;  // System commands (future use)

// Button commands (0x01XX)
constexpr uint16_t BUTTON_DOWN = 0x0100;  // + button_id
constexpr uint16_t BUTTON_UP = 0x0101;    // + button_id

// Astro commands (0x02XX)
constexpr uint16_t ASTRO_START = 0x0200;       // No params
constexpr uint16_t ASTRO_PAUSE = 0x0201;       // No params
constexpr uint16_t ASTRO_STOP = 0x0202;        // No params
constexpr uint16_t ASTRO_RESET = 0x0203;       // No params
constexpr uint16_t ASTRO_SET_PARAMS = 0x0204;  // + AstroParamPacket

// Helper functions
constexpr uint8_t getType(uint16_t cmd) {
    return cmd >> 8;
}
constexpr uint8_t getSubCommand(uint16_t cmd) {
    return cmd & 0xFF;
}
}  // namespace RemoteCmd

// Feedback status
enum class CommandStatus {
    SUCCESS,
    FAILURE,
    BUSY,
    INVALID,
    BUTTON_STATE_ERROR,  // New status for invalid button state transitions
    ASTRO_ERROR          // New status for astro-specific errors
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
    uint8_t state;  // Maps to AstroProcess::State
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
    // Update callback to use 16-bit commands
    using CommandCallback =
        std::function<void(uint16_t cmd, const uint8_t* params, size_t paramCount)>;

    static void init(const char* deviceName = "M5Remote");
    static void setCommandCallback(CommandCallback callback);
    static void sendFeedback(CommandStatus status);
    static void sendAstroStatus(const AstroStatusPacket& status);
    static bool isConnected();
    static bool sendCommand16(uint16_t cmd);
    static bool sendCommand24(uint16_t cmd, uint8_t param);
    static bool sendCommandWithPayload(uint16_t cmd, const uint8_t* data, size_t len);
    static void stop();

private:
    static BLEServer* pServer;
    static BLEService* pService;
    static BLECharacteristic* pControlChar;
    static BLECharacteristic* pFeedbackChar;
    static BLECharacteristic* pAstroStatusChar;
    static BLECharacteristic* pAstroControlChar;
    static CommandCallback commandCallback;
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

    static void handleAstroCommand(uint16_t cmd, const uint8_t* data, size_t length);
    static bool validateButtonTransition(uint16_t cmd, ButtonId button);

    static ServerCallbacks serverCallbacks;
    static ControlCharCallbacks controlCharCallbacks;
};
