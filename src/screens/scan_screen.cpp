#include "screens/scan_screen.h"
#include "transport/remote_control_manager.h"

ScanScreen::ScanScreen()
    : BaseScreen<ScanMenuItem>("Scan"), lastScanning(false), isConnecting(false) {
    display().setTextAlignment(textAlign::middle_center);
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;
    display().drawString("Wait...", centerX, centerY);

    auto state = ScanProcess::getState();
    setStatusText(ScanProcess::getStatusText(state.status));
    setStatusBgColor(ScanProcess::getStatusColor(state.status));

    // Start scanning immediately when screen is created
    if (!ScanProcess::startScan(5)) {  // 5-second scan
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Failed));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Failed));
    }

    updateMenuItems();
}

void ScanScreen::updateMenuItems() {
    menuItems.clear();

    auto state = ScanProcess::getState();
    for (const auto& deviceInfo : state.discoveredDevices) {
        std::string displayName = deviceInfo.getName();
        if (displayName.empty()) {
            displayName = deviceInfo.getAddress();
        }
        menuItems.addItem(ScanMenuItem::Device, displayName);
    }
}

void ScanScreen::drawContent() {
    display().fillScreen(display().getColor(display::colors::BLACK));

    auto state = ScanProcess::getState();
    if (isConnecting) {
        state.status = ScanProcess::Status::Connecting;
    }

    if (isConnecting) {
        display().setTextAlignment(textAlign::middle_center);
        int centerX = display().width() / 2;
        int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;
        display().drawString("Wait...", centerX, centerY);
    } else if (state.discoveredDevices.empty()) {
        display().setTextAlignment(textAlign::middle_center);
        int centerX = display().width() / 2;
        int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

        display().drawString("Not found", centerX, centerY - 10);
        display().drawString("Restarting...", centerX, centerY + 10);

        // Restart scan after a brief delay if not already scanning
        if (!state.isScanning) {
            if (!ScanProcess::startScan(5)) {
                setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Failed));
                setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Failed));
            }
        }
    } else {
        // Reset text alignment for menu drawing
        menuItems.draw();
    }

    setStatusText(ScanProcess::getStatusText(state.status));
    setStatusBgColor(ScanProcess::getStatusColor(state.status));
}

void ScanScreen::update() {
    auto state = ScanProcess::getState();

    // Check if scanning state changed
    if (lastScanning != state.isScanning) {
        lastScanning = state.isScanning;
        if (!lastScanning) {  // Scan just finished
            updateMenuItems();  // Update the menu with any found devices
        }
        draw();
        return;
    }

    if (!state.isScanning && !isConnecting) {
        if ((input().wasButtonPressed(ButtonId::BTN_A) ||
             RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) &&
            !state.discoveredDevices.empty()) {
            LOG_PERIPHERAL("[ScanScreen] [Btn] Confirm Button Clicked");
            selectMenuItem();
        }

        if ((input().wasButtonPressed(ButtonId::BTN_B) ||
             RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) &&
            !state.discoveredDevices.empty()) {
            LOG_PERIPHERAL("[ScanScreen] [Btn] Next Button Clicked");
            nextMenuItem();
        }

        if (RemoteControlManager::wasButtonPressed(ButtonId::UP) &&
            !state.discoveredDevices.empty()) {
            LOG_PERIPHERAL("[ScanScreen] [Btn] Prev Button Clicked");
            prevMenuItem();
        }
    }
}

void ScanScreen::selectMenuItem() {
    size_t selectedIndex = menuItems.getSelectedIndex();
    isConnecting = true;
    draw();

    if (ScanProcess::connectToDevice(selectedIndex)) {
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Connected));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Connected));
        draw();
        delay(500);  // Show success message briefly
        isConnecting = false;
        MenuSystem::goHome();  // Return to previous screen
    } else {
        isConnecting = false;
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Failed));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Failed));
        ScanProcess::clearDevices();
        draw();
        delay(1000);  // Show error message

        if (!ScanProcess::startScan(5)) {
            setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Failed));
        }
        draw();
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