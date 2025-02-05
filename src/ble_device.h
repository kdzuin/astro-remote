#pragma once

#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <vector>
#include <Arduino.h>

// Sony BLE definitions
#define SONY_COMPANY_ID 0x012D
#define SONY_CAMERA_TYPE 0x0003
#define SONY_REMOTE_SERVICE_UUID "8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF"

struct SonyCameraInfo {
    uint8_t protocolVersion;
    uint16_t modelCode;
    bool isPairingMode;
    
    SonyCameraInfo() : protocolVersion(0), modelCode(0), isPairingMode(false) {}
    
    static bool parseMfgData(const uint8_t* data, size_t length, SonyCameraInfo& info) {
        if (length < 11) return false;
        
        // Check Sony identifier (0x2D, 0x01)
        if (data[0] != 0x2D || data[1] != 0x01) return false;
        
        // Check camera type (0x03, 0x00)
        if (data[2] != 0x03 || data[3] != 0x00) return false;
        
        info.protocolVersion = data[4];
        info.modelCode = (data[6] << 8) | data[7];
        info.isPairingMode = (data[8] == 0x22 && data[9] == 0xEF && data[10] == 0x00);
        
        return true;
    }
};

struct DeviceInfo
{
    BLEAdvertisedDevice* device;
    mutable String name;
    mutable bool nameRequested;
    mutable SonyCameraInfo cameraInfo;

    DeviceInfo(BLEAdvertisedDevice* dev) : device(dev), nameRequested(false)
    {
        name = dev->getAddress().toString().c_str();
        
        // Try to parse Sony camera info
        if (dev->haveManufacturerData()) {
            std::string mfgData = dev->getManufacturerData();
            if (mfgData.length() >= 11) {
                SonyCameraInfo::parseMfgData((const uint8_t*)mfgData.c_str(), mfgData.length(), cameraInfo);
            }
        }
    }

    void updateName() const
    {
        if (!nameRequested && device->haveName())
        {
            name = device->getName().c_str();
            nameRequested = true;
        }
    }
    
    bool isSonyCamera() const {
        return device->haveManufacturerData() && cameraInfo.isPairingMode;
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
    static unsigned long scanEndTime;
    static unsigned long lastScanTime;
};
