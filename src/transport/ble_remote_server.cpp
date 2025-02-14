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
BLERemoteServer::AstroCommandCallback BLERemoteServer::astroCommandCallback = nullptr;
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

void BLERemoteServer::setAstroCommandCallback(AstroCommandCallback callback) {
    astroCommandCallback = callback;
}

void BLERemoteServer::sendFeedback(CommandStatus status) {
    if (deviceConnected && pFeedbackChar != nullptr) {
        uint8_t value = static_cast<uint8_t>(status);
        pFeedbackChar->setValue(&value, 1);
        pFeedbackChar->notify();
    }
}

void BLERemoteServer::sendAstroStatus(const AstroStatusPacket& status) {
    if (deviceConnected && pAstroStatusChar != nullptr) {
        pAstroStatusChar->setValue((uint8_t*)&status, sizeof(AstroStatusPacket));
        pAstroStatusChar->notify();
    }
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
    if (value.length() < 2) {
        LOG_PERIPHERAL("[BLE] Invalid command length");
        sendFeedback(CommandStatus::INVALID);
        return;
    }

    RemoteCommand cmd = static_cast<RemoteCommand>(value[0]);
    ButtonId button = static_cast<ButtonId>(value[1]);

    // Handle button commands
    if (cmd == RemoteCommand::BUTTON_DOWN || cmd == RemoteCommand::BUTTON_UP) {
        if (!validateButtonTransition(cmd, button)) {
            LOG_PERIPHERAL("[BLE] Invalid button state transition");
            sendFeedback(CommandStatus::BUTTON_STATE_ERROR);
            return;
        }

        buttonStates[button] = (cmd == RemoteCommand::BUTTON_DOWN);
    }

    // Forward command to callback
    if (commandCallback) {
        commandCallback(cmd, (uint8_t*)value.data() + 1, value.length() - 1);
    }

    sendFeedback(CommandStatus::SUCCESS);
}

void BLERemoteServer::handleAstroCommand(RemoteCommand cmd, const uint8_t* data, size_t length) {
    if (!astroCommandCallback) {
        LOG_PERIPHERAL("[BLE] No astro command callback registered");
        sendFeedback(CommandStatus::FAILURE);
        return;
    }

    if (cmd == RemoteCommand::ASTRO_SET_PARAMS) {
        if (length != sizeof(AstroParamPacket)) {
            LOG_PERIPHERAL("[BLE] Invalid astro params packet size");
            sendFeedback(CommandStatus::INVALID);
            return;
        }

        const AstroParamPacket* params = reinterpret_cast<const AstroParamPacket*>(data);
        astroCommandCallback(cmd, params);
    } else {
        astroCommandCallback(cmd, nullptr);
    }

    sendFeedback(CommandStatus::SUCCESS);
}

bool BLERemoteServer::validateButtonTransition(RemoteCommand cmd, ButtonId button) {
    auto it = buttonStates.find(button);

    // If button state not found, allow only BUTTON_DOWN
    if (it == buttonStates.end()) {
        return cmd == RemoteCommand::BUTTON_DOWN;
    }

    // If button is down, allow only BUTTON_UP
    if (it->second) {
        return cmd == RemoteCommand::BUTTON_UP;
    }

    // If button is up, allow only BUTTON_DOWN
    return cmd == RemoteCommand::BUTTON_DOWN;
}
