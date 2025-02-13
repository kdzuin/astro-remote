#pragma once

#include "components/menu_system.h"
#include "hardware_interface.h"
#include "transport/ble_device.h"
#include "utils/display_constants.h"
#include "utils/preferences.h"

class SettingsProcess {
public:
    enum class Status { Normal, Success, Error };

    struct ConnectionState {
        bool isConnected;
        bool isPaired;
        bool isAutoConnectEnabled;
    };

    struct DeviceState {
        int batteryLevel;
        int brightness;
    };

    static ConnectionState getConnectionState() {
        return {BLEDeviceManager::isConnected(), BLEDeviceManager::isPaired(),
                BLEDeviceManager::isAutoConnectEnabled()};
    }

    static DeviceState getDeviceState() {
        auto& power = MenuSystem::getHardware()->getPower();
        return {power.getBatteryLevel(), PreferencesManager::getBrightness()};
    }

    static bool connectToDevice() {
        if (BLEDeviceManager::connectToSavedDevice()) {
            return true;
        }
        return false;
    }

    static void disconnectDevice() {
        BLEDeviceManager::disconnect();
        BLEDeviceManager::setManuallyDisconnected(true);
    }

    static void forgetDevice() {
        BLEDeviceManager::unpairCamera();
        BLEDeviceManager::setManuallyDisconnected(true);
    }

    static void toggleAutoConnect() {
        bool newState = !BLEDeviceManager::isAutoConnectEnabled();
        BLEDeviceManager::setAutoConnect(newState);
        PreferencesManager::setAutoConnect(newState);
    }

    static void cycleBrightness() {
        auto& display = MenuSystem::getHardware()->getDisplay();
        auto nextLevel =
            PreferencesManager::getNextBrightnessLevel(PreferencesManager::getBrightness());
        display.setBrightness(static_cast<uint8_t>(nextLevel));
        PreferencesManager::setBrightness(static_cast<uint8_t>(nextLevel));
    }

    static uint16_t getBatteryStatusColor(int batteryLevel) {
        auto& display = MenuSystem::getHardware()->getDisplay();
        if (batteryLevel < 20) {
            return display.getColor(display::colors::ERROR);  // Critical
        }
        if (batteryLevel < 50) {
            return display.getColor(display::colors::WARNING);  // Warning
        }
        return display.getColor(display::colors::SUCCESS);  // Good
    }
};
