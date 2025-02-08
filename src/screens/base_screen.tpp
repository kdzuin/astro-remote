#pragma once

template <typename MenuItemType>
BaseScreen<MenuItemType>::BaseScreen(const char *name) : screenName(name), statusText(""), statusBgColor(M5.Display.color565(55, 55, 55))
{
    M5.Display.fillScreen(BLACK);
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::draw()
{
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
    const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

    // Draw connection status indicator
    if (BLEDeviceManager::isConnected())
    {
        // Connected - solid green line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color565(0, 255, 0));
    }
    else if (BLEDeviceManager::isPaired())
    {
        // Paired but not connected - yellow line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color565(255, 255, 0));
    }
    else
    {
        // Not paired - red line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color565(255, 0, 0));
    }
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawStatusBar() const
{
    const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

    // Draw status bar background
    M5.Display.fillRect(0, statusBarY, M5.Display.width(), STATUS_BAR_HEIGHT, statusBgColor);

    // Draw status text
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(M5.Display.color565(255, 255, 255));
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString(statusText.c_str(), M5.Display.width() / 2, statusBarY + STATUS_BAR_HEIGHT / 2);

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
