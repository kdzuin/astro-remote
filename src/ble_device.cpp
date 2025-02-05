#include "ble_device.h"

bool BLEDeviceManager::scanning = false;
std::vector<DeviceInfo> BLEDeviceManager::discoveredDevices;
BLEScan *BLEDeviceManager::pBLEScan = nullptr;
unsigned long BLEDeviceManager::scanStartTime = 0;
unsigned long BLEDeviceManager::scanDuration = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (!advertisedDevice.haveManufacturerData()) {
            return;
        }

        std::string mfgData = advertisedDevice.getManufacturerData();
        if (mfgData.length() < 11) {
            return;
        }

        SonyCameraInfo info;
        if (!SonyCameraInfo::parseMfgData((const uint8_t*)mfgData.c_str(), mfgData.length(), info)) {
            return;
        }

        Serial.printf("Found Sony camera: %s, Protocol: %d, Model: %04X, Pairing: %s\n",
            advertisedDevice.getAddress().toString().c_str(),
            info.protocolVersion,
            info.modelCode,
            info.isPairingMode ? "YES" : "NO");
            
        if (info.isPairingMode) {
            BLEDeviceManager::addDiscoveredDevice(new BLEAdvertisedDevice(advertisedDevice));
        }
    }
};

void BLEDeviceManager::init()
{
    if (pBLEScan != nullptr) {
        return;  // Already initialized
    }

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);  // Get more data including manufacturer data
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // Less time between scans
}

void BLEDeviceManager::startScan(int duration)
{
    if (scanning || pBLEScan == nullptr) {
        return;  // Already scanning or not initialized
    }

    Serial.println("Starting scan for Sony cameras...");
    scanning = true;
    scanStartTime = millis();
    scanDuration = duration * 1000;  // Convert to milliseconds
    discoveredDevices.clear();
    pBLEScan->clearResults();
    pBLEScan->start(duration, false);
}

void BLEDeviceManager::stopScan()
{
    if (!scanning || pBLEScan == nullptr) {
        return;  // Not scanning or not initialized
    }

    Serial.println("Stopping scan...");
    scanning = false;
    pBLEScan->stop();
    pBLEScan->clearResults();
}

void BLEDeviceManager::update()
{
    if (!scanning || pBLEScan == nullptr) {
        return;  // Not scanning or not initialized
    }

    // Check if scan duration has elapsed
    if (millis() - scanStartTime >= scanDuration)
    {
        stopScan();
    }
}

void BLEDeviceManager::clearDiscoveredDevices()
{
    for (auto &deviceInfo : discoveredDevices)
    {
        if (deviceInfo.device != nullptr) {
            delete deviceInfo.device;
        }
    }
    discoveredDevices.clear();
}

void BLEDeviceManager::addDiscoveredDevice(BLEAdvertisedDevice *device)
{
    if (device == nullptr) {
        return;
    }
    discoveredDevices.push_back(DeviceInfo(device));
}

const std::vector<DeviceInfo> &BLEDeviceManager::getDiscoveredDevices()
{
    return discoveredDevices;
}

bool BLEDeviceManager::isScanning()
{
    return scanning;
}
