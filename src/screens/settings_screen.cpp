#include "settings_screen.h"

SettingsScreen::SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings"), brightness(M5.Display.getBrightness())
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

void SettingsScreen::drawContent()
{
    menuItems.draw();
}

void SettingsScreen::update()
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
