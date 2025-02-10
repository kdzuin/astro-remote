#include "scan_screen.h"
#include "../transport/encoder_device.h"

ScanScreen::ScanScreen()
    : BaseScreen<ScanMenuItem>("Scan"), lastScanning(false), isConnecting(false)
{
    M5.Display.setTextDatum(middle_center);
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;
    M5.Display.drawString("Wait...", centerX, centerY);

    setStatusText("Scanning...");
    setStatusBgColor(M5.Display.color565(128, 128, 0)); // Yellow for scanning

    // Start scanning immediately when screen is created
    if (!BLEDeviceManager::startScan(5)) // 5-second scan
    {
        setStatusText("Scan failed!");
        setStatusBgColor(M5.Display.color565(200, 0, 0));
    }

    updateMenuItems();
}

void ScanScreen::updateMenuItems()
{
    menuItems.clear();

    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
    for (const auto &deviceInfo : discoveredDevices)
    {
        std::string displayName = deviceInfo.getName();
        if (displayName.empty())
        {
            displayName = deviceInfo.getAddress();
        }
        menuItems.addItem(ScanMenuItem::Device, displayName);
    }
}

void ScanScreen::drawContent()
{
    M5.Display.fillScreen(BLACK);

    if (isConnecting)
    {
        M5.Display.setTextDatum(middle_center);
        int centerX = M5.Display.width() / 2;
        int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;
        M5.Display.drawString("Wait...", centerX, centerY);

        setStatusText("Connecting...");
        setStatusBgColor(M5.Display.color565(128, 128, 0)); // Yellow for connecting
    }
    else
    {
        const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
        if (!discoveredDevices.empty())
        {
            // Reset text alignment for menu drawing
            menuItems.draw();

            setStatusText("Select camera");
            setStatusBgColor(M5.Display.color565(0, 0, 100)); // Blue for selection
        }
        else
        {
            M5.Display.setTextDatum(middle_center);
            int centerX = M5.Display.width() / 2;
            int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

            M5.Display.drawString("Not found", centerX, centerY - 10);
            M5.Display.drawString("Restarting...", centerX, centerY + 10);

            setStatusText("No devices");
            setStatusBgColor(M5.Display.color565(200, 0, 0)); // Red for no devices

            // Restart scan after a brief delay if not already scanning
            if (!BLEDeviceManager::isScanning())
            {
                if (!BLEDeviceManager::startScan(5))
                {
                    setStatusText("Scan failed!");
                }
            }
        }
    }
}

void ScanScreen::update()
{
    EncoderDevice::update();
    int16_t delta = EncoderDevice::getDelta();

    // Check if scanning state changed
    if (lastScanning != BLEDeviceManager::isScanning())
    {
        lastScanning = BLEDeviceManager::isScanning();
        if (!lastScanning) // Scan just finished
        {
            updateMenuItems(); // Update the menu with any found devices
        }
        draw();
        return;
    }

    if (!BLEDeviceManager::isScanning() && !isConnecting)
    {
        if ((M5.BtnA.wasClicked() || EncoderDevice::wasClicked()) && !BLEDeviceManager::getDiscoveredDevices().empty())
        {
            selectMenuItem();
        }

        if ((delta > 0 || M5.BtnB.wasClicked()) && !BLEDeviceManager::getDiscoveredDevices().empty())
        {
            nextMenuItem();
        }
    }
}

void ScanScreen::selectMenuItem()
{
    EncoderDevice::indicateClick();

    size_t selectedIndex = menuItems.getSelectedIndex();
    if (selectedIndex < BLEDeviceManager::getDiscoveredDevices().size())
    {
        const auto &selectedDev = BLEDeviceManager::getDiscoveredDevices()[selectedIndex];
        isConnecting = true;
        draw();

        if (BLEDeviceManager::connectToCamera(selectedDev.device))
        {
            setStatusText("Connected!");
            setStatusBgColor(M5.Display.color565(0, 200, 0)); // Green for success
            draw();
            delay(500); // Show success message briefly
            isConnecting = false;
            MenuSystem::goHome(); // Return to previous screen
        }
        else
        {
            isConnecting = false;
            setStatusText("Connection failed!");
            setStatusBgColor(M5.Display.color565(200, 0, 0)); // Red for failure
            BLEDeviceManager::clearDiscoveredDevices();
            draw();
            delay(1000); // Show error message

            if (!BLEDeviceManager::startScan(5))
            {
                setStatusText("Scan failed!");
            }
            draw();
        }
    }
}

void ScanScreen::nextMenuItem()
{
    menuItems.selectNext();
    EncoderDevice::indicateNext();
    draw();
}

void ScanScreen::prevMenuItem()
{
    // menuItems.selectPrev();
    EncoderDevice::indicatePrev();
    draw();
}