#pragma once

#include "transport/ble_device.h"
#include "utils/colors.h"
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
        return {M5.Power.getBatteryLevel(), PreferencesManager::getBrightness()};
    }

    static bool connectToDevice() {
        if (BLEDeviceManager::connectToSavedDevice()) {
            return true;
        }
        return false;
    }

    static void disconnectDevice() { BLEDeviceManager::disconnect(); }

    static void forgetDevice() { BLEDeviceManager::unpairCamera(); }

    static void toggleAutoConnect() {
        bool newState = !BLEDeviceManager::isAutoConnectEnabled();
        BLEDeviceManager::setAutoConnect(newState);
        PreferencesManager::setAutoConnect(newState);
    }

    static void cycleBrightness() {
        auto nextLevel =
            PreferencesManager::getNextBrightnessLevel(PreferencesManager::getBrightness());
        M5.Display.setBrightness(static_cast<uint8_t>(nextLevel));
        PreferencesManager::setBrightness(static_cast<uint8_t>(nextLevel));
    }

    // Battery level below this (percent) is "critical" — red status colour and,
    // on the Astro run screen, the full-screen alert flash.
    static constexpr int BATTERY_CRITICAL_PCT = 20;

    // A negative level means the reading is unavailable — not critical.
    static bool isBatteryCritical(int batteryLevel) {
        return batteryLevel >= 0 && batteryLevel < BATTERY_CRITICAL_PCT;
    }

    static uint32_t getBatteryStatusColor(int batteryLevel) {
        if (batteryLevel < BATTERY_CRITICAL_PCT) {
            return colors::get(colors::ERROR);  // Critical
        }
        if (batteryLevel < 50) {
            return colors::get(colors::WARNING);  // Warning
        }
        return colors::get(colors::SUCCESS);  // Good
    }
};
