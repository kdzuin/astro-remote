#include "menu_system.h"

int MenuSystem::currentMenu = 0;
int MenuSystem::selectedDevice = -1;
bool MenuSystem::needsRedraw = true;

void MenuSystem::init()
{
    currentMenu = 0;
    selectedDevice = -1;
    needsRedraw = true;
    drawMenu();
}

void MenuSystem::update()
{
    M5.update(); // Make sure this is called first
    BLEDeviceManager::update();

    if (needsRedraw)
    {
        drawMenu();
        needsRedraw = false;
    }

    switch (currentMenu)
    {
    case 0:
        handleMainMenu();
        break;
    case 1:
        handleScanMenu();
        break;
    case 2:
        handleControlMenu();
        break;
    case 3:
        handleSettingsMenu();
        break;
    }
}

void MenuSystem::drawMenu()
{
    if (!needsRedraw)
    {
        return;
    }

    switch (currentMenu)
    {
    case 0:
        drawMainMenu();
        break;
    case 1:
        drawScanMenu();
        break;
    case 2:
        drawControlMenu();
        break;
    case 3:
        drawSettingsMenu();
        break;
    }
}

void MenuSystem::drawMainMenu()
{
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);

    M5.Display.println("Sony Camera Remote");
    M5.Display.println();

    if (BLEDeviceManager::isConnected())
    {
        M5.Display.println("Camera Connected");
        M5.Display.println();
        M5.Display.println("A: Control Camera");
    }
    else if (BLEDeviceManager::isPaired())
    {
        M5.Display.println("Last Camera Available");
        M5.Display.println();
        M5.Display.println("A: Connect Last Camera");
    }

    M5.Display.println("B: Settings");
    M5.Display.println();

    needsRedraw = false;
}

void MenuSystem::handleMainMenu()
{
    if (M5.BtnA.wasClicked())
    {
        Serial.println("Button A clicked in main menu");

        // If already connected, go straight to camera control
        if (BLEDeviceManager::isConnected())
        {
            Serial.println("Already connected, going to camera control");
            currentMenu = 2; // Camera control menu
            needsRedraw = true;
            return;
        }

        // If we have a paired camera, try to connect to it
        if (BLEDeviceManager::isPaired())
        {
            Serial.println("Attempting to connect to last camera");
            if (BLEDeviceManager::connectToSavedDevice())
            {
                Serial.println("Connected to last camera successfully");
                currentMenu = 2; // Go to camera control
            }
            needsRedraw = true;
            return;
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        Serial.println("Button B clicked in main menu");
        currentMenu = 3; // Switch to settings menu
        needsRedraw = true;
    }
}

void MenuSystem::drawScanMenu()
{
    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
    static bool lastScanning = false;
    static int lastSelectedDevice = -1;
    static bool isConnecting = false; // Add state for connection attempt

    if (needsRedraw || lastScanning != BLEDeviceManager::isScanning() ||
        (!BLEDeviceManager::isScanning() && lastSelectedDevice != selectedDevice))
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        if (BLEDeviceManager::isScanning())
        {
            M5.Display.println("Scanning for Sony cameras...");
            M5.Display.println("Please wait...");
            M5.Display.println();
        }
        else if (isConnecting)
        {
            M5.Display.println("Connecting to camera...");
            M5.Display.println("Please wait...");
            M5.Display.println();
        }
        else
        {
            M5.Display.println("Found devices:");
            M5.Display.println();
        }

        int deviceNum = 0;
        for (const auto &deviceInfo : discoveredDevices)
        {
            if (deviceNum == selectedDevice)
            {
                M5.Display.setTextColor(BLACK, WHITE);
            }
            else
            {
                M5.Display.setTextColor(WHITE, BLACK);
            }

            M5.Display.printf("%s\n", deviceInfo.getName().c_str());
            M5.Display.printf("   RSSI: %d\n", deviceInfo.getRSSI());
            M5.Display.printf("   Addr: %s\n", deviceInfo.getAddress().c_str());
            M5.Display.println();

            deviceNum++;
        }

        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.println();

        if (!BLEDeviceManager::isScanning() && !isConnecting)
        {
            if (!discoveredDevices.empty())
            {
                M5.Display.println("A:Connect  B:Next");
                M5.Display.println("PWR: Back");
            }
            else
            {
                M5.Display.println("PWR: Back");
            }
        }

        needsRedraw = false;
        lastScanning = BLEDeviceManager::isScanning();
        lastSelectedDevice = selectedDevice;
    }
}

