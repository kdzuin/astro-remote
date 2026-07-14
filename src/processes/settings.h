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

    // Battery tiers (percent). Below WARNING → amber; below CRITICAL → red +
    // the Astro run-screen alert flash; at/below EMERGENCY → auto-pause and the
    // full-screen emergency takeover. One source for every battery threshold.
    static constexpr int BATTERY_WARNING_PCT = 50;
    static constexpr int BATTERY_CRITICAL_PCT = 20;
    static constexpr int BATTERY_EMERGENCY_PCT = 10;

    // A negative level means the reading is unavailable — not critical/emergency.
    static bool isBatteryCritical(int batteryLevel) {
        return batteryLevel >= 0 && batteryLevel < BATTERY_CRITICAL_PCT;
    }

    // True only when running on battery. Charging or unknown-charge-state both
    // count as "not on battery" — never trip the emergency lockout on cable.
    static bool isCharging() {
        return M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
    }

    // Emergency: on battery (not charging) and at/below the emergency threshold.
    static bool isBatteryEmergency(int batteryLevel) {
        return batteryLevel >= 0 && batteryLevel <= BATTERY_EMERGENCY_PCT && !isCharging();
    }

    static uint32_t getBatteryStatusColor(int batteryLevel) {
        if (batteryLevel < BATTERY_CRITICAL_PCT) {
            return colors::get(colors::ERROR);  // Critical
        }
        if (batteryLevel < BATTERY_WARNING_PCT) {
            return colors::get(colors::WARNING);  // Warning
        }
        return colors::get(colors::SUCCESS);  // Good
    }
};
