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
    Astro,
    Video,
    Photo,
    Settings
};

class MainScreen : public BaseScreen<MainMenuItem>
{
public:
    MainScreen() : BaseScreen<MainMenuItem>("Main")
    {
        menuItems.setTitle("Main Menu");
        updateMenuItems();

        if (BLEDeviceManager::isConnected())
        {
            setStatusText("Connected");
            setStatusBgColor(M5.Display.color888(0, 100, 0));
        }
        else
        {
            setStatusText("Select Option");
            setStatusBgColor(M5.Display.color888(0, 0, 100));
        }

        // Try to auto-connect on startup if enabled
        if (BLEDeviceManager::isAutoConnectEnabled() && BLEDeviceManager::isPaired() && !BLEDeviceManager::wasManuallyDisconnected())
        {
            setStatusText("Auto-connecting...");
            setStatusBgColor(M5.Display.color888(128, 128, 0));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice())
            {
                setStatusText("Connected");
                setStatusBgColor(M5.Display.color888(0, 200, 0));
                updateMenuItems();
                draw();
            }
        }
    }

    void updateMenuItems()
    {
        const bool isConnected = BLEDeviceManager::isConnected();

        menuItems.clear();
        if (!isConnected)
        {
            menuItems.addItem(MainMenuItem::Connect, "Connect");
        }
        menuItems.addItem(MainMenuItem::Astro, "Astro Remote", isConnected);
        menuItems.addItem(MainMenuItem::Video, "Video Remote", isConnected);
        menuItems.addItem(MainMenuItem::Photo, "Photo Remote", isConnected);

        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }

    void drawContent() override
    {
        menuItems.draw();
    }

    void update() override
    {

        if (M5.BtnA.wasClicked())
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
};
