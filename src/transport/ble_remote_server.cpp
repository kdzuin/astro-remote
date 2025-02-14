#include "transport/ble_remote_server.h"

#include <BLE2902.h>
#include <BLEDevice.h>

// Static member initialization
BLEServer* BLERemoteServer::pServer = nullptr;
BLEService* BLERemoteServer::pService = nullptr;
BLECharacteristic* BLERemoteServer::pControlChar = nullptr;
BLECharacteristic* BLERemoteServer::pFeedbackChar = nullptr;
BLECharacteristic* BLERemoteServer::pAstroStatusChar = nullptr;
BLECharacteristic* BLERemoteServer::pAstroControlChar = nullptr;
BLERemoteServer::CommandCallback BLERemoteServer::commandCallback = nullptr;
bool BLERemoteServer::deviceConnected = false;
std::map<ButtonId, bool> BLERemoteServer::buttonStates;
BLERemoteServer::ServerCallbacks BLERemoteServer::serverCallbacks;
BLERemoteServer::ControlCharCallbacks BLERemoteServer::controlCharCallbacks;

void BLERemoteServer::init(const char* deviceName) {
    // Initialize BLE
    BLEDevice::init(deviceName);

    // Create server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    // Create service
    pService = pServer->createService(REMOTE_SERVICE_UUID);

    // Create characteristics
    pControlChar =
        pService->createCharacteristic(CONTROL_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pControlChar->setCallbacks(&controlCharCallbacks);

    pFeedbackChar =
        pService->createCharacteristic(FEEDBACK_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    pFeedbackChar->addDescriptor(new BLE2902());

    pAstroStatusChar =
        pService->createCharacteristic(ASTRO_STATUS_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    pAstroStatusChar->addDescriptor(new BLE2902());

    pAstroControlChar =
        pService->createCharacteristic(ASTRO_CONTROL_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    pAstroControlChar->setCallbacks(&controlCharCallbacks);

    // Start service and advertising
    pService->start();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(REMOTE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    LOG_PERIPHERAL("[BLE] Server initialized");
}

void BLERemoteServer::setCommandCallback(CommandCallback callback) {
    commandCallback = callback;
}

void BLERemoteServer::sendFeedback(CommandStatus status) {
    if (!pFeedbackChar || !deviceConnected) {
        return;
    }

    uint8_t value = static_cast<uint8_t>(status);
    pFeedbackChar->setValue(&value, 1);
    pFeedbackChar->notify();
}

void BLERemoteServer::sendAstroStatus(const AstroStatusPacket& status) {
    if (!pAstroStatusChar || !deviceConnected) {
        return;
    }

    pAstroStatusChar->setValue((uint8_t*)&status, sizeof(status));
    pAstroStatusChar->notify();
}

bool BLERemoteServer::isConnected() {
    return deviceConnected;
}

void BLERemoteServer::stop() {
    if (pServer != nullptr) {
        pServer->getAdvertising()->stop();
        BLEDevice::deinit(true);
        pServer = nullptr;
        pService = nullptr;
        pControlChar = nullptr;
        pFeedbackChar = nullptr;
        pAstroStatusChar = nullptr;
        pAstroControlChar = nullptr;
        deviceConnected = false;
        LOG_PERIPHERAL("[BLE] Server stopped");
    }
}

void BLERemoteServer::ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    LOG_PERIPHERAL("[BLE] Client connected");
}

void BLERemoteServer::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    buttonStates.clear();
    pServer->startAdvertising();
    LOG_PERIPHERAL("[BLE] Client disconnected");
}

void BLERemoteServer::ControlCharCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() < 2) {  // Need at least command word
        LOG_PERIPHERAL("[BLE] Invalid command length");
        sendFeedback(CommandStatus::INVALID);
        return;
    }

    // Extract 16-bit command from first two bytes
    uint16_t cmd = (static_cast<uint16_t>(static_cast<uint8_t>(value[0])) << 8) | 
                   static_cast<uint8_t>(value[1]);
    uint8_t cmdType = RemoteCmd::getType(cmd);
    LOG_PERIPHERAL("[BLE] Received command: 0x%04X (type: 0x%02X)", cmd, cmdType);
    
    switch (cmdType) {
        case RemoteCmd::TYPE_BUTTON: {
            if (value.length() != 3) {  // Command word + button ID
                LOG_PERIPHERAL("[BLE] Invalid button command length");
                sendFeedback(CommandStatus::INVALID);
                return;
            }

            ButtonId button = static_cast<ButtonId>(value[2]);
            LOG_PERIPHERAL("[BLE] Button command: %s, Button: 0x%02X", 
                          (cmd == RemoteCmd::BUTTON_DOWN ? "DOWN" : "UP"),
                          static_cast<uint8_t>(button));

            // Validate button ID
            if (static_cast<uint8_t>(button) < static_cast<uint8_t>(ButtonId::UP) ||
                static_cast<uint8_t>(button) > static_cast<uint8_t>(ButtonId::BTN_EMERGENCY)) {
                LOG_PERIPHERAL("[BLE] Invalid button ID");
                sendFeedback(CommandStatus::INVALID);
                return;
            }

            if (!validateButtonTransition(cmd, button)) {
                LOG_PERIPHERAL("[BLE] Invalid button state transition");
                sendFeedback(CommandStatus::BUTTON_STATE_ERROR);
                return;
            }

            buttonStates[button] = (cmd == RemoteCmd::BUTTON_DOWN);
            RemoteControlManager::setButtonState(button, cmd == RemoteCmd::BUTTON_DOWN);
            LOG_PERIPHERAL("[BLE] Button state updated");

            // Forward command to callback
            if (commandCallback) {
                const uint8_t* paramPtr = reinterpret_cast<const uint8_t*>(&value[2]);
                commandCallback(cmd, paramPtr, 1);  // Pass button ID as parameter
            }
            break;
        }

        case RemoteCmd::TYPE_ASTRO: {
            const uint8_t* paramPtr = value.length() > 2 ? 
                reinterpret_cast<const uint8_t*>(&value[2]) : nullptr;
            size_t paramLen = value.length() > 2 ? value.length() - 2 : 0;
            
            handleAstroCommand(cmd, paramPtr, paramLen);
            return;
        }

        default:
            LOG_PERIPHERAL("[BLE] Unknown command type: 0x%02X", cmdType);
            sendFeedback(CommandStatus::INVALID);
            return;
    }

    sendFeedback(CommandStatus::SUCCESS);
}

void BLERemoteServer::handleAstroCommand(uint16_t cmd, const uint8_t* data, size_t length) {
    LOG_PERIPHERAL("[BLE] Processing astro command: 0x%04X", cmd);

    switch (cmd) {
        case RemoteCmd::ASTRO_SET_PARAMS:
            if (length != sizeof(AstroParamPacket)) {
                LOG_PERIPHERAL("[BLE] Invalid astro params size");
                sendFeedback(CommandStatus::INVALID);
                return;
            }
            break;

        case RemoteCmd::ASTRO_START:
        case RemoteCmd::ASTRO_PAUSE:
        case RemoteCmd::ASTRO_STOP:
        case RemoteCmd::ASTRO_RESET:
            if (length != 0) {
                LOG_PERIPHERAL("[BLE] Unexpected parameters for command");
                sendFeedback(CommandStatus::INVALID);
                return;
            }
            break;

        default:
            LOG_PERIPHERAL("[BLE] Unknown astro command: 0x%04X", cmd);
            sendFeedback(CommandStatus::INVALID);
            return;
    }

    if (commandCallback) {
        commandCallback(cmd, data, length);
    }

    sendFeedback(CommandStatus::SUCCESS);
}

bool BLERemoteServer::validateButtonTransition(uint16_t cmd, ButtonId button) {
    bool isDown = (cmd == RemoteCmd::BUTTON_DOWN);
    bool currentlyPressed = buttonStates[button];

    // Prevent duplicate press/release
    if (isDown == currentlyPressed) {
        LOG_PERIPHERAL("[BLE] Invalid button transition: already in state %s", 
                      isDown ? "pressed" : "released");
        return false;
    }

    return true;
}
