#pragma once

#include <Arduino.h>
#include <BLEAdvertisedDevice.h>
#include "ble_transport.h"

namespace SonyProtocol {
    // Sony-specific identifiers
    const uint16_t COMPANY_ID = 0x054C;
    const uint8_t CAMERA_TYPE = 0x0C;
    
    // Service and characteristic UUIDs
    const char* SERVICE_UUID = "8000FF00-FF00-FF00-FF00-FF00FF00FF00";
    const char* CHAR_CONTROL_UUID = "8000FF01-FF00-FF00-FF00-FF00FF00FF00";
    const char* CHAR_STATUS_UUID = "8000FF02-FF00-FF00-FF00-FF00FF00FF00";
    
    // Camera commands
    const std::string CMD_SHUTTER = "0100";
    const std::string CMD_START_REC = "0200";
    const std::string CMD_STOP_REC = "0201";
    const std::string CMD_FOCUS_START = "0300";
    const std::string CMD_FOCUS_STOP = "0301";
}

class SonyCamera {
public:
    // Initialization
    static void init();
    
    // Device discovery
    static bool isSonyCamera(const BLEAdvertisedDevice* device);
    static bool isPairingMode(const BLEAdvertisedDevice* device);
    
    // Connection management
    static bool connect(const BLEAdvertisedDevice* device);
    static void disconnect();
    static bool isConnected();
    
    // Camera operations
    static bool takePhoto();
    static bool startRecording();
    static bool stopRecording();
    static bool startFocus();
    static bool stopFocus();
    
    // Device management
    static bool savePairedDevice(const BLEAdvertisedDevice* device);
    static void forgetPairedDevice();
    static bool isPaired();
    static String getPairedDeviceAddress();
};
