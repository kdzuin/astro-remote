#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <vector>
#include <Preferences.h>

// Forward declarations
class BLETransportCallback;
class BLEDeviceInfo;

class BLETransport {
public:
    // Initialization
    static void init();
    
    // Device discovery
    static void startScan(int duration);
    static void stopScan();
    static void update();
    static bool isScanning();
    static const std::vector<BLEDeviceInfo>& getDiscoveredDevices();
    static void clearDiscoveredDevices();
    
    // Connection management
    static bool connect(const BLEAdvertisedDevice* device);
    static void disconnect();
    static bool isConnected();
    
    // Service and characteristic management
    static bool setService(const char* serviceUUID);
    static bool setCharacteristic(const char* characteristicUUID);
    static bool writeCharacteristic(const std::string& data);
    static bool readCharacteristic(std::string& data);
    
    // Device info management
    static bool saveDevice(const BLEAdvertisedDevice* device);
    static void forgetDevice();
    static bool hasSavedDevice();
    static String getSavedDeviceAddress();

private:
    friend class BLETransportCallback;
    
    static BLEClient* pClient;
    static BLERemoteService* pRemoteService;
    static BLERemoteCharacteristic* pRemoteCharacteristic;
    static BLEScan* pBLEScan;
    static std::vector<BLEDeviceInfo> discoveredDevices;
    static bool scanning;
    static Preferences preferences;
    static String savedDeviceAddress;
};

class BLEDeviceInfo {
public:
    BLEDeviceInfo(BLEAdvertisedDevice* dev) : device(dev) {}
    ~BLEDeviceInfo() { delete device; }
    
    String getName() const { 
        return device ? device->getName().c_str() : "Unknown"; 
    }
    
    String getAddress() const { 
        return device ? device->getAddress().toString().c_str() : ""; 
    }
    
    int getRSSI() const { 
        return device ? device->getRSSI() : 0; 
    }
    
    const BLEAdvertisedDevice* getDevice() const { return device; }

private:
    BLEAdvertisedDevice* device;
};
