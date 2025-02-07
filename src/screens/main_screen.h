#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "video_screen.h"
#include "../components/selectable_list.h"
#include "../transport/ble_device.h"

// Menu item IDs
enum class MainMenuItem
{
    Connection,
    Video,
    Control,
    Settings
};

class MainScreen : public BaseScreen<MainMenuItem>
{
public:
    MainScreen() : BaseScreen<MainMenuItem>("Main"), reconnectAttempts(0), lastConnectionCheck(0)
    {
        menuItems.setTitle("Main Menu");
        BLEDeviceManager::setAutoConnect(false);

        // Initialize screen state based on current connection
        if (BLEDeviceManager::isConnected())
        {
            setStatusText("Connected");
            setStatusBgColor(M5.Display.color888(0, 200, 0));
        }
        else
        {
            setStatusText("Select Option");
            setStatusBgColor(M5.Display.color888(0, 0, 200));
        }

        updateMenuItems();

        // Try to auto-connect on startup if enabled
        if (BLEDeviceManager::isAutoConnectEnabled() && BLEDeviceManager::isPaired() && !BLEDeviceManager::wasManuallyDisconnected())
        {
            setStatusText("Auto-connecting...");
            setStatusBgColor(M5.Display.color888(128, 128, 0));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice())
            {
                setStatusText("Connected");
                setStatusBgColor(M5.Display.color888(0, 200, 0));
                updateMenuItems();
                draw();
            }
        }
    }

    void updateMenuItems()
    {
        const bool isConnected = BLEDeviceManager::isConnected();

        menuItems.clear();
        menuItems.addItem(MainMenuItem::Connection, isConnected ? "Disconnect" : "Connect");
        menuItems.addItem(MainMenuItem::Video, "Video", isConnected);
        menuItems.addItem(MainMenuItem::Control, "Control", isConnected);
        menuItems.addItem(MainMenuItem::Settings, "Settings");
    }

    void drawContent() override
    {
        menuItems.draw();
    }

    void checkConnection()
    {
        // Check connection every 1 second
        if (millis() - lastConnectionCheck < 1000)
        {
            return;
        }
        lastConnectionCheck = millis();

        // Only attempt reconnection if:
        // 1. We were connected but lost connection
        // 2. We haven't manually disconnected
        // 3. Auto-connect is enabled
        if (!BLEDeviceManager::isConnected() &&
            BLEDeviceManager::isPaired() &&
            !BLEDeviceManager::wasManuallyDisconnected() &&
            BLEDeviceManager::isAutoConnectEnabled())
        {
            if (reconnectAttempts < 10)
            {
                setStatusText("Reconnecting...");
                setStatusBgColor(M5.Display.color888(128, 128, 128));
                drawStatusBar();

                if (BLEDeviceManager::connectToSavedDevice())
                {
                    // Connection restored
                    reconnectAttempts = 0;
                    setStatusText("Connected");
                    setStatusBgColor(M5.Display.color888(0, 200, 0));
                    updateMenuItems();
                    draw();
                }
                else
                {
                    reconnectAttempts++;
                }
            }
            else if (reconnectAttempts == 10)
            {
                // Max attempts reached, show error and update menu
                setStatusText("Connection lost");
                setStatusBgColor(M5.Display.color888(200, 0, 0));
                reconnectAttempts++; // Increment to avoid showing this message again
                updateMenuItems();
                draw();
            }
        }
    }

    void update() override
    {
        checkConnection();
        const bool isConnected = BLEDeviceManager::isConnected();

        if (M5.BtnA.wasClicked() && !M5.BtnB.wasPressed())
        {

            // Handle menu selection
            switch (menuItems.getSelectedId())
            {
            case MainMenuItem::Connection:
                if (!isConnected)
                {
                    // Try to connect
                    setStatusText("Connecting...");
                    setStatusBgColor(M5.Display.color888(128, 128, 0));
                    drawStatusBar();

                    // Reset manual disconnect flag when user explicitly tries to connect
                    BLEDeviceManager::setManuallyDisconnected(false);

                    if (BLEDeviceManager::connectToSavedDevice())
                    {
                        setStatusText("Connected");
                        setStatusBgColor(M5.Display.color888(0, 200, 0));
                        reconnectAttempts = 0;
                    }
                    else
                    {
                        setStatusText("Connection failed");
                        setStatusBgColor(M5.Display.color888(200, 0, 0));
                        drawStatusBar();
                    }
                }
                else
                {

                    // Disconnect
                    BLEDeviceManager::disconnectCamera();
                    // Set manual disconnect flag to prevent auto-reconnect
                    BLEDeviceManager::setManuallyDisconnected(true);
                    setStatusText("Disconnected");
                    setStatusBgColor(M5.Display.color888(55, 55, 55));
                }
                updateMenuItems();
                draw();

                break;

            case MainMenuItem::Video:
                MenuSystem::setScreen(new VideoScreen());
                break;

            case MainMenuItem::Control:
                break;

            case MainMenuItem::Settings:
                break;
            }
        }

        if (M5.BtnB.wasClicked())
        {
            menuItems.selectNext();
            draw();
        }
    }

    void setScreenChangeCallback(std::function<void(BaseScreen<MainMenuItem> *)> callback)
    {
        onScreenChange = callback;
    }

private:
    unsigned long lastConnectionCheck;
    int reconnectAttempts;
    std::function<void(BaseScreen<MainMenuItem> *)> onScreenChange;
};