void MenuSystem::handleScanMenu()
{
    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
    static bool isConnecting = false;

    // Handle back button regardless of scanning state
    if (M5.BtnPWR.wasClicked())
    {
        Serial.println("Power button clicked in scan menu, going back");
        if (BLEDeviceManager::isScanning())
        {
            BLEDeviceManager::stopScan();
        }
        currentMenu = 0; // Go back to main menu
        needsRedraw = true;
        return;
    }

    if (!BLEDeviceManager::isScanning() && !isConnecting)
    {
        // If no devices found after scan completes, start a new scan
        if (discoveredDevices.empty())
        {
            Serial.println("No devices found, starting new scan");
            BLEDeviceManager::startScan(5); // Start a new 5-second scan
            needsRedraw = true;
            return;
        }

        if (M5.BtnA.wasClicked())
        {
            if (!discoveredDevices.empty() && selectedDevice >= 0 && selectedDevice < discoveredDevices.size())
            {
                const auto &selectedDev = discoveredDevices[selectedDevice];
                Serial.printf("Attempting to connect to: %s\n", selectedDev.getName().c_str());

                isConnecting = true;
                needsRedraw = true;

                // Try to connect to the selected device
                if (BLEDeviceManager::connectToCamera(selectedDev.device))
                {
                    Serial.println("Connected successfully!");
                    isConnecting = false;
                    currentMenu = 2; // Switch to device menu
                    needsRedraw = true;
                }
                else
                {
                    Serial.println("Connection failed!");
                    isConnecting = false;
                    // Start a new scan to try again
                    BLEDeviceManager::clearDiscoveredDevices();
                    BLEDeviceManager::startScan(5);
                    selectedDevice = -1;
                    needsRedraw = true;
                }
            }
        }
        else if (M5.BtnB.wasClicked())
        {
            if (!discoveredDevices.empty())
            {
                selectedDevice = (selectedDevice + 1) % discoveredDevices.size();
                needsRedraw = true;
            }
        }
    }
}

void MenuSystem::drawDeviceMenu()
{
    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();

    if (BLEDeviceManager::isScanning())
    {
        return;
    }

    if (selectedDevice >= 0 && selectedDevice < discoveredDevices.size())
    {
        const auto &deviceInfo = discoveredDevices[selectedDevice];

        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        M5.Display.println("Device Details:");
        M5.Display.println();
        M5.Display.printf("Name: %s\n", deviceInfo.getName().c_str());
        M5.Display.printf("Address: %s\n", deviceInfo.getAddress().c_str());
        M5.Display.printf("RSSI: %d\n", deviceInfo.getRSSI());
        M5.Display.println();

        if (!BLEDeviceManager::isConnected())
        {
            M5.Display.println("A: Connect");
        }
        else
        {
            M5.Display.println("A: Disconnect");
        }
        M5.Display.println("PWR: Back");
    }
}

void MenuSystem::handleDeviceMenu()
{
    if (M5.BtnA.wasClicked())
    {
        if (BLEDeviceManager::isConnected())
        {
            BLEDeviceManager::disconnectCamera();
            needsRedraw = true;
        }
        else
        {
            const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
            if (selectedDevice >= 0 && selectedDevice < discoveredDevices.size())
            {
                if (BLEDeviceManager::connectToCamera(discoveredDevices[selectedDevice].device))
                {
                    needsRedraw = true;
                }
            }
        }
    }
    else if (M5.BtnPWR.wasClicked())
    {
        currentMenu = 0;
        needsRedraw = true;
    }
}

void MenuSystem::drawControlMenu()
{
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Camera Control\n");

    if (!BLEDeviceManager::isConnected())
    {
        M5.Display.println("Camera disconnected!");
        delay(1000);
        currentMenu = 0;
        needsRedraw = true;
        return;
    }

    M5.Display.println("A: Shutter");
    M5.Display.println("B: Record");
    M5.Display.println("PWR: Back");
}

void MenuSystem::handleControlMenu()
{
    if (!BLEDeviceManager::isConnected())
    {
        currentMenu = 0;
        needsRedraw = true;
        return;
    }

    if (M5.BtnPWR.wasClicked())
    {
        currentMenu = 0;
        needsRedraw = true;
    }
}

void MenuSystem::drawSettingsMenu()
{
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);

    M5.Display.println("Settings\n");

    // Show connection status
    if (BLEDeviceManager::isConnected())
    {
        M5.Display.println("Camera: Connected");
    }
    else if (BLEDeviceManager::isPaired())
    {
        M5.Display.println("Camera: Paired");
    }
    else
    {
        M5.Display.println("Camera: Not Paired");
    }
    M5.Display.println();

    // Show menu options
    M5.Display.println("A: Connect New Camera");

    if (BLEDeviceManager::isPaired())
    {
        M5.Display.println("B: Forget Camera");
    }

    M5.Display.println("\nPWR: Back");

    needsRedraw = false;
}

void MenuSystem::handleSettingsMenu()
{
    if (M5.BtnA.wasClicked())
    {
        // Start scanning for new cameras
        currentMenu = 1;                            // Switch to scan menu
        selectedDevice = -1;                        // Reset device selection
        BLEDeviceManager::clearDiscoveredDevices(); // Clear old scan results
        BLEDeviceManager::startScan(5);             // Start a 5-second scan
        needsRedraw = true;
    }
    else if (M5.BtnB.wasClicked() && BLEDeviceManager::isPaired())
    {
        // Forget the paired camera
        if (BLEDeviceManager::isConnected())
        {
            BLEDeviceManager::disconnectCamera();
        }
        BLEDeviceManager::unpairCamera();
        needsRedraw = true;
    }
    else if (M5.BtnPWR.wasClicked())
    {
        currentMenu = 0; // Go back to main menu
        needsRedraw = true;
    }
}

void MenuSystem::goBack()
{
    // If we're connected and not in the control menu, disconnect
    if (BLEDeviceManager::isConnected() && currentMenu != 2)
    {
        Serial.println("Disconnecting on menu exit");
        BLEDeviceManager::disconnectCamera();
        delay(200); // Give some time for cleanup
    }

    currentMenu = 0; // Return to main menu
    needsRedraw = true;
}
