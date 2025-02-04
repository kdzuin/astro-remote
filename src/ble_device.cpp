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
        // Serial.print("Device found: ");
        // Serial.println(advertisedDevice.getAddress().toString().c_str());
        BLEDeviceManager::addDiscoveredDevice(new BLEAdvertisedDevice(advertisedDevice));
    }
};

void BLEDeviceManager::init()
{
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false);
}

void BLEDeviceManager::startScan(int duration)
{
    Serial.println("Starting scan...");
    scanning = true;
    scanStartTime = millis();
    scanDuration = duration * 1000; // Convert to milliseconds
    discoveredDevices.clear();
    pBLEScan->clearResults();
    pBLEScan->start(duration, false);
}

void BLEDeviceManager::stopScan()
{
    Serial.println("Stopping scan...");
    scanning = false;
    pBLEScan->stop();
    pBLEScan->clearResults();
}

void BLEDeviceManager::update()
{
    if (scanning && (millis() - scanStartTime >= scanDuration))
    {
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
