#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"
#include "../menu_system.h"
#include "control_screen.h"
#include "settings_screen.h"

// Forward declare screens we'll create
class ControlScreen;
class SettingsScreen;

enum class MainMenuItem
{
    Connect,
    Disconnect,
    Control,
    Settings
};

class MainScreen : public BaseScreen<MainMenuItem>
{
public:
    MainScreen() : BaseScreen<MainMenuItem>("Main")
    {
        menuItems.setTitle("Main Menu");
        updateMenuItems();
    }

    void updateMenuItems()
    {
        menuItems.clear();
        if (BLEDeviceManager::isConnected())
        {
            menuItems.addItem(MainMenuItem::Disconnect, "Disconnect");
        }
        else
        {
            menuItems.addItem(MainMenuItem::Connect, "Connect");
        }
        menuItems.addItem(MainMenuItem::Control, "Control");
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }

    void drawContent() override
    {
        menuItems.draw();
    }

    void update() override
    {
        // Check connection status periodically
        unsigned long now = millis();
        if (now - lastConnectionCheck > 1000)
        {
            bool isConnected = BLEDeviceManager::isConnected();

            if (wasConnected != isConnected)
            {
                updateMenuItems();
                draw();
            }

            lastConnectionCheck = now;
            wasConnected = isConnected;
        }

        if (M5.BtnA.wasClicked() && !M5.BtnB.wasPressed())
        {
            switch (menuItems.getSelectedId())
            {
            case MainMenuItem::Connect:
                if (BLEDeviceManager::isPaired())
                {
                    BLEDeviceManager::connectToSavedDevice();
                }
                updateMenuItems();
                draw();
                break;
            case MainMenuItem::Disconnect:
                BLEDeviceManager::disconnect();
                BLEDeviceManager::setManuallyDisconnected(true);
                updateMenuItems();
                draw();
                break;

            case MainMenuItem::Control:
                MenuSystem::setScreen(new ControlScreen());
                break;

            case MainMenuItem::Settings:
                MenuSystem::setScreen(new SettingsScreen());
                break;
            }
        }

        if (M5.BtnB.wasClicked())
        {
            menuItems.selectNext();
            draw();
        }
    }

private:
    unsigned long lastConnectionCheck = 0;
    bool wasConnected = false;
};
