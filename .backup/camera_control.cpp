#include "camera_control.h"

bool CameraControl::deviceConnected = false;
bool CameraControl::shooting = false;
int CameraControl::exposureCount = 0;
int CameraControl::targetExposures = -1;
String CameraControl::pairedDeviceAddress = "";
BLEClient* CameraControl::pClient = nullptr;
BLERemoteService* CameraControl::pRemoteService = nullptr;
BLERemoteCharacteristic* CameraControl::pRemoteCharacteristic = nullptr;
Preferences CameraControl::preferences;
unsigned long CameraControl::exposureStartTime = 0;
unsigned long CameraControl::currentExposureTime = 30000;

class ConnectionCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) override {
        CameraControl::deviceConnected = true;
        Serial.println("Connected!");
    }

    void onDisconnect(BLEClient* pclient) override {
        CameraControl::deviceConnected = false;
        Serial.println("Disconnected!");
    }
};

void CameraControl::init() {
    preferences.begin("camera", true);
    pairedDeviceAddress = preferences.getString("addr", "");
    preferences.end();
}

void CameraControl::connect() {
    if (pairedDeviceAddress.isEmpty()) {
        Serial.println("No paired device address");
        return;
    }

    if (pClient == nullptr) {
        pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(new ConnectionCallback());
    }

    Serial.printf("Connecting to %s\n", pairedDeviceAddress.c_str());
    pClient->connect(BLEAddress(pairedDeviceAddress.c_str()));

    pRemoteService = pClient->getService(BLEUUID(SONY_SERVICE_UUID));
    if (pRemoteService != nullptr) {
        pRemoteCharacteristic =
            pRemoteService->getCharacteristic(BLEUUID(SONY_CHARACTERISTIC_UUID));
    }
}

void CameraControl::disconnect() {
    if (pClient) {
        pClient->disconnect();
        delete pClient;
        pClient = nullptr;
    }
    deviceConnected = false;
}

void CameraControl::startBulbExposure() {
    if (!deviceConnected || pRemoteCharacteristic == nullptr)
        return;

    Serial.println("Starting exposure");
    uint8_t cmd[] = {0x01};  // Placeholder command
    pRemoteCharacteristic->writeValue(cmd, sizeof(cmd));
    shooting = true;
    exposureStartTime = millis();
}

void CameraControl::stopBulbExposure() {
    if (!deviceConnected || pRemoteCharacteristic == nullptr)
        return;

    Serial.println("Stopping exposure");
    uint8_t cmd[] = {0x00};  // Placeholder command
    pRemoteCharacteristic->writeValue(cmd, sizeof(cmd));
    shooting = false;
    exposureCount++;
}

void CameraControl::forgetDevice() {
    disconnect();
    pairedDeviceAddress = "";
    preferences.begin("camera", false);
    preferences.clear();
    preferences.end();
}
