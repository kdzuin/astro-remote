#include "menu_system.h"
#include "camera_control.h"

int MenuSystem::menuState = 0;
int MenuSystem::selectedDevice = 0;
bool MenuSystem::needsRedraw = true;

void MenuSystem::init()
{
    menuState = 0;
    selectedDevice = 0;
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

    switch (menuState)
    {
    case 0:
        handleMainMenu();
        break;
    case 1:
        handleDeviceMenu();
        break;
    case 2:
        handleControlMenu();
        break;
    }
}

void MenuSystem::drawMenu()
{
    switch (menuState)
    {
    case 0:
        drawMainMenu();
        break;
    case 1:
        drawDeviceMenu();
        break;
    case 2:
        drawControlMenu();
        break;
    }
}

void MenuSystem::drawMainMenu()
{
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Sony Camera Remote\n");

    if (BLEDeviceManager::isPaired())
    {
        if (BLEDeviceManager::isConnected())
        {
            M5.Display.println("Camera connected");
            M5.Display.println("A: Control camera");
            M5.Display.println("B: Disconnect");
        }
        else
        {
            M5.Display.println("Camera paired");
            M5.Display.println("A: Connect");
            M5.Display.println("B: Forget camera");
        }
    }
    else
    {
        M5.Display.println("No camera paired");
        M5.Display.println("A: Scan for cameras");
    }
}

void MenuSystem::handleMainMenu()
{
    if (M5.BtnA.wasClicked())
    {
        Serial.println("Button A clicked in main menu"); // Debug print

        if (BLEDeviceManager::isPaired())
        {
            if (BLEDeviceManager::isConnected())
            {
                menuState = 2; // Go to camera control menu
                needsRedraw = true;
            }
            else
            {
                // Try to connect to paired camera
                std::string address = BLEDeviceManager::getPairedDeviceAddress();
                if (!address.empty())
                {
                    M5.Display.fillScreen(BLACK);
                    M5.Display.println("Connecting...");
                    needsRedraw = true;
                }
            }
        }
        else
        {
            Serial.println("Starting scan..."); // Debug print
            menuState = 1;                      // Go to device menu
            BLEDeviceManager::startScan(5);     // 5 second scan
            needsRedraw = true;
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        Serial.println("Button B clicked in main menu"); // Debug print

        if (BLEDeviceManager::isPaired())
        {
            if (BLEDeviceManager::isConnected())
            {
                BLEDeviceManager::disconnectCamera();
            }
            else
            {
                BLEDeviceManager::unpairCamera();
            }
            needsRedraw = true;
        }
    }
}

void MenuSystem::goBack()
{
    if (BLEDeviceManager::isScanning())
    {
        BLEDeviceManager::stopScan();
    }
    BLEDeviceManager::clearDiscoveredDevices();
    selectedDevice = 0;
    menuState = 0;
    needsRedraw = true;
    drawMainMenu();
}

void MenuSystem::drawScanMenu()
{
    static bool lastScanning = false;
    static int lastDeviceCount = 0;
    static int lastSelectedDevice = -1;
    const int itemsPerPage = 4;

    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();

    // Check if we need to redraw
    bool needsRedraw = MenuSystem::needsRedraw ||
                       lastScanning != BLEDeviceManager::isScanning() ||
                       lastDeviceCount != discoveredDevices.size() ||
                       (!BLEDeviceManager::isScanning() && lastSelectedDevice != selectedDevice);

    if (needsRedraw)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        if (BLEDeviceManager::isScanning())
        {
            M5.Display.println("Scanning for Sony cameras...\n");
            if (!discoveredDevices.empty())
            {
                M5.Display.printf("Found %d camera%s\n\n",
                                  discoveredDevices.size(),
                                  discoveredDevices.size() == 1 ? "" : "s");
            }
        }
        else if (discoveredDevices.empty())
        {
            M5.Display.println("No cameras found\n");
            M5.Display.println("\nB:Refresh  PWR:Back");
        }
        else
        {
            M5.Display.printf("Found %d camera%s\n\n",
                              discoveredDevices.size(),
                              discoveredDevices.size() == 1 ? "" : "s");

            int currentPage = selectedDevice / itemsPerPage;
            int startIdx = currentPage * itemsPerPage;
            int endIdx = min(startIdx + itemsPerPage, (int)discoveredDevices.size());

            for (int i = startIdx; i < endIdx; i++)
            {
                const auto &deviceInfo = discoveredDevices[i];
                deviceInfo.updateName();

                M5.Display.print(i == selectedDevice ? "> " : "  ");
                if (deviceInfo.device->haveName())
                {
                    M5.Display.printf("%s\n", deviceInfo.name.c_str());
                }
                else
                {
                    M5.Display.printf("%s\n", deviceInfo.device->getAddress().toString().c_str());
                }

                // Show camera info if this is the selected device
                if (i == selectedDevice)
                {
                    M5.Display.printf("   Model: %04X, Ver: %d\n",
                                      deviceInfo.cameraInfo.modelCode,
                                      deviceInfo.cameraInfo.protocolVersion);
                }
            }

            if (discoveredDevices.size() > itemsPerPage)
            {
                M5.Display.printf("\nPage %d/%d",
                                  currentPage + 1,
                                  (discoveredDevices.size() + itemsPerPage - 1) / itemsPerPage);
            }

            M5.Display.println("\nA:Select  B:Next  PWR:Back");
        }

        lastScanning = BLEDeviceManager::isScanning();
        lastSelectedDevice = selectedDevice;
        lastDeviceCount = discoveredDevices.size();
        MenuSystem::needsRedraw = false;
    }
}

