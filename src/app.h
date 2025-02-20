#pragma once

#include <M5Unified.h>

#include "components/menu_system.h"
#include "transport/ble_device.h"
#include "transport/ble_remote_server.h"
#include "utils/colors.h"
#include "utils/preferences.h"

class Application {
public:
    Application() = default;

    static void emergencyStop() {
        // Implementation
        LOG_DEBUG("Emergency stop");
    }

    void setup() {
        LOG_APP("Starting Sony Camera Remote");

        // Initialize preferences first
        PreferencesManager::init();

        // Setup display
        M5.Display.setRotation(0);

        // Show splash screen
        M5.Display.fillScreen(colors::get(colors::BLACK));
        M5.Display.setTextColor(colors::get(colors::WHITE));
        M5.Display.setTextSize(1.75);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString("Astro Remote", M5.Display.width() / 2, M5.Display.height() / 2);

        delay(1000);

        M5.Display.setTextSize(1.25);
        M5.Display.setBrightness(PreferencesManager::getBrightness());

        // Initialize BLE components
        BLEDeviceManager::setAutoConnect(PreferencesManager::getAutoConnect());
        BLEDeviceManager::init();

        // Initialize BLE Remote Server
        BLERemoteServer::init("M5Remote");
        RemoteControlManager::init();

        MenuSystem::init();
    }

    void loop() {
        // Check for emergency stop FIRST
        if (RemoteControlManager::wasEmergencyPressed()) {
            LOG_APP("[!] EMERGENCY STOP TRIGGERED!");
            emergencyStop();
            return;
        }

        // Update BLE and menu
        BLEDeviceManager::update();      // Update BLE state
        RemoteControlManager::update();  // Update remote control state
        MenuSystem::update();            // This will handle input internally
    }
};
