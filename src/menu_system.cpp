#include "menu_system.h"
#include "camera_control.h"

int MenuSystem::menuState = 0;
int MenuSystem::selectedDevice = 0;
bool MenuSystem::forceRedraw = true;

void MenuSystem::init()
{
    M5.Display.setRotation(1);
    M5.Display.setTextSize(1.25, 1.5);
    M5.Display.fillScreen(BLACK);
    M5.Display.println("Sony Camera Remote");
    delay(1000);
    menuState = 0;
    selectedDevice = 0;
    forceRedraw = true;
}

void MenuSystem::update()
{
    BLEDeviceManager::update();

    switch (menuState)
    {
    case 0:
        handleMainMenu();
        break;
    case 1:
        handleScanMenu();
        break;
    case 2:
        handleDeviceMenu();
        break;
    }
}

void MenuSystem::draw()
{
    switch (menuState)
    {
    case 0:
        drawMainMenu();
        break;
    case 1:
        drawScanMenu();
        break;
    case 2:
        drawDeviceMenu();
        break;
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
    forceRedraw = true;
    drawMainMenu();
}

void MenuSystem::drawMainMenu()
{
    static bool lastConnected = false;
    static bool lastPaired = true; // Initialize to opposite to force first draw
    static String lastDeviceName = "";

    // Get current state
    bool isPaired = !CameraControl::getPairedDeviceAddress().isEmpty();
    bool isConnected = CameraControl::isConnected();

    // Get device name
    String deviceName = "";
    if (isPaired)
    {
        Preferences prefs;
        prefs.begin("camera", true);
        deviceName = prefs.getString("name", "");
        prefs.end();
    }

    // Check if we need to redraw
    bool needsRedraw = forceRedraw ||
                       lastConnected != isConnected ||
                       lastPaired != isPaired ||
                       lastDeviceName != deviceName;

    if (needsRedraw)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Sony Camera Remote\n");

        if (isConnected)
        {
            if (deviceName.length() > 0)
            {
                M5.Display.printf("Connected: %s\n", deviceName.c_str());
            }
            else
            {
                M5.Display.println("Connected");
            }
        }
        else
        {
            M5.Display.println("Disconnected");
            if (isPaired)
            {
                if (deviceName.length() > 0)
                {
                    M5.Display.printf("Last: %s\n", deviceName.c_str());
                }
                M5.Display.println("\nA: Connect");
            }
            M5.Display.println("\nB: Find camera");
        }

        // Update state tracking
        lastConnected = isConnected;
        lastPaired = isPaired;
        lastDeviceName = deviceName;
        forceRedraw = false;
    }
}

void MenuSystem::drawScanMenu()
{
    static bool lastScanning = false;
    static int lastDeviceCount = 0;
    static int lastSelectedDevice = -1;
    const int itemsPerPage = 4;

    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();

    // Check if we need to redraw
    bool needsRedraw = forceRedraw ||
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
        forceRedraw = false;
    }
}

void MenuSystem::drawDeviceMenu()
{
    static int lastSelected = -1;
    static const char *items[] = {
        "Forget device",
        "Back"};
    static const int numItems = sizeof(items) / sizeof(items[0]);

    if (lastSelected != selectedDevice)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Device Menu:\n");

        for (int i = 0; i < numItems; i++)
        {
            M5.Display.print(i == selectedDevice ? "> " : "  ");
            M5.Display.println(items[i]);
        }

        M5.Display.println("\nA:Select  B:Next  PWR:Back");
        lastSelected = selectedDevice;
    }
}

void MenuSystem::handleMainMenu()
{
    if (M5.BtnA.wasClicked())
    {
        Serial.println("Button A pressed in main menu");
    }
    else if (M5.BtnPWR.wasClicked())
    {
        Serial.println("Button PWR pressed in main menu");
    }
    else if (M5.BtnB.wasClicked())
    {
        Serial.println("Button B pressed in main menu");
        if (CameraControl::getPairedDeviceAddress().isEmpty())
        {
            menuState = 1; // Go directly to scanning
            BLEDeviceManager::startScan(3);
        }
        else
        {
            menuState = 2; // Show device menu
        }
    }
}

void MenuSystem::handleScanMenu()
{
    if (!BLEDeviceManager::isScanning())
    {
        if (M5.BtnA.wasClicked())
        {
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
                forceRedraw = true;
            }
        }
        else if (M5.BtnPWR.wasClicked())
        {
            Serial.println("Power button pressed");
            BLEDeviceManager::clearDiscoveredDevices();
            selectedDevice = 0;
            menuState = 0;
            forceRedraw = true;
        }
        else if (M5.BtnB.wasClicked())
        {
            const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
            if (!discoveredDevices.empty())
            {
                selectedDevice = (selectedDevice + 1) % discoveredDevices.size();
            }
        }
    }
}

void MenuSystem::handleDeviceMenu()
{
    if (M5.BtnA.wasClicked())
    {
        switch (selectedDevice)
        {
        case 0: // Forget device
            if (!CameraControl::getPairedDeviceAddress().isEmpty())
            {
                CameraControl::forgetDevice();
                menuState = 0;
                M5.Display.fillScreen(BLACK);
                M5.Display.println("Device forgotten");
                delay(1000);
            }
            break;
        case 1: // Back
            menuState = 0;
            break;
        }
        forceRedraw = true;
    }
    else if (M5.BtnB.wasClicked())
    {
        selectedDevice = (selectedDevice + 1) % 2;
        drawDeviceMenu();
    }
}
