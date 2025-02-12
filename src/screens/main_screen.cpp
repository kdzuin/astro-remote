#include "components/menu_system.h"
#include "screens/main_screen.h"
#include "screens/video_screen.h"
#include "screens/photo_screen.h"
#include "screens/astro_screen.h"
#include "screens/settings_screen.h"
#include "screens/manual_screen.h"
#include "transport/remote_control_manager.h"

MainScreen::MainScreen() : BaseScreen<MainMenuItem>("Main")
{
    auto &display = MenuSystem::getHardware()->getDisplay();

    menuItems.setTitle("Main Menu");
    updateMenuItems();

    if (BLEDeviceManager::isConnected())
    {
        setStatusText("Connected");
        setStatusBgColor(display.color(0, 100, 0));
    }
    else
    {
        setStatusText("Select Option");
        setStatusBgColor(display.color(0, 0, 100));
    }

    // Try to auto-connect on startup if enabled
    if (BLEDeviceManager::isAutoConnectEnabled() && BLEDeviceManager::isPaired() && !BLEDeviceManager::wasManuallyDisconnected())
    {
        setStatusText("Auto-connecting...");
        setStatusBgColor(display.color(128, 128, 0));
        drawStatusBar();

        if (BLEDeviceManager::connectToSavedDevice())
        {
            setStatusText("Connected");
            setStatusBgColor(display.color(0, 200, 0));
            updateMenuItems();
            draw();
        }
    }
}

void MainScreen::updateMenuItems()
{
    const bool isConnected = BLEDeviceManager::isConnected();

    menuItems.clear();
    if (!isConnected)
    {
        menuItems.addItem(MainMenuItem::Connect, "Connect");
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }
    else
    {
        menuItems.addItem(MainMenuItem::Photo, "Photo");
        menuItems.addItem(MainMenuItem::Video, "Video");
        menuItems.addItem(MainMenuItem::Astro, "Astro");
        menuItems.addItem(MainMenuItem::Manual, "Manual");
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }
}

void MainScreen::drawContent()
{
    menuItems.draw();
}

void MainScreen::update()
{
    auto &input = MenuSystem::getHardware()->getInput();

    // Check for button presses
    if (input.wasButtonPressed(ButtonId::BTN_A) || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM))
    {
        LOG_PERIPHERAL("[MainScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    }
    else if (input.wasButtonPressed(ButtonId::BTN_B) || RemoteControlManager::wasButtonPressed(ButtonId::DOWN))
    {
        LOG_PERIPHERAL("[MainScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }
    else if (RemoteControlManager::wasButtonPressed(ButtonId::UP))
    {
        LOG_PERIPHERAL("[MainScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    checkConnection();
}

void MainScreen::selectMenuItem()
{
    switch (menuItems.getSelectedId())
    {
    case MainMenuItem::Connect:
        if (BLEDeviceManager::isConnected())
        {
            BLEDeviceManager::disconnect();
            setStatusText("Disconnected");
            setStatusBgColor(MenuSystem::getHardware()->getDisplay().color(100, 0, 0));
            updateMenuItems();
            draw();
        }
        else if (BLEDeviceManager::isPaired())
        {
            setStatusText("Connecting...");
            setStatusBgColor(MenuSystem::getHardware()->getDisplay().color(128, 128, 0));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice())
            {
                setStatusText("Connected");
                setStatusBgColor(MenuSystem::getHardware()->getDisplay().color(0, 200, 0));
                updateMenuItems();
                draw();
            }
        }
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
    default:
        break;
    }
}

void MainScreen::nextMenuItem()
{
    menuItems.selectNext();
    draw();
}

void MainScreen::prevMenuItem()
{
    menuItems.selectPrev();
    draw();
}
