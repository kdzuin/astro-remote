#include "settings_screen.h"
#include "scan_screen.h"
#include "../utils/preferences.h"

SettingsScreen::SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings")
{
    setStatusText("Select Option");
    setStatusBgColor(M5.Display.color888(0, 0, 100));
    menuItems.setTitle("Settings Menu");
    updateMenuItems();

    setStatusText("Select Option");
    setStatusBgColor(M5.Display.color888(0, 0, 100));
}

void SettingsScreen::updateMenuItems()
{
    menuItems.clear();

    if (BLEDeviceManager::isConnected())
    {
        menuItems.addItem(SettingsMenuItem::Disconnect, "Disconnect");
    }
    else if (BLEDeviceManager::isPaired())
    {
        menuItems.addItem(SettingsMenuItem::Connect, "Connect");
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

    menuItems.addItem(SettingsMenuItem::Brightness, std::string("Brightness: ") + std::to_string(PreferencesManager::getBrightness()));
}

void SettingsScreen::drawContent()
{
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void SettingsScreen::update()
{
    if (M5.BtnA.wasClicked())
    {
        switch (menuItems.getSelectedId())
        {
        case SettingsMenuItem::Connect:
            if (BLEDeviceManager::connectToSavedDevice())
            {
                setStatusText("Connected!");
                setStatusBgColor(M5.Display.color888(0, 200, 0));
            }
            else
            {
                setStatusText("Connection failed!");
                setStatusBgColor(M5.Display.color888(200, 0, 0));
            }
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Forget:
            BLEDeviceManager::unpairCamera();
            BLEDeviceManager::setManuallyDisconnected(true);
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Scan:
            MenuSystem::setScreen(new ScanScreen());
            break;

        case SettingsMenuItem::AutoConnect:
        {
            bool newState = !BLEDeviceManager::isAutoConnectEnabled();
            BLEDeviceManager::setAutoConnect(newState);
            PreferencesManager::setAutoConnect(newState);
            updateMenuItems();
            draw();
        }
        break;

        case SettingsMenuItem::Brightness:
        {
            auto nextLevel = PreferencesManager::getNextBrightnessLevel(PreferencesManager::getBrightness());
            M5.Display.setBrightness(static_cast<uint8_t>(nextLevel));
            PreferencesManager::setBrightness(static_cast<uint8_t>(nextLevel));
            updateMenuItems();
            draw();
        }
        break;

        case SettingsMenuItem::Disconnect:
            BLEDeviceManager::disconnect();
            BLEDeviceManager::setManuallyDisconnected(true);
            setStatusText("Select Option");
            setStatusBgColor(M5.Display.color888(0, 0, 100));
            updateMenuItems();
            draw();
            break;
        }
    }

    if (M5.BtnB.wasClicked())
    {
        menuItems.selectNext();
        selectedItem = menuItems.getSelectedIndex();
        draw();
    }
}
