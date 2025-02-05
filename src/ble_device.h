#pragma once

#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>

// Sony BLE definitions
#define SONY_COMPANY_ID 0x012D
#define SONY_CAMERA_TYPE 0x0003
#define SONY_REMOTE_SERVICE_UUID "8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF"
#define SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID "8000FF01-FF00-FFFF-FFFF-FFFFFFFFFFFF"
#define SONY_REMOTE_STATUS_CHARACTERISTIC_UUID "8000FF02-FF00-FFFF-FFFF-FFFFFFFFFFFF"

struct SonyCameraInfo {
    uint8_t protocolVersion;
    uint16_t modelCode;
    bool isPairingMode;

    static bool parseMfgData(const uint8_t* data, size_t length, SonyCameraInfo& info) {
        if (length < 11) return false;
        
        // Check Sony identifier (0x2D, 0x01)
        if (data[0] != 0x2D || data[1] != 0x01) return false;
        
        // Check if it's a camera (0x03)
        if (data[2] != 0x03) return false;
        
        info.protocolVersion = data[3];
        info.modelCode = (data[4] << 8) | data[5];
        info.isPairingMode = (data[10] == 0x01);
        
        return true;
    }
};

struct DeviceInfo {
    BLEAdvertisedDevice* device;
    std::string name;
    SonyCameraInfo cameraInfo;

    DeviceInfo(BLEAdvertisedDevice* d) : device(d) {
        updateName();
    }

    void updateName() const {
        if (device && device->haveName()) {
            const_cast<DeviceInfo*>(this)->name = device->getName().c_str();
        } else if (device) {
            const_cast<DeviceInfo*>(this)->name = device->getAddress().toString().c_str();
        }
    }
};

// Client callbacks
class ClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* client) {
        Serial.println("Client connected");
    }

    void onDisconnect(BLEClient* client) {
        Serial.println("Client disconnected");
    }
};

class BLEDeviceManager
{
public:
    static void init();
    static void startScan(int duration);
    static void stopScan();
    static void update();
    static void clearDiscoveredDevices();
    static void addDiscoveredDevice(BLEAdvertisedDevice* device);
    static const std::vector<DeviceInfo>& getDiscoveredDevices();
    static bool isScanning();

    // Connection management
    static bool connectToCamera(const BLEAdvertisedDevice* device);
    static void disconnectCamera();
    static bool isConnected();
    
    // Pairing management
    static bool pairCamera(const BLEAdvertisedDevice* device);
    static void unpairCamera();
    static bool isPaired() { return !cachedAddress.empty(); }
    static const std::string& getPairedDeviceAddress() { return cachedAddress; }

private:
    static bool scanning;
    static unsigned long scanEndTime;
    static std::vector<DeviceInfo> discoveredDevices;
    static BLEScan* pBLEScan;
    static BLEClient* pClient;
    static BLERemoteService* pRemoteService;
    static BLERemoteCharacteristic* pRemoteControlChar;
    static BLERemoteCharacteristic* pRemoteStatusChar;
    static Preferences preferences;
    static std::string cachedAddress;
    
    static bool initConnection();
    static void saveDeviceAddress(const std::string& address);
    static void loadDeviceAddress();
};
