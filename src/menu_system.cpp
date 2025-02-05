#include "menu_system.h"

int MenuSystem::currentMenu = 0;
bool MenuSystem::needsRedraw = true;
bool MenuSystem::needsFullRedraw = true;
bool MenuSystem::navigationTextChanged = true;
SelectableList MenuSystem::mainList;
SelectableList MenuSystem::scanList;
SelectableList MenuSystem::controlList;
SelectableList MenuSystem::settingsList;

void MenuSystem::init()
{
    M5.begin();
    M5.Display.setRotation(1); // Rotate to landscape
    M5.Display.setTextSize(1.25, 1.5);
    Serial.printf("[MenuSystem] Display size: %dx%d\n", M5.Display.width(), M5.Display.height());
    BLEDeviceManager::init();
}

void MenuSystem::drawNavigation(const char *leftBtn, const char *rightBtn)
{
    int displayHeight = M5.Display.height();
    int displayWidth = M5.Display.width();
    int fontHeight = M5.Display.fontHeight();
    Serial.printf("[MenuSystem] Display size: %dx%d\n", displayWidth, displayHeight);
    Serial.printf("[MenuSystem] Font height: %d\n", fontHeight);

    // Draw navigation bar background
    M5.Display.fillRect(0, displayHeight - NAV_HEIGHT, displayWidth, NAV_HEIGHT, NAV_BG_COLOR);

    // Set text properties for navigation
    M5.Display.setTextColor(NAV_TEXT_COLOR);

    // Draw left button (B)
    if (leftBtn != nullptr)
    {
        M5.Display.setCursor(4, displayHeight - NAV_HEIGHT + ceil((NAV_HEIGHT - fontHeight) / 2));
        M5.Display.print(leftBtn);
    }

    // Draw right button (A)
    if (rightBtn != nullptr)
    {
        int textWidth = M5.Display.textWidth(rightBtn);
        M5.Display.setCursor(displayWidth - textWidth - 10, displayHeight - NAV_HEIGHT + ceil((NAV_HEIGHT - fontHeight) / 2));
        M5.Display.print(rightBtn);
    }

    // Reset text color to white for main content
    M5.Display.setTextColor(WHITE);
}

void MenuSystem::drawMainMenu()
{
    if (needsRedraw)
    {
        Serial.printf("[MenuSystem] Drawing main menu, current selection: %d\n", mainList.getSelectedIndex());

        bool fullRedraw = needsFullRedraw; // Only fill screen and redraw everything if needed
        if (fullRedraw)
        {
            Serial.println("[MenuSystem] Doing full redraw");
            M5.Display.fillScreen(BLACK);
            M5.Display.setTextSize(1.25, 1.5);
        }

        // Store current selection before clearing
        int prevIndex = mainList.getSelectedIndex();

        // Update main menu items if doing full redraw
        if (fullRedraw)
        {
            mainList.clear();
            mainList.setTitle("Astro Remote");
            if (BLEDeviceManager::isConnected())
            {
                mainList.addItem("Control Camera");
            }
            else if (BLEDeviceManager::isPaired())
            {
                mainList.addItem("Connect Last Camera");
            }
            mainList.addItem("Settings");
            mainList.setActionLabel("Select");

            // Restore selection if valid
            if (prevIndex < mainList.size())
            {
                mainList.setSelectedIndex(prevIndex);
            }
        }

        // Draw the list in the available space
        mainList.draw(4, fullRedraw);

        // Draw navigation only if text changed
        if (fullRedraw || navigationTextChanged)
        {
            drawNavigation("Next", "Select");
            navigationTextChanged = false;
        }

        needsRedraw = false;
        needsFullRedraw = false;
    }
}

void MenuSystem::drawScanMenu()
{
    const auto &discoveredDevices = BLEDeviceManager::getDiscoveredDevices();
    static bool lastScanning = false;
    static bool isConnecting = false;

    if (needsRedraw || lastScanning != BLEDeviceManager::isScanning())
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        scanList.clear();
        scanList.setTitle("Available Cameras");

        if (BLEDeviceManager::isScanning())
        {
            M5.Display.println("Scanning for Sony cameras...");
            M5.Display.println("Please wait...");
            drawNavigation(nullptr, nullptr);
        }
        else if (isConnecting)
        {
            M5.Display.println("Connecting to camera...");
            M5.Display.println("Please wait...");
            drawNavigation(nullptr, nullptr);
        }
        else
        {
            // Add discovered devices to the list
            for (const auto &deviceInfo : discoveredDevices)
            {
                std::string desc = "RSSI: " + std::to_string(deviceInfo.getRSSI()) +
                                   "\nAddr: " + deviceInfo.getAddress();
                scanList.addItem(deviceInfo.getName(), desc);
            }

            if (!discoveredDevices.empty())
            {
                scanList.setActionLabel("Connect");
                scanList.draw(M5.Display.getCursorY());
                drawNavigation("Next", "Select");
            }
            else
            {
                M5.Display.println("No devices found");
                drawNavigation(nullptr, nullptr);
            }
        }

        needsRedraw = false;
        lastScanning = BLEDeviceManager::isScanning();
    }
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

