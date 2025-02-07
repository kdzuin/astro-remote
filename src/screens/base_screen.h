#pragma once

#include <M5Unified.h>
#include <vector>
#include <string>
#include <functional>
#include "../components/selectable_list.h"
#include "../transport/ble_device.h"

class BaseScreen
{
public:
    BaseScreen(const char *name) : screenName(name), statusText(""), statusBgColor(M5.Display.color888(55, 55, 55)) {}
    virtual ~BaseScreen() = default;

    // Pure virtual functions that must be implemented by derived screens
    virtual void update() = 0;
    virtual void beforeExit() {};

    const int STATUS_BAR_HEIGHT = 20;

    // Base draw implementation with status bar
    virtual void draw()
    {
        // First clear the entire screen
        M5.Display.fillScreen(BLACK);

        // Draw main content in the upper area
        M5.Display.setClipRect(0, 0, M5.Display.width(), M5.Display.height() - STATUS_BAR_HEIGHT);
        drawContent();
        M5.Display.clearClipRect();

        // Draw status bar at the bottom
        drawStatusBar();
    }

    void drawConnectionStatus() const
    {
        const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

        // Draw connection status indicator
        if (BLEDeviceManager::isConnected())
        {
            // Connected - solid green line
            M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color888(0, 255, 0));
        }
        else if (BLEDeviceManager::isPaired())
        {
            // Paired but not connected - yellow line
            M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color888(255, 255, 0));
        }
        else
        {
            // Not paired - red line
            M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY, M5.Display.color888(255, 0, 0));
        }
    }

    // Draw the status bar only
    void drawStatusBar() const
    {
        const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

        // Draw status bar background
        M5.Display.fillRect(0, statusBarY, M5.Display.width(), STATUS_BAR_HEIGHT, statusBgColor);

        // Draw status text
        M5.Display.setTextColor(M5.Display.color565(255, 255, 255));
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(statusText.c_str(), M5.Display.width() / 2, statusBarY + STATUS_BAR_HEIGHT / 2);

        drawConnectionStatus();
    }

    // New pure virtual function for content drawing
    virtual void drawContent() = 0;

    const char *getName() const { return screenName; }

    // Status bar methods
    void setStatusText(const std::string &text) { statusText = text; }
    void setStatusBgColor(uint32_t color) { statusBgColor = color; }

protected:
    SelectableList menuItems;
    const char *screenName;
    std::string statusText;
    uint32_t statusBgColor;
};
