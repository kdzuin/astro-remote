#include "screens/settings_screen.h"

#include "screens/camera_list_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

SettingsScreen::SettingsScreen() : BaseScreen<SettingsMenuItem>("Settings") {
    setStatusText("Select Option");
    setStatusBgColor(colors::get(colors::NORMAL));
    menuItems.setTitle("Settings Menu");
    updateMenuItems();
}

void SettingsScreen::updateMenuItems() {
    menuItems.clear();
    auto connState = SettingsProcess::getConnectionState();
    auto devState = SettingsProcess::getDeviceState();

    if (connState.isConnected) {
        menuItems.addItem(SettingsMenuItem::Disconnect, "Disconnect");
    } else if (connState.isPaired) {
        menuItems.addItem(SettingsMenuItem::Connect, "Connect");
    }

    // All add/forget/switch lives in the Camera list; this is the entry point.
    menuItems.addItem(SettingsMenuItem::Cameras, "Cameras");
    menuItems.addSeparator();
    menuItems.addItem(SettingsMenuItem::AutoConnect, "AutoConnect",
                      (connState.isAutoConnectEnabled ? "On" : "Off"), true);

    menuItems.addItem(SettingsMenuItem::Brightness, "Brightness",
                      std::to_string(devState.brightness), true);
    menuItems.addSeparator();
    menuItems.addItem(SettingsMenuItem::Battery, "Battery",
                      std::to_string(devState.batteryLevel) + "%",
                      SettingsProcess::getBatteryStatusColor(devState.batteryLevel), false);
}

void SettingsScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void SettingsScreen::update() {
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
        RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[SettingsScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    }
}

void SettingsScreen::selectMenuItem() {
    switch (menuItems.getSelectedId()) {
        case SettingsMenuItem::Connect:
            if (SettingsProcess::connectToDevice()) {
                setStatusText("Connected!");
                setStatusBgColor(colors::get(colors::SUCCESS));
            } else {
                setStatusText("Failed to connect!");
                setStatusBgColor(colors::get(colors::ERROR));
            }
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Cameras:
            // Navigates away and deletes this screen; return immediately.
            MenuSystem::setScreen(new CameraListScreen());
            return;

        case SettingsMenuItem::AutoConnect:
            SettingsProcess::toggleAutoConnect();
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Brightness:
            SettingsProcess::cycleBrightness();
            updateMenuItems();
            draw();
            break;

        case SettingsMenuItem::Disconnect:
            SettingsProcess::disconnectDevice();
            setStatusText("Select Option");
            setStatusBgColor(colors::get(colors::NORMAL));
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
