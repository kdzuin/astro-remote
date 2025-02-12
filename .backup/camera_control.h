#pragma once

#include <Arduino.h>
#include <BLEClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <M5Unified.h>
#include <Preferences.h>

#define SONY_SERVICE_UUID "8000FF00-FF00-FF00-FF00-FF00FF00FF00"
#define SONY_CHARACTERISTIC_UUID "8000FF01-FF00-FF00-FF00-FF00FF00FF00"

// Forward declare the callback class
class ConnectionCallback;

class CameraControl {
    friend class ConnectionCallback;  // Allow ConnectionCallback to access private members
public:
    static void init();
    static void connect();
    static void disconnect();
    static bool isConnected() { return deviceConnected; }
    static void startBulbExposure();
    static void stopBulbExposure();
    static void forgetDevice();
    static const String& getPairedDeviceAddress() { return pairedDeviceAddress; }
    static void setPairedDeviceAddress(const String& addr) { pairedDeviceAddress = addr; }
    static bool isShooting() { return shooting; }
    static int getExposureCount() { return exposureCount; }
    static int getTargetExposures() { return targetExposures; }
    static void setTargetExposures(int count) { targetExposures = count; }
    static unsigned long getCurrentExposureTime() { return currentExposureTime; }
    static void setCurrentExposureTime(unsigned long time) { currentExposureTime = time; }
    static unsigned long getExposureStartTime() { return exposureStartTime; }

private:
    static bool deviceConnected;
    static bool shooting;
    static int exposureCount;
    static int targetExposures;
    static String pairedDeviceAddress;
    static BLEClient* pClient;
    static BLERemoteService* pRemoteService;
    static BLERemoteCharacteristic* pRemoteCharacteristic;
    static Preferences preferences;
    static unsigned long exposureStartTime;
    static unsigned long currentExposureTime;
};
