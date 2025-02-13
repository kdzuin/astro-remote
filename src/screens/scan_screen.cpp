#include "scan_screen.h"

#include "../transport/remote_control_manager.h"

ScanScreen::ScanScreen()
    : BaseScreen<ScanMenuItem>("Scan"), lastScanning(false), isConnecting(false) {
    display().setTextAlignment(textAlign::middle_center);
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;
    display().drawString("Wait...", centerX, centerY);

    setStatusText("Scanning...");
    setStatusBgColor(display().getColor(display::colors::YELLOW));  // Yellow for scanning

    // Start scanning immediately when screen is created
    if (!BLEDeviceManager::startScan(5))  // 5-second scan
    {
        setStatusText("Scan failed!");
        setStatusBgColor(display().getColor(display::colors::RED));
    }

    updateMenuItems();
}

void ScanScreen::updateMenuItems() {
    menuItems.clear();

    const auto& discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
    for (const auto& deviceInfo : discoveredDevices) {
        std::string displayName = deviceInfo.getName();
        if (displayName.empty()) {
            displayName = deviceInfo.getAddress();
        }
        menuItems.addItem(ScanMenuItem::Device, displayName);
    }
}

void ScanScreen::drawContent() {
    display().fillScreen(BLACK);

    if (isConnecting) {
        display().setTextAlignment(textAlign::middle_center);
        int centerX = display().width() / 2;
        int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;
        display().drawString("Wait...", centerX, centerY);

        setStatusText("Connecting...");
        setStatusBgColor(display().getColor(display::colors::YELLOW));  // Yellow for connecting
    } else {
        const auto& discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
        if (!discoveredDevices.empty()) {
            // Reset text alignment for menu drawing
            menuItems.draw();

            setStatusText("Select camera");
            setStatusBgColor(display().getColor(display::colors::GRAY_800));
        } else {
            display().setTextAlignment(textAlign::middle_center);
            int centerX = display().width() / 2;
            int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

            display().drawString("Not found", centerX, centerY - 10);
            display().drawString("Restarting...", centerX, centerY + 10);

            setStatusText("No devices");
            setStatusBgColor(display().getColor(display::colors::RED));  // Red for no devices

            // Restart scan after a brief delay if not already scanning
            if (!BLEDeviceManager::isScanning()) {
                if (!BLEDeviceManager::startScan(5)) {
                    setStatusText("Scan failed!");
                }
            }
        }
    }
}

void ScanScreen::update() {
    // Check if scanning state changed
    if (lastScanning != BLEDeviceManager::isScanning()) {
        lastScanning = BLEDeviceManager::isScanning();
        if (!lastScanning)  // Scan just finished
        {
            updateMenuItems();  // Update the menu with any found devices
        }
        draw();
        return;
    }

    if (!BLEDeviceManager::isScanning() && !isConnecting) {
        if ((input().wasButtonPressed(ButtonId::BTN_A) ||
             RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) &&
            !BLEDeviceManager::getDiscoveredDevices().empty()) {
            LOG_PERIPHERAL("[ScanScreen] [Btn] Confirm Button Clicked");
            selectMenuItem();

            if ((input().wasButtonPressed(ButtonId::BTN_B) ||
                 RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) &&
                !BLEDeviceManager::getDiscoveredDevices().empty()) {
                LOG_PERIPHERAL("[ScanScreen] [Btn] Next Button Clicked");
                nextMenuItem();
            }

            if (RemoteControlManager::wasButtonPressed(ButtonId::UP) &&
                !BLEDeviceManager::getDiscoveredDevices().empty()) {
                LOG_PERIPHERAL("[ScanScreen] [Btn] Prev Button Clicked");
                prevMenuItem();
            }
        }
    }
}

void ScanScreen::selectMenuItem() {
    size_t selectedIndex = menuItems.getSelectedIndex();
    if (selectedIndex < BLEDeviceManager::getDiscoveredDevices().size()) {
        const auto& selectedDev = BLEDeviceManager::getDiscoveredDevices()[selectedIndex];
        isConnecting = true;
        draw();

        if (BLEDeviceManager::connectToCamera(selectedDev.device)) {
            setStatusText("Connected!");
            setStatusBgColor(display().getColor(display::colors::GREEN));  // Green for success
            draw();
            delay(500);  // Show success message briefly
            isConnecting = false;
            MenuSystem::goHome();  // Return to previous screen
        } else {
            isConnecting = false;
            setStatusText("Connection failed!");
            setStatusBgColor(display().getColor(display::colors::RED));  // Red for failure
            BLEDeviceManager::clearDiscoveredDevices();
            draw();
            delay(1000);  // Show error message

            if (!BLEDeviceManager::startScan(5)) {
                setStatusText("Scan failed!");
            }
            draw();
        }
    }
}

void ScanScreen::nextMenuItem() {
    menuItems.selectNext();
    draw();
}

void ScanScreen::prevMenuItem() {
    menuItems.selectPrev();
    draw();
}