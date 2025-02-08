#include "main_screen.h"
#include "video_screen.h"
#include "photo_screen.h"
#include "astro_screen.h"
#include "settings_screen.h"
#include "manual_screen.h"
#include "../components/menu_system.h"

MainScreen::MainScreen() : BaseScreen<MainMenuItem>("Main")
{
    menuItems.setTitle("Main Menu");
    updateMenuItems();

    if (BLEDeviceManager::isConnected())
    {
        setStatusText("Connected");
        setStatusBgColor(M5.Display.color565(0, 100, 0));
    }
    else
    {
        setStatusText("Select Option");
        setStatusBgColor(M5.Display.color565(0, 0, 100));
    }

    // Try to auto-connect on startup if enabled
    if (BLEDeviceManager::isAutoConnectEnabled() && BLEDeviceManager::isPaired() && !BLEDeviceManager::wasManuallyDisconnected())
    {
        setStatusText("Auto-connecting...");
        setStatusBgColor(M5.Display.color565(128, 128, 0));
        drawStatusBar();

        if (BLEDeviceManager::connectToSavedDevice())
        {
            setStatusText("Connected");
            setStatusBgColor(M5.Display.color565(0, 200, 0));
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
    }
    menuItems.addItem(MainMenuItem::Video, "Video Remote", isConnected);
    menuItems.addItem(MainMenuItem::Photo, "Photo Remote", isConnected);
    menuItems.addItem(MainMenuItem::Astro, "Astro Remote", isConnected);
    menuItems.addItem(MainMenuItem::Manual, "Manual Control", isConnected);
    menuItems.addItem(MainMenuItem::Settings, "Settings");
}

void MainScreen::drawContent()
{
    menuItems.draw();
}

void MainScreen::update()
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
        }
    }

    if (M5.BtnB.wasClicked())
    {
        menuItems.selectNext();
        draw();
    }
}
