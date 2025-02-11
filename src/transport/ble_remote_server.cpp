#include "ble_remote_server.h"
#include "../debug.h"
#include <BLEDevice.h>
#include <BLE2902.h>

// Initialize static members
BLEServer* BLERemoteServer::pServer = nullptr;
BLECharacteristic* BLERemoteServer::pControlChar = nullptr;
BLECharacteristic* BLERemoteServer::pFeedbackChar = nullptr;
bool BLERemoteServer::deviceConnected = false;
BLERemoteServer::CommandCallback BLERemoteServer::onCommandReceived = nullptr;
std::map<ButtonId, bool> BLERemoteServer::buttonStates;

// Initialize static callback instances
BLERemoteServer::ServerCallbacks BLERemoteServer::serverCallbacks;
BLERemoteServer::ControlCharCallbacks BLERemoteServer::controlCharCallbacks;

struct CommandPacket {
    RemoteCommand command;
    uint8_t parameterCount;
    uint8_t parameters[16];

    static CommandPacket parse(const uint8_t* data, size_t length) {
        CommandPacket packet;
        if (length < 2) {  // Minimum packet size: command (1) + parameter count (1)
            packet.command = RemoteCommand::NONE;
            packet.parameterCount = 0;
            return packet;
        }

        packet.command = static_cast<RemoteCommand>(data[0]);
        packet.parameterCount = data[1];

        // Validate parameter count
        if (packet.parameterCount > 16 || length < (2 + packet.parameterCount)) {
            packet.command = RemoteCommand::NONE;
            packet.parameterCount = 0;
            return packet;
        }

        // Copy parameters
        memcpy(packet.parameters, data + 2, packet.parameterCount);
        return packet;
    }

    ButtonId getButton() {
        return static_cast<ButtonId>(parameters[0]);
    }
};

void BLERemoteServer::init(const char* deviceName) {
    // Initialize button states
    buttonStates[ButtonId::UP] = false;
    buttonStates[ButtonId::DOWN] = false;
    buttonStates[ButtonId::LEFT] = false;
    buttonStates[ButtonId::RIGHT] = false;
    buttonStates[ButtonId::CONFIRM] = false;
    buttonStates[ButtonId::BACK] = false;

    // Create the BLE Server
    BLEDevice::init(deviceName);

    // Set security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;  // Display only
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(auth_option));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(oob_support));

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    // Create the BLE Service
    BLEService *pService = pServer->createService(REMOTE_SERVICE_UUID);

    // Create BLE Characteristics with larger MTU for parameters
    pControlChar = pService->createCharacteristic(
        CONTROL_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pControlChar->setCallbacks(&controlCharCallbacks);
    pControlChar->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);

    // Create feedback characteristic with notifications
    pFeedbackChar = pService->createCharacteristic(
        FEEDBACK_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Set permissions before adding descriptor
    pFeedbackChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_READ_ENCRYPTED);
    
    // Create and configure the notification descriptor (CCCD)
    BLE2902* p2902 = new BLE2902();
    p2902->setNotifications(true);
    p2902->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    pFeedbackChar->addDescriptor(p2902);

    // Start the service
    pService->start();

    // Configure advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    
    // Set advertising data
    BLEAdvertisementData advData;
    advData.setFlags(0x06);  // General discoverable mode (0x02) | BR/EDR not supported (0x04)
    advData.setName(deviceName);
    advData.setCompleteServices(BLEUUID(REMOTE_SERVICE_UUID));
    
    // Set scan response data with the same name for consistency
    BLEAdvertisementData scanResponse;
    scanResponse.setName(deviceName);
    
    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponse);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    pAdvertising->start();

    LOG_PERIPHERAL("[BLE Server] Remote control server initialized with name: %s", deviceName);
}

void BLERemoteServer::setCommandCallback(CommandCallback callback) {
    onCommandReceived = callback;
}

void BLERemoteServer::sendFeedback(CommandStatus status) {
    if (!deviceConnected || !pFeedbackChar) return;

    uint8_t statusValue = static_cast<uint8_t>(status);
    pFeedbackChar->setValue(&statusValue, 1);
    pFeedbackChar->notify();
}

bool BLERemoteServer::isConnected() {
    return deviceConnected;
}

void BLERemoteServer::stop() {
    if (pServer) {
        pServer->getAdvertising()->stop();
        deviceConnected = false;
    }
}

void BLERemoteServer::ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    LOG_PERIPHERAL("[BLE Server] Client connected");
}

void BLERemoteServer::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    LOG_PERIPHERAL("[BLE Server] Client disconnected");

    // Reset all button states on disconnect
    for (auto& state : buttonStates) {
        state.second = false;
    }

    // Restart advertising after a short delay
    delay(500);  // Give some time for cleanup
    pServer->startAdvertising();
}

void BLERemoteServer::ControlCharCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0 && onCommandReceived) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
        size_t length = value.length();
        
        CommandPacket packet = CommandPacket::parse(data, length);
        if (packet.command != RemoteCommand::NONE) {
            // Handle button state validation for button commands
            if (packet.command == RemoteCommand::BUTTON_DOWN || 
                packet.command == RemoteCommand::BUTTON_UP) {
                
                if (packet.parameterCount < 1) {
                    sendFeedback(CommandStatus::INVALID);
                    return;
                }
                
                ButtonId button = packet.getButton();
                if (!validateButtonTransition(packet.command, button)) {
                    sendFeedback(CommandStatus::BUTTON_STATE_ERROR);
                    return;
                }
            }
            
            // Process the command
            CommandStatus status = onCommandReceived(packet.command, 
                                                  packet.parameters, 
                                                  packet.parameterCount);
            sendFeedback(status);
        } else {
            sendFeedback(CommandStatus::INVALID);
        }
    }
}

bool BLERemoteServer::validateButtonTransition(RemoteCommand cmd, ButtonId button) {
    bool currentState = buttonStates[button];
    
    if (cmd == RemoteCommand::BUTTON_DOWN && currentState) {
        return false;  // Button already down
    }
    
    if (cmd == RemoteCommand::BUTTON_UP && !currentState) {
        return false;  // Button already up
    }
    
    // Update state if validation passed
    buttonStates[button] = (cmd == RemoteCommand::BUTTON_DOWN);
    return true;
}
