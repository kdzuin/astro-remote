#include "ble_device.h"

bool BLEDeviceManager::scanning = false;
unsigned long BLEDeviceManager::scanEndTime = 0;
std::vector<DeviceInfo> BLEDeviceManager::discoveredDevices;
BLEScan* BLEDeviceManager::pBLEScan = nullptr;
BLEClient* BLEDeviceManager::pClient = nullptr;
BLERemoteService* BLEDeviceManager::pRemoteService = nullptr;
BLERemoteCharacteristic* BLEDeviceManager::pRemoteControlChar = nullptr;
BLERemoteCharacteristic* BLEDeviceManager::pRemoteStatusChar = nullptr;
Preferences BLEDeviceManager::preferences;
std::string BLEDeviceManager::cachedAddress;

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

void BLEDeviceManager::saveDeviceAddress(const std::string& address)
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
    for (auto& deviceInfo : discoveredDevices)
    {
        delete deviceInfo.device;
    }
    discoveredDevices.clear();
}

void BLEDeviceManager::addDiscoveredDevice(BLEAdvertisedDevice* device)
{
    // Check if device already exists
    for (const auto& deviceInfo : discoveredDevices)
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

const std::vector<DeviceInfo>& BLEDeviceManager::getDiscoveredDevices()
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
    
    // Start a scan to find the device
    startScan(5);
    delay(5000); // Wait for scan to complete

    // Check if we found our saved device
    bool deviceFound = false;
    BLEAdvertisedDevice* savedDevice = nullptr;
    
    for (const auto& device : discoveredDevices)
    {
        if (device.device->getAddress().toString() == cachedAddress)
        {
            Serial.println("Found saved device in scan results!");
            savedDevice = device.device;
            deviceFound = true;
            break;
        }
    }

    if (!deviceFound)
    {
        Serial.println("Saved device not found in scan results");
        return false;
    }

    // Try to connect
    return connectToCamera(savedDevice);
}

bool BLEDeviceManager::connectToCamera(const BLEAdvertisedDevice* device)
{
    if (!device)
    {
        Serial.println("No device provided for connection");
        return false;
    }

    // Clean up any existing connection
    disconnectCamera();

    BLEAddress address = const_cast<BLEAdvertisedDevice*>(device)->getAddress();
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
    connParams.max_int = 0x20;  // 24ms interval
    connParams.min_int = 0x10;  // 20ms interval
    connParams.timeout = 400;    // 4s timeout
    esp_ble_gap_update_conn_params(&connParams);

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
    
    // Set security after connection
    pClient->setMTU(517);  // Request maximum MTU
    
    // Set encryption using the correct address type
    esp_bd_addr_t bdAddr;
    memcpy(bdAddr, address.getNative(), sizeof(esp_bd_addr_t));
    esp_ble_set_encryption(bdAddr, ESP_BLE_SEC_ENCRYPT);

    // Give more time for pairing and service discovery
    delay(5000);  // Increased delay to allow for user confirmation

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
    if (!pClient->isConnected())
    {
        Serial.println("Client disconnected while initializing connection");
        return false;
    }

    Serial.println("Looking for Sony Remote service...");
    Serial.println("Available services:");
    std::map<std::string, BLERemoteService*>* services = pClient->getServices();
    for(auto const& service : *services) {
        Serial.printf("Service: %s\n", service.first.c_str());
    }

    BLERemoteService* pRemoteService = pClient->getService(SONY_REMOTE_SERVICE_UUID);
    if (pRemoteService == nullptr)
    {
        Serial.println("Failed to find Sony Remote service");
        return false;
    }
    Serial.println("Found Sony Remote service!");

    Serial.println("Looking for characteristics...");
    std::map<std::string, BLERemoteCharacteristic*>* characteristics = pRemoteService->getCharacteristics();
    Serial.println("Available characteristics:");
    for(auto const& characteristic : *characteristics) {
        Serial.printf("Characteristic: %s\n", characteristic.first.c_str());
        // Print properties
        BLERemoteCharacteristic* pChar = characteristic.second;
        Serial.printf("Properties: ");
        if(pChar->canRead()) Serial.printf("READ ");
        if(pChar->canWrite()) Serial.printf("WRITE ");
        if(pChar->canNotify()) Serial.printf("NOTIFY ");
        if(pChar->canIndicate()) Serial.printf("INDICATE ");
        Serial.println();
    }

    Serial.println("Looking for Remote Control characteristic...");
    pRemoteControlChar = pRemoteService->getCharacteristic(SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID);
    if (pRemoteControlChar == nullptr)
    {
        Serial.println("Failed to find Remote Control characteristic");
        return false;
    }
    Serial.println("Found Remote Control characteristic!");

    Serial.println("Looking for Remote Status characteristic...");
    pRemoteStatusChar = pRemoteService->getCharacteristic(SONY_REMOTE_STATUS_CHARACTERISTIC_UUID);
    if (pRemoteStatusChar == nullptr)
    {
        Serial.println("Failed to find Remote Status characteristic");
        return false;
    }
    Serial.println("Found Remote Status characteristic!");

    // Register for notifications if supported
    if(pRemoteStatusChar->canNotify()) {
        Serial.println("Registering for notifications...");
        pRemoteStatusChar->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                                uint8_t* pData, size_t length, bool isNotify) {
            Serial.print("Notification received: ");
            for(int i = 0; i < length; i++) {
                Serial.printf("%02X ", pData[i]);
            }
            Serial.println();
        });
    }

    Serial.println("Connection initialized successfully!");
    return true;
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
