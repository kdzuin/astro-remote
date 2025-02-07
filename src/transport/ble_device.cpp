#include "ble_device.h"
#include "camera_commands.h"

// Client callbacks implementation
class ClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *client)
    {
        Serial.println("Client connected");
        BLEDeviceManager::onConnect(client);
    }

    void onDisconnect(BLEClient *client)
    {
        Serial.println("Client disconnected");
        BLEDeviceManager::onDisconnect(client);
    }
};

// Initialize static members
BLEClient *BLEDeviceManager::pClient = nullptr;
BLEAdvertisedDevice *BLEDeviceManager::pDevice = nullptr;
BLERemoteCharacteristic *BLEDeviceManager::pRemoteControlChar = nullptr;
BLERemoteCharacteristic *BLEDeviceManager::pRemoteStatusChar = nullptr;
BLERemoteService *BLEDeviceManager::pRemoteService = nullptr;
bool BLEDeviceManager::initialized = false;
bool BLEDeviceManager::connected = false;
bool BLEDeviceManager::scanning = false;
bool BLEDeviceManager::manuallyDisconnected = false;
bool BLEDeviceManager::autoConnectEnabled = false;
unsigned long BLEDeviceManager::scanEndTime = 0;
BLEScan *BLEDeviceManager::pBLEScan = nullptr;
std::string BLEDeviceManager::lastDeviceAddress = "";
std::vector<DeviceInfo> BLEDeviceManager::discoveredDevices;
Preferences BLEDeviceManager::preferences;
std::string BLEDeviceManager::cachedAddress = "";

void BLEDeviceManager::onConnect(BLEClient *client)
{
    connected = true;
    Serial.println("BLEDeviceManager: Device connected");

    // Save the device address if it's not already saved
    if (pDevice && !cachedAddress.empty())
    {
        saveDeviceAddress(pDevice->getAddress().toString());
    }
}

void BLEDeviceManager::onDisconnect(BLEClient *client)
{
    connected = false;
    Serial.println("BLEDeviceManager: Device disconnected");

    // Clean up characteristics
    pRemoteControlChar = nullptr;
    pRemoteStatusChar = nullptr;
    pRemoteService = nullptr;

    // Try to reconnect if we have a saved device
    if (!cachedAddress.empty())
    {
        Serial.println("Will try to reconnect...");
        // Schedule reconnection attempt
        delay(1000); // Wait a bit before trying to reconnect
        connectToSavedDevice();
    }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.haveManufacturerData())
        {
            std::string data = advertisedDevice.getManufacturerData();
            if (data.length() >= 3)
            {
                uint16_t manufacturer = (uint8_t)data[0] | ((uint8_t)data[1] << 8);
                uint8_t type = (uint8_t)data[2];

                if (manufacturer == SONY_COMPANY_ID && type == SONY_CAMERA_TYPE)
                {
                    Serial.printf("Found Sony camera: %s\n", advertisedDevice.toString().c_str());
                    BLEDeviceManager::addDiscoveredDevice(new BLEAdvertisedDevice(advertisedDevice));
                }
            }
        }
    }
};

void BLEDeviceManager::init()
{
    // Set device name before initializing BLE
    esp_ble_gap_set_device_name("AstroRemote");

    BLEDevice::init("AstroRemote");

    // Set security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(auth_option));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(oob_support));

    // Set up scan
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    // Set security level
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurity());

    preferences.begin("sony-camera", false);
    loadDeviceAddress();
}

void BLEDeviceManager::loadDeviceAddress()
{
    String addr = preferences.getString("device_address", "");
    cachedAddress = addr.c_str();
}

void BLEDeviceManager::saveDeviceAddress(const std::string &address)
{
    preferences.putString("device_address", address.c_str());
    cachedAddress = address;
}

void BLEDeviceManager::unpairCamera()
{
    disconnectCamera();
    preferences.remove("device_address");
    cachedAddress.clear();
}

