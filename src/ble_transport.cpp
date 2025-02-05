#include "ble_transport.h"

// Static member initialization
BLEClient* BLETransport::pClient = nullptr;
BLERemoteService* BLETransport::pRemoteService = nullptr;
BLERemoteCharacteristic* BLETransport::pRemoteCharacteristic = nullptr;
BLEScan* BLETransport::pBLEScan = nullptr;
std::vector<BLEDeviceInfo> BLETransport::discoveredDevices;
bool BLETransport::scanning = false;
Preferences BLETransport::preferences;
String BLETransport::savedDeviceAddress = "";

// BLE callback implementation
class BLETransportCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* client) override {
        Serial.println("Connected to BLE device");
    }

    void onDisconnect(BLEClient* client) override {
        Serial.println("Disconnected from BLE device");
    }
};

void BLETransport::init() {
    BLEDevice::init("");
    
    // Initialize scan
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Initialize preferences
    preferences.begin("ble_transport", false);
    savedDeviceAddress = preferences.getString("device_addr", "");
}

void BLETransport::startScan(int duration) {
    if (scanning) {
        return;
    }
    
    clearDiscoveredDevices();
    scanning = true;
    pBLEScan->start(duration, false);
}

void BLETransport::stopScan() {
    if (!scanning) {
        return;
    }
    
    pBLEScan->stop();
    scanning = false;
}

void BLETransport::update() {
    if (!scanning) {
        return;
    }
    
    // Check if scan has completed
    BLEScanResults results = pBLEScan->start(0, false);
    if (results.getCount() == 0) {
        scanning = false;
    }
}

bool BLETransport::isScanning() {
    return scanning;
}

const std::vector<BLEDeviceInfo>& BLETransport::getDiscoveredDevices() {
    return discoveredDevices;
}

void BLETransport::clearDiscoveredDevices() {
    discoveredDevices.clear();
}

bool BLETransport::connect(const BLEAdvertisedDevice* device) {
    if (!device) {
        return false;
    }
    
    if (pClient) {
        if (pClient->isConnected()) {
            pClient->disconnect();
        }
        pClient = nullptr;
    }
    
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new BLETransportCallback());
    
    // Cast away const since the BLE library doesn't support const
    BLEAdvertisedDevice* nonConstDevice = const_cast<BLEAdvertisedDevice*>(device);
    if (!pClient->connect(nonConstDevice)) {
        Serial.println("Failed to connect");
        return false;
    }
    
    return true;
}

void BLETransport::disconnect() {
    if (pClient != nullptr) {
        if (pClient->isConnected()) {
            pClient->disconnect();
        }
        pClient = nullptr;
    }
    
    pRemoteService = nullptr;
    pRemoteCharacteristic = nullptr;
}

bool BLETransport::isConnected() {
    return pClient != nullptr && pClient->isConnected();
}

bool BLETransport::setService(const char* serviceUUID) {
    if (!isConnected()) {
        return false;
    }
    
    pRemoteService = pClient->getService(serviceUUID);
    return pRemoteService != nullptr;
}

bool BLETransport::setCharacteristic(const char* characteristicUUID) {
    if (!pRemoteService) {
        return false;
    }
    
    pRemoteCharacteristic = pRemoteService->getCharacteristic(characteristicUUID);
    return pRemoteCharacteristic != nullptr;
}

bool BLETransport::writeCharacteristic(const std::string& data) {
    if (!isConnected() || !pRemoteCharacteristic) {
        return false;
    }
    
    pRemoteCharacteristic->writeValue(data);
    return true;
}

bool BLETransport::readCharacteristic(std::string& data) {
    if (!isConnected() || !pRemoteCharacteristic) {
        return false;
    }
    
    data = pRemoteCharacteristic->readValue();
    return true;
}

bool BLETransport::saveDevice(const BLEAdvertisedDevice* device) {
    if (!device) {
        return false;
    }
    
    // Cast away const since the BLE library doesn't support const
    BLEAdvertisedDevice* nonConstDevice = const_cast<BLEAdvertisedDevice*>(device);
    savedDeviceAddress = nonConstDevice->getAddress().toString().c_str();
    preferences.putString("device_addr", savedDeviceAddress);
    return true;
}

void BLETransport::forgetDevice() {
    preferences.remove("device_addr");
    savedDeviceAddress = "";
}

bool BLETransport::hasSavedDevice() {
    return savedDeviceAddress.length() > 0;
}

String BLETransport::getSavedDeviceAddress() {
    return savedDeviceAddress;
}
