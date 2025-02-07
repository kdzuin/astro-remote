#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"

enum class SettingsMenuItem
{
    Disconnect,
    Forget,
    Scan,
    AutoConnect,
    Brightness,
};

class SettingsScreen : public BaseScreen<SettingsMenuItem>
{
public:
    SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings"), brightness(50)
    {
        menuItems.setTitle("Settings Menu");
        updateMenuItems();
    }

    void updateMenuItems()
    {
        menuItems.clear();

        if (BLEDeviceManager::isConnected())
        {
            menuItems.addItem(SettingsMenuItem::Disconnect, "Disconnect");
        }

        if (BLEDeviceManager::isPaired())
        {
            menuItems.addItem(SettingsMenuItem::Forget, "Forget Camera");
        }
        else
        {
            menuItems.addItem(SettingsMenuItem::Scan, "Scan New");
        }

        menuItems.addItem(SettingsMenuItem::AutoConnect,
                          std::string("AutoConnect: ") +
                              (BLEDeviceManager::isAutoConnectEnabled() ? "On" : "Off"));

        menuItems.addItem(SettingsMenuItem::Brightness, "Brightness");
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
            case SettingsMenuItem::Forget:
                BLEDeviceManager::disconnect();
                BLEDeviceManager::setManuallyDisconnected(true);
                // Clear saved device info
                // TODO: Add method to clear saved device info
                updateMenuItems();
                draw();
                break;

            case SettingsMenuItem::Scan:
                BLEDeviceManager::startScan(30); // 30 second scan
                updateMenuItems();
                draw();
                break;

            case SettingsMenuItem::AutoConnect:
            {
                bool newState = !BLEDeviceManager::isAutoConnectEnabled();
                BLEDeviceManager::setAutoConnect(newState);
                // TODO: Save to persistent storage
                updateMenuItems();
                draw();
            }
            break;

            case SettingsMenuItem::Brightness:
            {
                brightness = (brightness + 10) % 110;
                if (brightness < 10)
                    brightness = 10;
                M5.Display.setBrightness(brightness);
                // TODO: Save to persistent storage
            }
            break;

            case SettingsMenuItem::Disconnect:
                BLEDeviceManager::disconnect();
                BLEDeviceManager::setManuallyDisconnected(true);
                updateMenuItems();
                draw();
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
    int brightness;
};