void BLEDeviceManager::startScan(int duration)
{
    Serial.println("Starting BLE scan...");
    if (pBLEScan == nullptr)
    {
        Serial.println("BLE Scan not initialized!");
        return;
    }

    if (scanning)
    {
        Serial.println("Already scanning, stopping previous scan");
        stopScan();
    }

    clearDiscoveredDevices();
    pBLEScan->clearResults();
    scanning = true;
    scanEndTime = millis() + (duration * 1000);

    pBLEScan->start(duration, false);
    Serial.println("Scan started");
}

void BLEDeviceManager::stopScan()
{
    if (!scanning)
    {
        return;
    }

    pBLEScan->stop();
    pBLEScan->clearResults();
    scanning = false;
    scanEndTime = 0;
}

void BLEDeviceManager::update()
{
    if (scanning && millis() >= scanEndTime)
    {
        Serial.println("Scan timeout reached");
        stopScan();
    }
}

void BLEDeviceManager::clearDiscoveredDevices()
{
    for (auto &deviceInfo : discoveredDevices)
    {
        delete deviceInfo.device;
    }
    discoveredDevices.clear();
}

void BLEDeviceManager::addDiscoveredDevice(BLEAdvertisedDevice *device)
{
    // Check if device already exists
    for (const auto &deviceInfo : discoveredDevices)
    {
        if (deviceInfo.device->getAddress().equals(device->getAddress()))
        {
            delete device;
            return;
        }
    }

    DeviceInfo info(device);
    discoveredDevices.push_back(info);
}

const std::vector<DeviceInfo> &BLEDeviceManager::getDiscoveredDevices()
{
    return discoveredDevices;
}

bool BLEDeviceManager::isScanning()
{
    return scanning;
}

bool BLEDeviceManager::connectToSavedDevice()
{
    if (cachedAddress.empty())
    {
        Serial.println("No saved device address");
        return false;
    }

    Serial.printf("Attempting to connect to saved device: %s\n", cachedAddress.c_str());

    // Create a client
    if (pClient != nullptr)
    {
        Serial.println("Cleaning up old client");
        delete pClient;
        pClient = nullptr;
    }

    pClient = BLEDevice::createClient();
    if (!pClient)
    {
        Serial.println("Failed to create client");
        return false;
    }

    // Connect directly using the address
    BLEAddress bleAddress(cachedAddress);
    if (!pClient->connect(bleAddress))
    {
        Serial.println("Failed to connect to saved device");
        delete pClient;
        pClient = nullptr;
        return false;
    }

    Serial.println("Connected to saved device!");

    // Initialize the connection (set up service and characteristics)
    if (!initConnection())
    {
        Serial.println("Failed to initialize connection");
        disconnectCamera();
        return false;
    }

    return true;
}

bool BLEDeviceManager::connectToCamera(const BLEAdvertisedDevice *device)
{
    if (!device)
    {
        Serial.println("No device provided for connection");
        return false;
    }

    // Clean up any existing connection
    disconnectCamera();

    BLEAddress address = const_cast<BLEAdvertisedDevice *>(device)->getAddress();
    Serial.printf("Connecting to camera: %s\n", address.toString().c_str());

    pClient = BLEDevice::createClient();
    if (!pClient)
    {
        Serial.println("Failed to create client");
        return false;
    }

    pClient->setClientCallbacks(new ClientCallback());

    // Set connection parameters using GAP API
    esp_ble_conn_update_params_t connParams;
    memset(&connParams, 0, sizeof(connParams));
    memcpy(connParams.bda, address.getNative(), sizeof(esp_bd_addr_t));
    connParams.latency = 0;
    connParams.max_int = 0x40; // 80ms interval
    connParams.min_int = 0x30; // 60ms interval
    connParams.timeout = 500;  // 5s timeout

    // Set the connection parameters before connecting
    esp_ble_gap_update_conn_params(&connParams);

    // Set security level to high
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    // Connect with security
    Serial.println("Attempting connection...");
    if (!pClient->connect(address))
    {
        Serial.println("Connection failed");
        delete pClient;
        pClient = nullptr;
        return false;
    }

    Serial.println("Connected to camera!");

    // Set MTU and wait for completion
    Serial.println("Setting MTU...");
    pClient->setMTU(517); // Request maximum MTU
    delay(1000);          // Wait for MTU exchange

    // Set encryption using the correct address type
    Serial.println("Setting up encryption...");
    esp_bd_addr_t bdAddr;
    memcpy(bdAddr, address.getNative(), sizeof(esp_bd_addr_t));
    esp_ble_set_encryption(bdAddr, ESP_BLE_SEC_ENCRYPT);

    // Give more time for pairing and service discovery
    Serial.println("Waiting for pairing and service discovery...");
    delay(3000); // Reduced from 5000 since we have other delays

    if (!initConnection())
    {
        Serial.println("Failed to initialize connection");
        disconnectCamera();
        return false;
    }

    // Save the device address if connection was successful
    saveDeviceAddress(address.toString());
    return true;
}

