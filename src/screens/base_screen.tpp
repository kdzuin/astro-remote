#pragma once

#include "utils/display_constants.h"

template <typename MenuItemType>
BaseScreen<MenuItemType>::BaseScreen(const char* name)
    : screenName(name), statusText(""), statusBgColor(0) {
    auto& display = MenuSystem::getHardware()->getDisplay();
    statusBgColor = display.getColor(display::colors::GRAY_800);
    display.fillScreen(display.getColor(display::colors::BLACK));
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::draw() {
    auto& display = MenuSystem::getHardware()->getDisplay();

    // Draw main content in the upper area
    display.setClipRect(0, 0, display.width(), display.height() - STATUS_BAR_HEIGHT);
    drawContent();
    display.clearClipRect();
    drawStatusBar();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawConnectionStatus() const {
    auto& display = MenuSystem::getHardware()->getDisplay();
    const int statusBarY = display.height() - STATUS_BAR_HEIGHT;

    // Draw connection status indicator
    if (BLEDeviceManager::isConnected()) {
        // Connected - solid green line
        display.drawLine(0, statusBarY, display.width(), statusBarY,
                         display.getColor(display::colors::GREEN));
    } else if (BLEDeviceManager::isPaired()) {
        // Paired but not connected - yellow line
        display.drawLine(0, statusBarY, display.width(), statusBarY,
                         display.getColor(display::colors::YELLOW));
    } else {
        // Not paired - red line
        display.drawLine(0, statusBarY, display.width(), statusBarY,
                         display.getColor(display::colors::RED));
    }
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::drawStatusBar() const {
    auto& display = MenuSystem::getHardware()->getDisplay();
    const int statusBarY = display.height() - STATUS_BAR_HEIGHT;

    // Draw status bar background
    display.fillRect(0, statusBarY, display.width(), STATUS_BAR_HEIGHT, statusBgColor);

    // Draw status text
    if (!statusText.empty()) {
        display.setTextSize(1);
        display.setTextAlignment(textAlign::middle_center);
        display.setTextColor(display.getColor(display::colors::WHITE));
        display.drawString(statusText.c_str(), display.width() / 2,
                           statusBarY + STATUS_BAR_HEIGHT / 2);
    }

    // Draw connection status line at the top of status bar
    drawConnectionStatus();
}

template <typename MenuItemType>
void BaseScreen<MenuItemType>::checkConnection() {
    // Check connection every 1 second
    if (millis() - lastConnectionCheck < 1000) {
        return;
    }
    lastConnectionCheck = millis();

    auto& display = MenuSystem::getHardware()->getDisplay();

    // Only attempt reconnection if:
    // 1. We were connected but lost connection
    // 2. We haven't manually disconnected
    // 3. Auto-connect is enabled
    if (!BLEDeviceManager::isConnected() && BLEDeviceManager::isPaired() &&
        !BLEDeviceManager::wasManuallyDisconnected() && BLEDeviceManager::isAutoConnectEnabled()) {
        if (reconnectAttempts < 10) {
            setStatusText("Reconnecting...");
            setStatusBgColor(display.getColor(display::colors::GRAY_500));
            drawStatusBar();

            if (BLEDeviceManager::connectToSavedDevice()) {
                // Connection restored
                reconnectAttempts = 0;
                setStatusText("Connected");
                setStatusBgColor(display.getColor(display::colors::SUCCESS));
                updateMenuItems();
                draw();
            } else {
                reconnectAttempts++;
            }
        } else if (reconnectAttempts == 10) {
            // Max attempts reached, show error and update menu
            setStatusText("Connection lost");
            setStatusBgColor(display.getColor(display::colors::ERROR));
            reconnectAttempts++;  // Increment to avoid showing this message again
            updateMenuItems();
            draw();
        }
    }
}
