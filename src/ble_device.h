#pragma once

#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <vector>
#include <Arduino.h>

struct DeviceInfo
{
    BLEAdvertisedDevice* device;
    mutable String name;
    mutable bool nameRequested;

    DeviceInfo(BLEAdvertisedDevice* dev) : device(dev), nameRequested(false)
    {
        name = dev->getAddress().toString().c_str();
    }

    void updateName() const
    {
        if (!nameRequested && device->haveName())
        {
            name = device->getName().c_str();
            nameRequested = true;
        }
    }
};

class BLEDeviceManager {
public:
    static void init();
    static void startScan(int duration);
    static void stopScan();
    static void update();
    static void clearDiscoveredDevices();
    static void addDiscoveredDevice(BLEAdvertisedDevice* device);
    static const std::vector<DeviceInfo>& getDiscoveredDevices();
    static bool isScanning();

private:
    static bool scanning;
    static std::vector<DeviceInfo> discoveredDevices;
    static BLEScan* pBLEScan;
    static unsigned long scanStartTime;
    static unsigned long scanDuration;
};
