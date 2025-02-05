#pragma once

#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <Preferences.h>
#include <vector>

// Sony BLE definitions
#define SONY_COMPANY_ID 0x012D
#define SONY_CAMERA_TYPE 0x03
#define SONY_REMOTE_SERVICE_UUID "8000ff00-ff00-ffff-ffff-ffffffffffff"
#define SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID "0000ff01-0000-1000-8000-00805f9b34fb"
#define SONY_REMOTE_STATUS_CHARACTERISTIC_UUID "0000ff02-0000-1000-8000-00805f9b34fb"

// Security callback
class MySecurity : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        Serial.println("PassKey Request");
        return 123456;
    }

    void onPassKeyNotify(uint32_t pass_key) {
        Serial.printf("PassKey Notify number:%d\n", pass_key);
        // Show pairing code on display
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Pairing with camera");
        M5.Display.printf("Code: %06d\n", pass_key);
        M5.Display.println("\nPlease confirm on camera");
    }

    bool onConfirmPIN(uint32_t pass_key) {
        Serial.printf("Confirm pin:%d\n", pass_key);
        return true;
    }

    bool onSecurityRequest() {
        Serial.println("Security Request");
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
        if(auth_cmpl.success) {
            Serial.println("Authentication Success");
            M5.Display.println("\nPairing successful!");
            delay(1000);
        } else {
            Serial.println("Authentication Failure");
            M5.Display.println("\nPairing failed!");
            delay(1000);
        }
    }
};

struct DeviceInfo {
    BLEAdvertisedDevice* device;
    
    DeviceInfo(BLEAdvertisedDevice* d) : device(d) {
        // Constructor just stores the device pointer
    }
    
    std::string getName() const {
        return device ? device->getName() : "Unknown";
    }
    
    std::string getAddress() const {
        return device ? device->getAddress().toString() : "";
    }
    
    int getRSSI() const {
        return device ? device->getRSSI() : 0;
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
    static bool connectToSavedDevice();
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
