#include "ble_device.h"

bool BLEDeviceManager::scanning = false;
unsigned long BLEDeviceManager::scanEndTime = 0;
std::vector<DeviceInfo> BLEDeviceManager::discoveredDevices;
BLEScan *BLEDeviceManager::pBLEScan = nullptr;
BLEClient *BLEDeviceManager::pClient = nullptr;
BLERemoteService *BLEDeviceManager::pRemoteService = nullptr;
BLERemoteCharacteristic *BLEDeviceManager::pRemoteControlChar = nullptr;
BLERemoteCharacteristic *BLEDeviceManager::pRemoteStatusChar = nullptr;
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
                if (SonyCameraInfo::parseMfgData((uint8_t *)data.c_str(), data.length(), info))
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

    Serial.println("Stopping BLE scan");
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

bool BLEDeviceManager::initConnection()
{
    if (!pClient || !pClient->isConnected())
    {
        Serial.println("Client not connected during service discovery");
        return false;
    }

    // Give BLE stack more time to settle
    delay(1000);

    // Get remote service
    Serial.println("Looking for Sony Remote service...");
    BLEUUID serviceUUID(SONY_REMOTE_SERVICE_UUID);
    std::map<std::string, BLERemoteService *> *services = pClient->getServices();

    Serial.println("Available services:");
    for (auto &it : *services)
    {
        Serial.printf("Service: %s\n", it.first.c_str());
    }

    pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.println("Failed to find Sony Remote service");
        return false;
    }

    // Get characteristics
    Serial.println("Looking for characteristics...");
    std::map<std::string, BLERemoteCharacteristic *> *characteristics = pRemoteService->getCharacteristics();

    Serial.println("Available characteristics:");
    for (auto &it : *characteristics)
    {
        Serial.printf("Characteristic: %s\n", it.first.c_str());
    }

    // Get Remote Control characteristic
    Serial.println("Looking for Remote Control characteristic...");
    BLEUUID controlUUID(SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID);
    pRemoteControlChar = pRemoteService->getCharacteristic(controlUUID);
    if (pRemoteControlChar == nullptr)
    {
        Serial.println("Failed to find Remote Control characteristic");
        return false;
    }

    // Get Remote Status characteristic
    Serial.println("Looking for Remote Status characteristic...");
    BLEUUID statusUUID(SONY_REMOTE_STATUS_CHARACTERISTIC_UUID);
    pRemoteStatusChar = pRemoteService->getCharacteristic(statusUUID);
    if (pRemoteStatusChar == nullptr)
    {
        Serial.println("Failed to find Remote Status characteristic");
        return false;
    }

    // Register for notifications
    if (pRemoteStatusChar->canNotify())
    {
        pRemoteStatusChar->registerForNotify([](BLERemoteCharacteristic *pChar, uint8_t *data, size_t length, bool isNotify)
                                             {
            Serial.print("Status notification: ");
            for(int i = 0; i < length; i++) {
                Serial.printf("%02X ", data[i]);
            }
            Serial.println(); });
    }

    Serial.println("All services and characteristics found!");
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

    // Connect with security
    if (!pClient->connect(address, BLE_ADDR_TYPE_RANDOM))
    {
        Serial.println("Connection failed");
        delete pClient;
        pClient = nullptr;
        return false;
    }

    Serial.println("Connected to camera!");

    // Give more time for services to be discovered
    delay(2000);

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
    if (pClient != nullptr)
    {
        if (pClient->isConnected())
        {
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