void BLEDeviceManager::disconnectCamera()
{
    Serial.println("Disconnecting from camera...");

    if (pClient != nullptr)
    {
        if (pClient->isConnected())
        {
            pClient->disconnect();
            delay(100); // Give it time to disconnect cleanly
        }

        // Clear all pointers
        pRemoteService = nullptr;
        pRemoteControlChar = nullptr;
        pRemoteStatusChar = nullptr;
        pClient = nullptr;
    }

    Serial.println("Disconnected and cleaned up");
}

bool BLEDeviceManager::initConnection()
{
    if (!pClient || !pClient->isConnected())
    {
        Serial.println("Not connected to device");
        return false;
    }

    Serial.println("Looking for Sony Remote service...");
    pRemoteService = pClient->getService(SONY_REMOTE_SERVICE_UUID);
    if (pRemoteService == nullptr)
    {
        Serial.println("Failed to find Sony Remote service");
        return false;
    }

    Serial.println("Looking for Remote Control characteristic...");
    pRemoteControlChar = pRemoteService->getCharacteristic(SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID);
    if (pRemoteControlChar == nullptr)
    {
        Serial.println("Failed to find Remote Control characteristic");
        return false;
    }

    Serial.println("Looking for Remote Status characteristic...");
    pRemoteStatusChar = pRemoteService->getCharacteristic(SONY_REMOTE_STATUS_CHARACTERISTIC_UUID);
    if (pRemoteStatusChar == nullptr)
    {
        Serial.println("Failed to find Status characteristic");
        return false;
    }

    Serial.println("Registering for status notifications...");
    if (pRemoteStatusChar->canNotify())
    {
        pRemoteStatusChar->registerForNotify(CameraCommands::onStatusNotification);
    }

    Serial.println("Reading initial camera status...");
    BLERemoteCharacteristic *pStatusReadChar = pRemoteService->getCharacteristic(SONY_REMOTE_STATUS_READ_CHARACTERISTIC_UUID);
    if (pStatusReadChar && pStatusReadChar->canRead())
    {
        std::string value = pStatusReadChar->readValue();
        if (!value.empty())
        {
            uint8_t *data = (uint8_t *)value.data();
            size_t length = value.length();
            CameraCommands::onStatusNotification(pStatusReadChar, data, length, false);
        }
    }

    Serial.println("Connection initialized successfully");
    return true;
}

BLERemoteService *BLEDeviceManager::getService()
{
    return pRemoteService;
}

BLERemoteCharacteristic *BLEDeviceManager::getControlCharacteristic()
{
    return pRemoteControlChar;
}

BLERemoteCharacteristic *BLEDeviceManager::getStatusCharacteristic()
{
    return pRemoteStatusChar;
}

bool BLEDeviceManager::pairCamera(const BLEAdvertisedDevice *device)
{
    if (!device)
    {
        return false;
    }

    // First connect to the camera
    if (!connectToCamera(device))
    {
        return false;
    }

    // Save the device address
    BLEAddress address = const_cast<BLEAdvertisedDevice *>(device)->getAddress();
    saveDeviceAddress(address.toString());
    return true;
}

bool BLEDeviceManager::isConnected()
{
    return pClient != nullptr && pClient->isConnected();
}
