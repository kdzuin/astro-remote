#include "main_screen.h"
#include "video_screen.h"
#include "photo_screen.h"
#include "astro_screen.h"
#include "settings_screen.h"
#include "manual_screen.h"
#include "../components/menu_system.h"
#include "../transport/encoder_device.h"

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
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();

    EncoderDevice::update();

    // Handle encoder rotation
    int16_t delta = EncoderDevice::getDelta();
    if (delta > 0 || M5.BtnB.wasClicked())
    {
        nextMenuItem();
    }

    // Handle clicks
    if (M5.BtnA.wasClicked() || EncoderDevice::wasClicked())
    {
        Serial.printf("[MainScreen] Click at %lu\n", now);
        selectMenuItem();
    }

    lastUpdate = now;
}

void MainScreen::selectMenuItem()
{
    Serial.printf("[MainScreen] selectMenuItem called at %lu\n", millis());
    EncoderDevice::indicateClick();

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

void MainScreen::nextMenuItem()
{
    menuItems.selectNext();
    EncoderDevice::indicateNext();
    draw();
}

void MainScreen::prevMenuItem()
{
    // menuItems.selectPrev();
    EncoderDevice::indicatePrev();
    draw();
}
