#include "screens/settings_screen.h"

#include "hardware_interface.h"
#include "screens/scan_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/display_constants.h"
#include "utils/preferences.h"

using namespace display::colors;

SettingsScreen::SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings") {
    setStatusText("Select Option");
    setStatusBgColor(display().getColor(display::colors::NORMAL));
    menuItems.setTitle("Settings Menu");
    updateMenuItems();
}

void SettingsScreen::updateMenuItems() {
    menuItems.clear();
    auto& power = MenuSystem::getHardware()->getPower();

    if (BLEDeviceManager::isConnected()) {
        menuItems.addItem(SettingsMenuItem::Disconnect, "Disconnect");
    } else if (BLEDeviceManager::isPaired()) {
        menuItems.addItem(SettingsMenuItem::Connect, "Connect");
    }

    if (BLEDeviceManager::isPaired()) {
        menuItems.addItem(SettingsMenuItem::Forget, "Forget Camera");
    } else {
        menuItems.addItem(SettingsMenuItem::Scan, "Scan New");
    }

    menuItems.addItem(SettingsMenuItem::AutoConnect, "AutoConnect",
                      (BLEDeviceManager::isAutoConnectEnabled() ? "On" : "Off"), true);

    menuItems.addItem(SettingsMenuItem::Brightness, "Brightness",
                      std::to_string(PreferencesManager::getBrightness()), true);
    menuItems.addItem(SettingsMenuItem::Battery, "Battery",
                      std::to_string(power.getBatteryLevel()) + "%",
                      getStatusBgColor(power.getBatteryLevel()), false);
}

void SettingsScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void SettingsScreen::update() {
    if (input().wasButtonPressed(ButtonId::BTN_B) ||
        RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (input().wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    }
}

void SettingsScreen::selectMenuItem() {
    switch (menuItems.getSelectedId()) {
        case SettingsMenuItem::Connect:
            if (BLEDeviceManager::connectToSavedDevice()) {
                setStatusText("Connected!");
                setStatusBgColor(display().getColor(display::colors::SUCCESS));
            } else {
                setStatusText("Failed to connect!");
                setStatusBgColor(display().getColor(display::colors::ERROR));
            }
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Forget:
            BLEDeviceManager::unpairCamera();
            BLEDeviceManager::setManuallyDisconnected(true);
            setStatusText("Select Option");
            setStatusBgColor(display().getColor(display::colors::NORMAL));
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Scan:
            MenuSystem::setScreen(new ScanScreen());
            break;

        case SettingsMenuItem::AutoConnect: {
            bool newState = !BLEDeviceManager::isAutoConnectEnabled();
            BLEDeviceManager::setAutoConnect(newState);
            PreferencesManager::setAutoConnect(newState);
            updateMenuItems();
            draw();
        } break;

        case SettingsMenuItem::Brightness: {
            auto nextLevel =
                PreferencesManager::getNextBrightnessLevel(PreferencesManager::getBrightness());
            display().setBrightness(static_cast<uint8_t>(nextLevel));
            PreferencesManager::setBrightness(static_cast<uint8_t>(nextLevel));
            updateMenuItems();
            draw();
        } break;

        case SettingsMenuItem::Disconnect:
            BLEDeviceManager::disconnect();
            BLEDeviceManager::setManuallyDisconnected(true);
            setStatusText("Select Option");
            setStatusBgColor(display().getColor(display::colors::NORMAL));
            updateMenuItems();
            draw();
            break;
    }
}

void SettingsScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void SettingsScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

uint16_t SettingsScreen::getStatusBgColor(int batteryLevel) {
    if (batteryLevel < 20) {
        return display().getColor(display::colors::ERROR);  // Critical
    }
    if (batteryLevel < 50) {
        return display().getColor(display::colors::WARNING);  // Warning
    }
    return display().getColor(display::colors::SUCCESS);  // Good
}
