#include "sony_camera.h"

void SonyCamera::init() {
    BLETransport::init();
}

bool SonyCamera::isSonyCamera(const BLEAdvertisedDevice* device) {
    if (!device) {
        return false;
    }
    
    // Cast away const for BLE library compatibility
    BLEAdvertisedDevice* nonConstDevice = const_cast<BLEAdvertisedDevice*>(device);
    
    if (!nonConstDevice->haveManufacturerData()) {
        return false;
    }
    
    std::string mfgData = nonConstDevice->getManufacturerData();
    if (mfgData.length() < 3) {
        return false;
    }
    
    uint16_t manufacturer = (uint8_t)mfgData[0] | ((uint8_t)mfgData[1] << 8);
    uint8_t type = (uint8_t)mfgData[2];
    
    return manufacturer == SonyProtocol::COMPANY_ID && type == SonyProtocol::CAMERA_TYPE;
}

bool SonyCamera::isPairingMode(const BLEAdvertisedDevice* device) {
    if (!device) {
        return false;
    }
    
    // Cast away const for BLE library compatibility
    BLEAdvertisedDevice* nonConstDevice = const_cast<BLEAdvertisedDevice*>(device);
    
    if (!nonConstDevice->haveManufacturerData()) {
        return false;
    }
    
    std::string mfgData = nonConstDevice->getManufacturerData();
    if (mfgData.length() < 8) {
        return false;
    }
    
    // Check pairing mode bytes (specific to Sony cameras)
    return (uint8_t)mfgData[7] == 0x22;
}

bool SonyCamera::connect(const BLEAdvertisedDevice* device) {
    if (!device || !isSonyCamera(device)) {
        return false;
    }
    
    if (!BLETransport::connect(device)) {
        return false;
    }
    
    if (!BLETransport::setService(SonyProtocol::SERVICE_UUID)) {
        BLETransport::disconnect();
        return false;
    }
    
    if (!BLETransport::setCharacteristic(SonyProtocol::CHAR_CONTROL_UUID)) {
        BLETransport::disconnect();
        return false;
    }
    
    return true;
}

void SonyCamera::disconnect() {
    BLETransport::disconnect();
}

bool SonyCamera::isConnected() {
    return BLETransport::isConnected();
}

bool SonyCamera::takePhoto() {
    return BLETransport::writeCharacteristic(SonyProtocol::CMD_SHUTTER);
}

bool SonyCamera::startRecording() {
    return BLETransport::writeCharacteristic(SonyProtocol::CMD_START_REC);
}

bool SonyCamera::stopRecording() {
    return BLETransport::writeCharacteristic(SonyProtocol::CMD_STOP_REC);
}

bool SonyCamera::startFocus() {
    return BLETransport::writeCharacteristic(SonyProtocol::CMD_FOCUS_START);
}

bool SonyCamera::stopFocus() {
    return BLETransport::writeCharacteristic(SonyProtocol::CMD_FOCUS_STOP);
}

bool SonyCamera::savePairedDevice(const BLEAdvertisedDevice* device) {
    if (!device || !isSonyCamera(device)) {
        return false;
    }
    
    return BLETransport::saveDevice(device);
}

void SonyCamera::forgetPairedDevice() {
    BLETransport::forgetDevice();
}

bool SonyCamera::isPaired() {
    return BLETransport::hasSavedDevice();
}

String SonyCamera::getPairedDeviceAddress() {
    return BLETransport::getSavedDeviceAddress();
}
