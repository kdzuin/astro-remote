#pragma once

#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLESecurity.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>

#include <string>
#include <vector>

#include "transport/camera_store.h"

// Sony BLE definitions
#define SONY_COMPANY_ID 0x012D
#define SONY_CAMERA_TYPE 0x03

// Sony Remote Control Service UUIDs
static BLEUUID SONY_REMOTE_SERVICE_UUID("8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF");
static BLEUUID SONY_REMOTE_CONTROL_CHARACTERISTIC_UUID((uint16_t)0xFF01);  // Write commands
static BLEUUID SONY_REMOTE_STATUS_CHARACTERISTIC_UUID((uint16_t)0xFF02);   // Status notifications
static BLEUUID SONY_REMOTE_STATUS_READ_CHARACTERISTIC_UUID(
    "0000cc05-0000-1000-8000-00805f9b34fb");  // Read status

// Forward declaration
class ClientCallback;

// Security callback
class MySecurity : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        LOG_PERIPHERAL("[BLE] PassKey Request");
        return 000000;  // Return 000000 for Sony cameras
    }

    void onPassKeyNotify(uint32_t pass_key) {
        LOG_PERIPHERAL("[BLE] PassKey Notify number:%d\n", pass_key);
        // Show pairing code on display
        // M5.Display.fillScreen(BLACK);  // BLACK
        // M5.Display.setCursor(0, 0);
        // M5.Display.println("Pairing with camera");
        // M5.Display.printf("Code: %06d\n", pass_key);
        // M5.Display.println("\nPlease confirm on camera");
    }

    bool onConfirmPIN(uint32_t pass_key) {
        LOG_PERIPHERAL("[BLE] Confirm pin:%d\n", pass_key);
        return true;
    }

    bool onSecurityRequest() {
        LOG_PERIPHERAL("[BLE] Security Request");
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
        if (auth_cmpl.success) {
            LOG_PERIPHERAL("[BLE] Authentication Success");
            delay(1000);
        } else {
            LOG_PERIPHERAL("[BLE] Authentication Failure");
            delay(1000);
        }
    }
};

// Device info structure
struct DeviceInfo {
    BLEAdvertisedDevice* device;
    std::string address;
    std::string name;
    int rssi;
    bool isSonyCamera;

    DeviceInfo(BLEAdvertisedDevice* d) : device(d) {
        address = d->getAddress().toString();
        name = d->getName();
        rssi = d->getRSSI();
        isSonyCamera = false;
    }

    std::string getName() const { return name; }
    std::string getAddress() const { return address; }
    int getRSSI() const { return rssi; }
};

class BLEDeviceManager {
public:
    static void init();
    static bool isInitialized();
    static bool isConnected();
    static bool connectToSavedDevice();
    static bool connectToDevice(BLEAdvertisedDevice* device);
    static void disconnect();
    static void scan();
    static bool initConnection();
    static BLERemoteCharacteristic* getControlCharacteristic();
    static BLERemoteCharacteristic* getStatusCharacteristic();
    static BLERemoteService* getService();
    static void onConnect(BLEClient* client);
    static void onDisconnect(BLEClient* client);
    static void onScanComplete();

    // Connection management
    static bool connectToCamera(const BLEAdvertisedDevice* device);
    static void disconnectCamera();

    // Pairing management
    static bool pairCamera(const BLEAdvertisedDevice* device);
    static void unpairCamera();
    static bool isPaired() { return !cachedAddress.empty(); }
    static const std::string& getPairedDeviceAddress() { return cachedAddress; }

    // Saved-camera list (multi-camera). The active camera is the one
    // cachedAddress points at; connecting any camera makes it active.
    static const std::vector<SavedCamera>& getSavedCameras();
    static bool hasSavedCameras();
    static const std::string& getActiveCameraAddress() { return cachedAddress; }
    // Connect to a specific saved camera by address (reuses the reconnect
    // path) and make it the active camera on success.
    static bool connectToAddress(const std::string& address);
    // Forget a specific saved camera. If it is the active camera, disconnect
    // and clear active (no auto-pick).
    static void forgetCamera(const std::string& address);

    // Scanning
    static bool startScan(int duration);
    static void stopScan();
    static void update();
    static void clearDiscoveredDevices();
    static void addDiscoveredDevice(BLEAdvertisedDevice* device);
    static const std::vector<DeviceInfo>& getDiscoveredDevices();
    static bool isScanning();

    // Auto-connect management
    static void setAutoConnect(bool enable) { autoConnectEnabled = enable; }
    static bool isAutoConnectEnabled() { return autoConnectEnabled; }
    static void setManuallyDisconnected(bool value) { manuallyDisconnected = value; }
    static bool wasManuallyDisconnected() { return manuallyDisconnected || !autoConnectEnabled; }

private:
    static BLEClient* pClient;
    static BLEAdvertisedDevice* pDevice;
    static BLERemoteCharacteristic* pRemoteControlChar;
    static BLERemoteCharacteristic* pRemoteStatusChar;
    static BLERemoteService* pRemoteService;
    static bool initialized;
    static bool connected;
    static bool scanning;
    static bool manuallyDisconnected;  // Flag to prevent auto-reconnect after manual disconnect
    static bool autoConnectEnabled;    // Flag to control auto-connect behavior
    static unsigned long scanEndTime;
    static BLEScan* pBLEScan;
    static std::string lastDeviceAddress;
    static std::vector<DeviceInfo> discoveredDevices;
    static Preferences preferences;
    static std::string cachedAddress;
    static CameraStore cameraStore;

    static void saveDeviceAddress(const std::string& address);
    static void loadDeviceAddress();

    // Persist the whole CameraStore (list + active) to NVS under indexed keys.
    static void saveCameraStore();
    // Load the CameraStore from NVS and sync cachedAddress to the active camera.
    static void loadCameraStore();
};
