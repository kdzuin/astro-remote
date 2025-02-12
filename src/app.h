#pragma once

#include "components/menu_system.h"
#include "hardware_interface.h"
#include "transport/ble_device.h"
#include "transport/ble_remote_server.h"
#include "utils/display_constants.h"
#include "utils/preferences.h"

class Application {
public:
    explicit Application(IHardware& hw) : hardware(hw) {}

    void setup() {
        LOG_APP("Starting Sony Camera Remote");

        // Initialize preferences first
        PreferencesManager::init();

        // Setup display
        auto& display = hardware.getDisplay();
        display.setRotation(0);

        // Show splash screen
        display.fillScreen(display.getColor(display::colors::BLACK));
        display.setTextColor(display.getColor(display::colors::WHITE));
        display.setTextSize(1.75);
        display.setTextAlignment(textAlign::middle_center);
        display.drawString("Astro Remote", display.width() / 2, display.height() / 2);

        delay(1000);

        display.setTextSize(1.25);
        display.setBrightness(PreferencesManager::getBrightness());

        // Initialize BLE components
        BLEDeviceManager::setAutoConnect(PreferencesManager::getAutoConnect());
        BLEDeviceManager::init();

        // Initialize BLE Remote Server
        BLERemoteServer::init("M5Remote");
        RemoteControlManager::init();

        MenuSystem::init(&hardware);
    }

    void loop() {
        // Update hardware state
        hardware.update();

        // Update BLE and menu
        BLEDeviceManager::update();      // Update BLE state
        RemoteControlManager::update();  // Update remote control state
        MenuSystem::update();            // This will handle input internally
    }

private:
    IHardware& hardware;
};
