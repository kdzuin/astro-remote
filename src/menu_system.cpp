#include "menu_system.h"
#include "camera_control.h"

int MenuSystem::menuState = 0;
int MenuSystem::selectedDevice = 0;
int MenuSystem::scanScrollPosition = 0;
unsigned long MenuSystem::scanStartTime = 0;
const unsigned long MenuSystem::DISPLAY_UPDATE_INTERVAL = 1000;
bool MenuSystem::forceRedraw = true;

// Tilt parameters
const float MenuSystem::TILT_THRESHOLD = 50.0f;
float MenuSystem::lastPitch = 0.0f;
unsigned long MenuSystem::lastTiltTime = 0;
const unsigned long MenuSystem::TILT_COOLDOWN = 300; // Reduced cooldown for better responsiveness

void MenuSystem::init()
{
    M5.Display.setRotation(1);
    M5.Display.setTextSize(1.25, 1.5);
    M5.Display.setBaseColor(BLACK);
    M5.Display.fillScreen(BLACK);
    M5.Display.println("Sony Camera Remote");
    delay(1000);
    menuState = 0;
    selectedDevice = 0;
    scanScrollPosition = 0;
    forceRedraw = true;
    lastPitch = 0.0f;
    lastTiltTime = 0;

    // Initialize IMU
    M5.Imu.begin();
}

void MenuSystem::update()
{
    BLEDeviceManager::update();
    handleTiltScroll();

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
    static bool lastScanning = !BLEDeviceManager::isScanning();
    static int lastSelectedDevice = -1;
    static size_t lastDeviceCount = 0;
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
            M5.Display.printf("Scanning...\n");
            M5.Display.printf("Found: %d\n\n", discoveredDevices.size());
        }
        else
        {
            M5.Display.println("Select a device:\n");

            // Show all devices with pagination
            int itemsPerPage = 5;
            int currentPage = selectedDevice / itemsPerPage;
            int startIdx = currentPage * itemsPerPage;
            int endIdx = min(startIdx + itemsPerPage, (int)discoveredDevices.size());

            for (int i = startIdx; i < endIdx; i++)
            {
                M5.Display.print(i == selectedDevice ? "> " : "  ");
                const auto &device = discoveredDevices[i];
                if (device.device->haveName())
                {
                    M5.Display.printf("%s\n", device.device->getName().c_str());
                }
                else
                {
                    M5.Display.printf("%s\n", device.device->getAddress().toString().c_str());
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

        // Update state tracking
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

void MenuSystem::handleTiltScroll()
{
    if (menuState != 1 || BLEDeviceManager::isScanning())
    {
        return; // Only handle tilt in scan menu when not scanning
    }

    float accX, accY, accZ;
    if (M5.Imu.getAccel(&accX, &accY, &accZ))
    {
        // Calculate pitch angle from accelerometer data
        // In landscape mode, Y is forward/back tilt
        float pitch = atan2(accY, accZ) * 180.0f / PI;

        // Check if enough time has passed since last tilt action
        if (millis() - lastTiltTime >= TILT_COOLDOWN)
        {
            // Only trigger if we're coming from a neutral position (within ±30°)
            bool wasNeutral = abs(lastPitch) < 30.0f;

            // Forward tilt
            if (wasNeutral && pitch < -TILT_THRESHOLD)
            {
                const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
                if (!discoveredDevices.empty())
                {
                    selectedDevice = (selectedDevice > 0) ? selectedDevice - 1 : discoveredDevices.size() - 1;
                    lastTiltTime = millis();
                }
            }
            // Backward tilt
            else if (wasNeutral && pitch > TILT_THRESHOLD)
            {
                const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
                if (!discoveredDevices.empty())
                {
                    selectedDevice = (selectedDevice + 1) % discoveredDevices.size();
                    lastTiltTime = millis();
                }
            }
        }

        lastPitch = pitch;
    }
}
