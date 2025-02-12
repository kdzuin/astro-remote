#include "settings_screen.h"
#include "scan_screen.h"
#include "../utils/preferences.h"
#include "../transport/remote_control_manager.h"

SettingsScreen::SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings")
{
    setStatusText("Select Option");
    setStatusBgColor(M5.Display.color565(0, 0, 100));
    menuItems.setTitle("Settings Menu");
    updateMenuItems();
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
                      "AutoConnect",
                      (BLEDeviceManager::isAutoConnectEnabled() ? "On" : "Off"),
                      true);

    menuItems.addItem(SettingsMenuItem::Brightness, "Brightness", std::to_string(PreferencesManager::getBrightness()), true);
    menuItems.addItem(SettingsMenuItem::Battery, "Battery", std::to_string(M5.Power.getBatteryLevel()) + "%", getStatusBgColor(M5.Power.getBatteryLevel()), false);
}

void SettingsScreen::drawContent()
{
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void SettingsScreen::update()
{
    if (M5.BtnB.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::DOWN))
    {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP))
    {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (M5.BtnA.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM))
    {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    }
}

void SettingsScreen::selectMenuItem()
{
    switch (menuItems.getSelectedId())
    {
    case SettingsMenuItem::Connect:
        if (BLEDeviceManager::connectToSavedDevice())
        {
            setStatusText("Connected!");
            setStatusBgColor(M5.Display.color565(0, 200, 0));
        }
        else
        {
            setStatusText("Connection failed!");
            setStatusBgColor(M5.Display.color565(200, 0, 0));
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
        setStatusBgColor(M5.Display.color565(0, 0, 100));
        updateMenuItems();
        draw();
        break;
    }
}

void SettingsScreen::nextMenuItem()
{
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void SettingsScreen::prevMenuItem()
{
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

uint16_t SettingsScreen::getStatusBgColor(int batteryLevel)
{
    if (batteryLevel < 20)
    {
        return M5.Display.color565(200, 0, 0); // Red
    }
    else if (batteryLevel < 50)
    {
        return M5.Display.color565(200, 200, 0); // Yellow
    }

    return M5.Display.color565(0, 200, 0); // Green
}