void MenuSystem::drawDeviceMenu()
{
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(0, 0);

    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();

    if (BLEDeviceManager::isScanning())
    {
        M5.Display.println("Scanning for cameras...");
        if (!discoveredDevices.empty())
        {
            M5.Display.printf("\nFound %d camera%s\n",
                              discoveredDevices.size(),
                              discoveredDevices.size() == 1 ? "" : "s");
        }
    }
    else if (discoveredDevices.empty())
    {
        M5.Display.println("No cameras found\n");
        M5.Display.println("PWR: Back");
        M5.Display.println("B: Scan again");
    }
    else
    {
        M5.Display.printf("Available Cameras (%d):\n\n",
                          discoveredDevices.size());

        // Show all devices with pagination
        int itemsPerPage = 4;
        int currentPage = selectedDevice / itemsPerPage;
        int startIdx = currentPage * itemsPerPage;
        int endIdx = min(startIdx + itemsPerPage, (int)discoveredDevices.size());

        for (int i = startIdx; i < endIdx; i++)
        {
            const auto &deviceInfo = discoveredDevices[i];
            M5.Display.print(i == selectedDevice ? "> " : "  ");

            if (deviceInfo.device->haveName())
            {
                M5.Display.println(deviceInfo.device->getName().c_str());
            }
            else
            {
                M5.Display.println(deviceInfo.device->getAddress().toString().c_str());
            }

            // Show additional info for selected device
            if (i == selectedDevice)
            {
                M5.Display.printf("   Model: %04X\n", deviceInfo.cameraInfo.modelCode);
                M5.Display.printf("   Protocol: v%d\n", deviceInfo.cameraInfo.protocolVersion);
            }
        }

        if (discoveredDevices.size() > itemsPerPage)
        {
            M5.Display.printf("\nPage %d/%d",
                              currentPage + 1,
                              (discoveredDevices.size() + itemsPerPage - 1) / itemsPerPage);
        }

        M5.Display.println("\nA: Pair  B: Next  PWR: Back");
    }
}

void MenuSystem::handleScanMenu()
{
    if (!BLEDeviceManager::isScanning())
    {
        if (M5.BtnA.wasClicked())
        {
            Serial.println("BtnA button pressed");
            const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
            if (!discoveredDevices.empty())
            {
                Serial.println("Device selected from scan menu");
                auto &selectedDev = discoveredDevices[selectedDevice];
                selectedDev.updateName();

                CameraControl::setPairedDeviceAddress(selectedDev.device->getAddress().toString().c_str());
                Preferences prefs;
                prefs.begin("camera", false);
                prefs.putString("addr", CameraControl::getPairedDeviceAddress());
                if (selectedDev.device->haveName())
                {
                    prefs.putString("name", selectedDev.device->getName().c_str());
                }
                else
                {
                    prefs.remove("name");
                }
                prefs.end();

                BLEDeviceManager::clearDiscoveredDevices();
                selectedDevice = 0;

                menuState = 0;
                needsRedraw = true;
            }
        }
        else if (M5.BtnPWR.wasClicked())
        {
            Serial.println("Power button pressed");
            BLEDeviceManager::clearDiscoveredDevices();
            selectedDevice = 0;
            menuState = 0;
            needsRedraw = true;
        }
        else if (M5.BtnB.wasClicked())
        {
            const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
            if (!discoveredDevices.empty())
            {
                selectedDevice = (selectedDevice + 1) % discoveredDevices.size();
                needsRedraw = true;
            }
        }
    }
}

void MenuSystem::handleDeviceMenu()
{
    if (M5.BtnA.wasClicked())
    {
        const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
        if (!discoveredDevices.empty() && selectedDevice < discoveredDevices.size())
        {
            Serial.println("Connecting to selected camera...");
            if (BLEDeviceManager::pairCamera(discoveredDevices[selectedDevice].device))
            {
                M5.Display.fillScreen(BLACK);
                M5.Display.println("Camera paired!");
                delay(1000);
                menuState = 0; // Return to main menu
                needsRedraw = true;
            }
            else
            {
                M5.Display.fillScreen(BLACK);
                M5.Display.println("Pairing failed!");
                delay(1000);
                needsRedraw = true;
            }
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        const auto &devices = BLEDeviceManager::getDiscoveredDevices();
        if (!devices.empty())
        {
            selectedDevice = (selectedDevice + 1) % devices.size();
            needsRedraw = true;
        }
    }
    else if (M5.BtnPWR.wasClicked())
    {
        menuState = 0;
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
        menuState = 0;
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
        menuState = 0;
        needsRedraw = true;
        return;
    }

    if (M5.BtnPWR.wasClicked())
    {
        menuState = 0;
        needsRedraw = true;
    }
}
