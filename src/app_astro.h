#pragma once

#include <M5Unified.h>

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/astro_screen.h"
#include "transport/ble_device.h"
#include "transport/ble_remote_server.h"
#include "utils/colors.h"
#include "utils/preferences.h"

class Application {
public:
    Application() = default;

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
        BLEDeviceManager::init();

        // Initialize BLE Remote Server
        BLERemoteServer::init("M5Remote");
        RemoteControlManager::init();

        // Register astro observers (BLE status push) once, up front.
        AstroProcess::instance().init();

        // Initialize menu system
        MenuSystem::init();
        MenuSystem::setScreen(new AstroScreen());
    }

    void loop() {
        BLEDeviceManager::update();      // Update BLE state
        RemoteControlManager::update();  // Update remote control state

        // Feed live camera-connection state, then tick the astro sequence
        // state machine so a running sequence actually advances.
        AstroProcess::instance().setCameraConnected(BLEDeviceManager::isConnected());
        AstroProcess::instance().update();

        MenuSystem::update();  // This will handle input internally
    }
};
