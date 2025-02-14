#include "screens/scan_screen.h"

#include "transport/remote_control_manager.h"

ScanScreen::ScanScreen()
    : BaseScreen<std::string>("Scan"), lastScanning(false), isConnecting(false) {
    M5.Display.setTextDatum(middle_center);
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;
    M5.Display.drawString("Wait...", centerX, centerY);

    auto state = ScanProcess::getState();
    setStatusText(ScanProcess::getStatusText(state.status));
    setStatusBgColor(ScanProcess::getStatusColor(state.status));

    // Start scanning immediately when screen is created
    if (!ScanProcess::startScan(5)) {  // 5-second scan
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Failed));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Failed));
    }

    updateMenuItems();
    menuItems.setTitle("Scan");
}

void ScanScreen::updateMenuItems() {
    menuItems.clear();

    const auto& state = ScanProcess::getState();
    const auto& devices = state.discoveredDevices;

    if (state.isScanning) {
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::Scanning));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::Scanning));
    } else if (devices.empty()) {
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::NoDevices));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::NoDevices));
    } else {
        setStatusText(ScanProcess::getStatusText(ScanProcess::Status::DevicesFound));
        setStatusBgColor(ScanProcess::getStatusColor(ScanProcess::Status::DevicesFound));
    }

    for (const auto& device : devices) {
        std::string label = device.getName();
        if (label.empty()) {
            label = device.getAddress();
        }
        menuItems.addItem(device.getAddress(), label, true);
    }
}

void ScanScreen::drawContent() {
    M5.Display.fillScreen(colors::get(colors::BLACK));

    auto state = ScanProcess::getState();
    if (isConnecting) {
        state.status = ScanProcess::Status::Connecting;
    }

    if (isConnecting) {
        M5.Display.setTextDatum(middle_center);
        int centerX = M5.Display.width() / 2;
        int centerY = M5.Display.height() / 2;
        M5.Display.drawString("Wait...", centerX, centerY);
    } else if (state.discoveredDevices.empty()) {
        M5.Display.setTextDatum(middle_center);
        int centerX = M5.Display.width() / 2;
        int centerY = M5.Display.height() / 2;

        M5.Display.drawString("Not found", centerX, centerY);

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
        if (!lastScanning) {    // Scan just finished
            updateMenuItems();  // Update the menu with any found devices
        }
        draw();
        return;
    }

    if (!state.isScanning && !isConnecting) {
        if ((M5.BtnA.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) &&
            !state.discoveredDevices.empty()) {
            LOG_PERIPHERAL("[ScanScreen] [Btn] Confirm Button Clicked");
            selectMenuItem();
        }

        if ((M5.BtnB.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) &&
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
    const auto& selectedId = menuItems.getSelectedId();
    isConnecting = true;
    draw();

    const auto& devices = ScanProcess::getState().discoveredDevices;
    auto it = std::find_if(devices.begin(), devices.end(), [&selectedId](const DeviceInfo& dev) {
        return dev.getAddress() == selectedId;
    });

    if (it != devices.end() && ScanProcess::connectToDevice(it->device)) {
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