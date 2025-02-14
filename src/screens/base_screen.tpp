#pragma once

#include "utils/colors.h"

template <typename MenuItemType>
BaseScreen<MenuItemType>::BaseScreen(const char* name)
    : screenName(name), statusText(""), statusBgColor(0) {
    statusBgColor = colors::get(colors::GRAY_800);
    M5.Display.fillScreen(colors::get(colors::BLACK));
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::draw() {
    // Draw main content in the upper area
    M5.Display.setClipRect(0, 0, M5.Display.width(), M5.Display.height() - STATUS_BAR_HEIGHT);
    drawContent();
    M5.Display.clearClipRect();
    drawStatusBar();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawConnectionStatus() const {
    const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

    // Draw connection status indicator
    if (BLEDeviceManager::isConnected()) {
        // Connected - solid green line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY,
                            colors::get(colors::GREEN));
    } else if (BLEDeviceManager::isPaired()) {
        // Paired but not connected - yellow line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY,
                            colors::get(colors::YELLOW));
    } else {
        // Not paired - red line
        M5.Display.drawLine(0, statusBarY, M5.Display.width(), statusBarY,
                            colors::get(colors::RED));
    }
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawStatusBar() const {
    const int statusBarY = M5.Display.height() - STATUS_BAR_HEIGHT;

    // Draw status bar background
    M5.Display.fillRect(0, statusBarY, M5.Display.width(), STATUS_BAR_HEIGHT, statusBgColor);

    // Draw status text if any
    if (!statusText.empty()) {
        M5.Display.setTextSize(1);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(colors::get(colors::WHITE));
        M5.Display.drawString(statusText.c_str(), M5.Display.width() / 2,
                              statusBarY + STATUS_BAR_HEIGHT / 2);
    }

    drawConnectionStatus();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::checkConnection() {
    // Check connection every 1 second
    if (millis() - lastConnectionCheck < 1000) {
        return;
    }
    lastConnectionCheck = millis();

    if (!BLEDeviceManager::isConnected()) {
        if (BLEDeviceManager::isPaired() && !BLEDeviceManager::wasManuallyDisconnected()) {
            setStatusText("Reconnecting...");
            setStatusBgColor(colors::get(colors::GRAY_500));
            drawStatusBar();

            if (reconnectAttempts < 10) {
                if (BLEDeviceManager::connectToSavedDevice()) {
                    // Connection restored
                    reconnectAttempts = 0;
                    setStatusText("Connected");
                    setStatusBgColor(colors::get(colors::SUCCESS));
                    updateMenuItems();
                    draw();
                } else {
                    reconnectAttempts++;
                }
            } else {
                // Max attempts reached, show error and update menu
                setStatusText("Connection lost");
                setStatusBgColor(colors::get(colors::ERROR));
                reconnectAttempts++;  // Increment to avoid showing this message again
                updateMenuItems();
                BLEDeviceManager::disconnect();
                draw();
            }
        } else {
            setStatusText("Not connected");
            setStatusBgColor(colors::get(colors::ERROR));
        }
    } else {
        setStatusText("Connected");
        setStatusBgColor(colors::get(colors::SUCCESS));
    }
}
