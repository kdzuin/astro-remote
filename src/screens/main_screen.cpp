#include "screens/main_screen.h"

#include "components/menu_system.h"
#include "screens/astro_screen.h"
#include "screens/focus_screen.h"
#include "screens/manual_screen.h"
#include "screens/photo_screen.h"
#include "screens/settings_screen.h"
#include "screens/video_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

MainScreen::MainScreen() : BaseScreen<MainMenuItem>("Main") {
    menuItems.setTitle("Main Menu");
    updateMenuItems();

    if (BLEDeviceManager::isConnected()) {
        setStatusText("Connected");
        setStatusBgColor(colors::get(colors::SUCCESS));
    } else {
        setStatusText("Select Option");
        setStatusBgColor(colors::get(colors::GRAY_800));
    }

    // Try to auto-connect on startup if enabled
    if (BLEDeviceManager::isAutoConnectEnabled() && BLEDeviceManager::isPaired() &&
        !BLEDeviceManager::wasManuallyDisconnected()) {
        setStatusText("Auto-connecting...");
        setStatusBgColor(colors::get(colors::WARNING));
        drawStatusBar();

        if (BLEDeviceManager::connectToSavedDevice()) {
            setStatusText("Connected");
            setStatusBgColor(colors::get(colors::SUCCESS));
            updateMenuItems();
            draw();
        }
    }
}

void MainScreen::updateMenuItems() {
    const bool isConnected = BLEDeviceManager::isConnected();

    menuItems.clear();
    if (!isConnected) {
        menuItems.addItem(MainMenuItem::Connect, "Connect");
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    } else {
        menuItems.addItem(MainMenuItem::Focus, "Focus");
        menuItems.addItem(MainMenuItem::Photo, "Photo");
        menuItems.addItem(MainMenuItem::Video, "Video");
        menuItems.addItem(MainMenuItem::Astro, "Astro");
        menuItems.addItem(MainMenuItem::Manual, "Manual");
        menuItems.addSeparator();
        menuItems.addItem(MainMenuItem::Disconnect, "Disconnect");
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }
}

void MainScreen::drawContent() {
    menuItems.draw();
}

void MainScreen::update() {
    // Check for button presses
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[MainScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    } else if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
               RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_PERIPHERAL("[MainScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    } else if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_PERIPHERAL("[MainScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    checkConnection();
}

void MainScreen::selectMenuItem() {
    switch (menuItems.getSelectedId()) {
        case MainMenuItem::Connect:

            setStatusText("Connecting...");
            setStatusBgColor(colors::get(colors::IN_PROGRESS));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice()) {
                setStatusText("Connected!");
                setStatusBgColor(colors::get(colors::SUCCESS));
                drawStatusBar();

            } else {
                setStatusText("Failed to connect!");
                setStatusBgColor(colors::get(colors::ERROR));
                drawStatusBar();
            }
            updateMenuItems();
            draw();
            break;
        case MainMenuItem::Disconnect:
            BLEDeviceManager::disconnect();
            setStatusText("Disconnected!");
            setStatusBgColor(colors::get(colors::WARNING));
            drawStatusBar();
            updateMenuItems();
            draw();
            break;
        case MainMenuItem::Settings:
            MenuSystem::setScreen(new SettingsScreen());
            break;
        case MainMenuItem::Photo:
            MenuSystem::setScreen(new PhotoScreen());
            break;
        case MainMenuItem::Video:
            MenuSystem::setScreen(new VideoScreen());
            break;
        case MainMenuItem::Astro:
            MenuSystem::setScreen(new AstroScreen());
            break;
        case MainMenuItem::Manual:
            MenuSystem::setScreen(new ManualScreen());
            break;
        case MainMenuItem::Focus:
            MenuSystem::setScreen(new FocusScreen());
            break;
        default:
            break;
    }
}

void MainScreen::nextMenuItem() {
    menuItems.selectNext();
    draw();
}

void MainScreen::prevMenuItem() {
    menuItems.selectPrev();
    draw();
}
