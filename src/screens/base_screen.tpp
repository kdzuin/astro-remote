#pragma once

template <typename MenuItemType>
BaseScreen<MenuItemType>::BaseScreen(const char *name) : screenName(name), statusText(""), statusBgColor(0)
{
    auto &display = MenuSystem::getHardware()->getDisplay();
    statusBgColor = display.color(55, 55, 55);
    display.fillScreen(display.color(0, 0, 0));
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::draw()
{
    auto &display = MenuSystem::getHardware()->getDisplay();

    // Draw main content in the upper area
    M5.Display.setClipRect(0, 0, M5.Display.width(), M5.Display.height() - STATUS_BAR_HEIGHT);
    drawContent();
    M5.Display.clearClipRect();

    // Draw status bar at the bottom
    drawStatusBar();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawConnectionStatus() const
{
    auto &display = MenuSystem::getHardware()->getDisplay();
    const int statusBarY = display.height() - STATUS_BAR_HEIGHT;

    // Draw connection status indicator
    if (BLEDeviceManager::isConnected())
    {
        // Connected - solid green line
        display.drawLine(0, statusBarY, display.width(), statusBarY, display.color(0, 255, 0));
    }
    else if (BLEDeviceManager::isPaired())
    {
        // Paired but not connected - yellow line
        display.drawLine(0, statusBarY, display.width(), statusBarY, display.color(255, 255, 0));
    }
    else
    {
        // Not paired - red line
        display.drawLine(0, statusBarY, display.width(), statusBarY, display.color(255, 0, 0));
    }
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawStatusBar() const
{
    auto &display = MenuSystem::getHardware()->getDisplay();
    const int statusBarY = display.height() - STATUS_BAR_HEIGHT;

    // Draw status bar background
    display.fillRect(0, statusBarY, display.width(), STATUS_BAR_HEIGHT, statusBgColor);

    // Draw status text
    if (!statusText.empty())
    {
        display.setTextSize(1);
        display.setTextDatum(textAlign::middle_center);
        display.setTextColor(display.color(255, 255, 255));
        display.drawString(statusText.c_str(), display.width() / 2, statusBarY + STATUS_BAR_HEIGHT / 2);
    }

    // Draw connection status line at the top of status bar
    drawConnectionStatus();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::checkConnection()
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
            setStatusBgColor(M5.Display.color565(128, 128, 128));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice())
            {
                // Connection restored
                reconnectAttempts = 0;
                setStatusText("Connected");
                setStatusBgColor(M5.Display.color565(0, 200, 0));
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
            setStatusBgColor(M5.Display.color565(200, 0, 0));
            reconnectAttempts++; // Increment to avoid showing this message again
            updateMenuItems();
            draw();
        }
    }
}