void MenuSystem::handleMainMenu()
{
    if (M5.BtnA.wasClicked())
    {
        Serial.printf("[MenuSystem] BtnA clicked, selection: %d\n", mainList.getSelectedIndex());
        const auto &selectedItem = mainList.getSelectedItem();

        if (selectedItem.label == "Control Camera")
        {
            currentMenu = 2; // Camera control menu
            needsFullRedraw = true;
            needsRedraw = true;
        }
        else if (selectedItem.label == "Connect Last Camera")
        {
            Serial.println("Attempting to connect to last camera");
            if (BLEDeviceManager::connectToSavedDevice())
            {
                Serial.println("Connected to last camera successfully");
                currentMenu = 2; // Go to camera control
            }
            needsFullRedraw = true;
            needsRedraw = true;
        }
        else if (selectedItem.label == "Settings")
        {
            currentMenu = 3; // Settings menu
            needsFullRedraw = true;
            needsRedraw = true;
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        Serial.printf("[MenuSystem] BtnB clicked, selection before next: %d\n", mainList.getSelectedIndex());
        mainList.next();
        Serial.printf("[MenuSystem] BtnB clicked, selection after next: %d\n", mainList.getSelectedIndex());
        needsRedraw = true; // Only need partial redraw for selection change
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
            if (!discoveredDevices.empty() && scanList.getSelectedIndex() >= 0 && scanList.getSelectedIndex() < discoveredDevices.size())
            {
                const auto &selectedDev = discoveredDevices[scanList.getSelectedIndex()];
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
                    needsRedraw = true;
                }
            }
        }
        else if (M5.BtnB.wasClicked())
        {
            if (!discoveredDevices.empty())
            {
                scanList.next();
                needsRedraw = true;
            }
        }
    }
}

void MenuSystem::drawControlMenu()
{
    if (needsRedraw)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        controlList.clear();
        controlList.setTitle("Camera Control");

        if (!BLEDeviceManager::isConnected())
        {
            M5.Display.println("Camera disconnected!");
            delay(1000);
            currentMenu = 0;
            needsRedraw = true;
            return;
        }

        controlList.addItem("Take Photo", "Press to capture");
        controlList.addItem("Record Video", "Press to start/stop");
        controlList.setActionLabel("Execute");

        controlList.draw(M5.Display.getCursorY());
        drawNavigation("Next", "Select");

        needsRedraw = false;
    }
}

void MenuSystem::handleControlMenu()
{
    if (M5.BtnA.wasClicked())
    {
        const auto &selectedItem = controlList.getSelectedItem();

        if (selectedItem.label == "Take Photo")
        {
            // TODO: Implement shutter
            Serial.println("Shutter pressed");
        }
        else if (selectedItem.label == "Record Video")
        {
            // TODO: Implement record
            Serial.println("Record pressed");
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        controlList.next();
        needsRedraw = true;
    }
    else if (M5.BtnPWR.wasClicked())
    {
        goBack();
    }
}

void MenuSystem::drawSettingsMenu()
{
    if (needsRedraw)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        settingsList.clear();
        settingsList.setTitle("Settings");

        // Show connection status
        std::string status;
        if (BLEDeviceManager::isConnected())
        {
            status = "Connected";
        }
        else if (BLEDeviceManager::isPaired())
        {
            status = "Paired";
        }
        else
        {
            status = "Not Paired";
        }

        settingsList.addItem(("Camera Status: " + status).c_str());
        settingsList.addItem("Connect New Camera");
        if (BLEDeviceManager::isPaired())
        {
            settingsList.addItem("Forget Camera");
        }

        settingsList.setActionLabel("Select");
        settingsList.draw(M5.Display.getCursorY());
        drawNavigation("Next", "Select");

        needsRedraw = false;
    }
}

void MenuSystem::handleSettingsMenu()
{
    if (M5.BtnA.wasClicked())
    {
        const auto &selectedItem = settingsList.getSelectedItem();

        if (selectedItem.label == "Connect New Camera")
        {
            currentMenu = 1;                            // Switch to scan menu
            scanList.clear();                           // Reset device selection
            BLEDeviceManager::clearDiscoveredDevices(); // Clear old scan results
            BLEDeviceManager::startScan(5);             // Start a 5-second scan
            needsRedraw = true;
        }
        else if (selectedItem.label == "Forget Camera")
        {
            BLEDeviceManager::unpairCamera();
            needsRedraw = true;
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        settingsList.next();
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
    if (BLEDeviceManager::isConnected() && currentMenu != 2)
    {
        BLEDeviceManager::disconnectCamera();
        delay(200); // Give some time for cleanup
    }

    currentMenu = 0; // Return to main menu
    needsRedraw = true;
}
