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
    void onResult(BLEAdvertisedDevice advertisedDevice) override
    {
        if (advertisedDevice.haveManufacturerData())
        {
            std::string data = advertisedDevice.getManufacturerData();
            
            // Check if it's a Sony device
            if (data.length() >= 2 && data[0] == 0x2D && data[1] == 0x01)
            {
                SonyCameraInfo info;
                if (SonyCameraInfo::parseMfgData((uint8_t*)data.c_str(), data.length(), info))
                {
                    Serial.printf("Found Sony camera: %s\n", advertisedDevice.getAddress().toString().c_str());
                    Serial.printf("Protocol: v%d, Model: %04X, Pairing: %s\n",
                        info.protocolVersion,
                        info.modelCode,
                        info.isPairingMode ? "yes" : "no");
                    
                    BLEDeviceManager::addDiscoveredDevice(new BLEAdvertisedDevice(advertisedDevice));
                }
            }
        }
    }
};

void BLEDeviceManager::init()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
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
    
    if (scanning) {
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
    if (!scanning) {
        return;
    }
    
    Serial.println("Stopping BLE scan");
    pBLEScan->stop();
    pBLEScan->clearResults();
    scanning = false;
    scanEndTime = 0;
}

void BLEDeviceManager::update()
{
    if (scanning && millis() >= scanEndTime) {
        Serial.println("Scan timeout reached");
        stopScan();
    }
}

void BLEDeviceManager::clearDiscoveredDevices()
{
    for (auto& deviceInfo : discoveredDevices) {
        delete deviceInfo.device;
    }
    discoveredDevices.clear();
}

void BLEDeviceManager::addDiscoveredDevice(BLEAdvertisedDevice* device)
{
    // Check if device already exists
    for (const auto& deviceInfo : discoveredDevices) {
        if (deviceInfo.device->getAddress().equals(device->getAddress())) {
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

bool BLEDeviceManager::initConnection()
{
    if (!pClient || !pClient->isConnected()) {
        Serial.println("Client not connected during service discovery");
        return false;
    }

    // Give BLE stack more time to settle
    delay(1000);

    // Try multiple times to get the service
    int retries = 3;
    while (retries-- > 0) {
        Serial.println("Looking for Sony Remote service...");
        pRemoteService = pClient->getService(BLEUUID(SONY_REMOTE_SERVICE_UUID));
        if (pRemoteService != nullptr) {
            break;
        }
        delay(500);
    }

    if (pRemoteService == nullptr) {
        Serial.println("Failed to find Sony Remote service");
        return false;
    }

    // Wait before getting characteristics
    delay(500);

    // Get Remote Control characteristic
    Serial.println("Looking for Remote Control characteristic...");
    for (retries = 3; retries > 0; retries--) {
        pRemoteControlChar = pRemoteService->getCharacteristic(BLEUUID(SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID));
        if (pRemoteControlChar != nullptr) {
            break;
        }
        delay(500);
    }

    if (pRemoteControlChar == nullptr) {
        Serial.println("Failed to find Remote Control characteristic");
        return false;
    }

    // Wait before getting next characteristic
    delay(500);

    // Get Remote Status characteristic
    Serial.println("Looking for Remote Status characteristic...");
    for (retries = 3; retries > 0; retries--) {
        pRemoteStatusChar = pRemoteService->getCharacteristic(BLEUUID(SONY_REMOTE_STATUS_CHARACTERISTIC_UUID));
        if (pRemoteStatusChar != nullptr) {
            break;
        }
        delay(500);
    }

    if (pRemoteStatusChar == nullptr) {
        Serial.println("Failed to find Remote Status characteristic");
        return false;
    }

    Serial.println("All services and characteristics found!");
    return true;
}

bool BLEDeviceManager::connectToCamera(const BLEAdvertisedDevice* device)
{
    if (!device) {
        Serial.println("No device provided for connection");
        return false;
    }

    // Clean up any existing connection
    disconnectCamera();

    BLEAddress address = const_cast<BLEAdvertisedDevice*>(device)->getAddress();
    Serial.printf("Connecting to camera: %s\n", address.toString().c_str());
    
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallback());
    
    // Try to connect multiple times
    int retries = 3;
    bool connected = false;
    while (retries-- > 0) {
        if (pClient->connect(address)) {
            connected = true;
            break;
        }
        Serial.println("Connection attempt failed, retrying...");
        delay(500);
    }

    if (!connected) {
        Serial.println("Connection failed after all retries");
        delete pClient;
        pClient = nullptr;
        return false;
    }

    Serial.println("Connected to camera!");
    
    // Give more time for services to be discovered
    delay(2000);
    
    if (!initConnection()) {
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
    if (pClient != nullptr) {
        if (pClient->isConnected()) {
            pClient->disconnect();
            delay(500); // Give time for disconnect to complete
        }
        delete pClient;
        pClient = nullptr;
    }
    
    pRemoteService = nullptr;
    pRemoteControlChar = nullptr;
    pRemoteStatusChar = nullptr;
}

bool BLEDeviceManager::pairCamera(const BLEAdvertisedDevice* device)
{
    if (!device) {
        return false;
    }

    // First connect to the camera
    if (!connectToCamera(device)) {
        return false;
    }

    // Save the device address
    BLEAddress address = const_cast<BLEAdvertisedDevice*>(device)->getAddress();
    saveDeviceAddress(address.toString());
    return true;
}

bool BLEDeviceManager::isConnected()
{
    return pClient != nullptr && pClient->isConnected();
}
