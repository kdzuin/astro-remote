#include "menu_system.h"
#include "camera_commands.h"

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
    BLEDeviceManager::init();
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

void MenuSystem::drawNavigation(const char *leftBtn, const char *rightBtn)
{
    int displayHeight = M5.Display.height();
    int displayWidth = M5.Display.width();
    int fontHeight = M5.Display.fontHeight();

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
    if (needsRedraw)
    {
        bool fullRedraw = needsFullRedraw; // Only fill screen and redraw everything if needed
        if (fullRedraw)
        {
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
                std::string desc = "Addr: " + deviceInfo.getAddress();
                scanList.addItem(deviceInfo.getName(), desc);
            }

            if (!discoveredDevices.empty())
            {
                scanList.draw();
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

void MenuSystem::drawControlMenu()
{
    if (needsRedraw)
    {
        bool fullRedraw = needsFullRedraw;
        if (fullRedraw)
        {
            M5.Display.fillScreen(BLACK);
            M5.Display.setTextSize(1.25, 1.5);
        }

        // Store current selection before clearing
        int prevIndex = controlList.getSelectedIndex();

        // Update menu items if doing full redraw
        if (fullRedraw)
        {
            controlList.clear();
            controlList.setTitle("Camera Control");

            // Basic controls
            controlList.addItem("Take Photo");
            if (CameraCommands::isRecording()) {
                controlList.addItem("Stop Recording");
            } else {
                controlList.addItem("Start Recording");
            }

            // Focus controls
            controlList.addItem("--- Focus ---");
            controlList.addItem("AF Lock", "", false); // Header
            controlList.addItem("Focus Lock");
            controlList.addItem("Focus Near");
            controlList.addItem("Focus Far");

            // Zoom controls
            controlList.addItem("--- Zoom ---");
            controlList.addItem("Zoom In");
            controlList.addItem("Zoom Out");

            // Custom buttons
            controlList.addItem("--- Custom ---");
            controlList.addItem("C1 Button");

            // Restore selection if valid
            if (prevIndex < controlList.size())
            {
                controlList.setSelectedIndex(prevIndex);
            }
        }

        // Draw the list in the available space
        controlList.draw(4, fullRedraw);

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

void MenuSystem::drawSettingsMenu()
{
    if (needsRedraw)
    {
        bool fullRedraw = needsFullRedraw; // Only fill screen and redraw everything if needed
        if (fullRedraw)
        {
            M5.Display.fillScreen(BLACK);
            M5.Display.setTextSize(1.25, 1.5);
        }

        // Store current selection before clearing
        int prevIndex = settingsList.getSelectedIndex();

        // Update menu items if doing full redraw
        if (fullRedraw)
        {
            settingsList.clear();
            settingsList.setTitle("Settings");

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

            settingsList.addItem(("Camera " + status).c_str());
            settingsList.addItem("Connect New Camera");
            if (BLEDeviceManager::isPaired())
            {
                settingsList.addItem("Forget Camera");
            }

            // Restore selection if valid
            if (prevIndex < settingsList.size())
            {
                settingsList.setSelectedIndex(prevIndex);
            }
        }

        // Draw the list in the available space
        settingsList.draw(4, fullRedraw);

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

void MenuSystem::handleMainMenu()
{
    if (M5.BtnA.wasClicked())
    {
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
        mainList.next();
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
        needsFullRedraw = true;
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
        needsRedraw = true; // Only need partial redraw for selection change
    }
    else if (M5.BtnPWR.wasClicked())
    {
        currentMenu = 0;
        needsRedraw = true;
        needsFullRedraw = true;
    }
}

void MenuSystem::handleControlMenu()
{
    if (M5.BtnA.wasClicked())
    {
        const auto &selectedItem = controlList.getSelectedItem();
        if (selectedItem.label == "Take Photo")
        {
            if (BLEDeviceManager::isConnected())
            {
                M5.Display.fillScreen(BLACK);
                M5.Display.setCursor(0, 0);
                M5.Display.println("Taking photo...");

                // Execute full shutter sequence
                if (CameraCommands::shutterPress())
                {
                    M5.Display.println("Photo taken!");
                }
                else
                {
                    M5.Display.println("Failed to take photo");
                }
                delay(1000);
                needsRedraw = true;
                needsFullRedraw = true;
            }
        }
        else if (selectedItem.label == "Start Recording")
        {
            if (BLEDeviceManager::isConnected())
            {
                if (CameraCommands::recordStart())
                {
                    needsRedraw = true;
                    needsFullRedraw = true;
                }
            }
        }
        else if (selectedItem.label == "Stop Recording")
        {
            if (BLEDeviceManager::isConnected())
            {
                if (CameraCommands::recordStop())
                {
                    needsRedraw = true;
                    needsFullRedraw = true;
                }
            }
        }
        else if (selectedItem.label == "Focus Lock")
        {
            if (BLEDeviceManager::isConnected())
            {
                M5.Display.fillScreen(BLACK);
                M5.Display.setCursor(0, 0);
                M5.Display.println("Acquiring focus...");

                if (CameraCommands::afPress())
                {
                    // Wait for focus confirmation
                    unsigned long startTime = millis();
                    while (millis() - startTime < 3000) // 3 second timeout
                    {
                        if (CameraCommands::isFocusAcquired())
                        {
                            M5.Display.println("Focus acquired!");
                            break;
                        }
                        delay(50);
                    }

                    if (!CameraCommands::isFocusAcquired())
                    {
                        M5.Display.println("Focus failed!");
                    }

                    CameraCommands::afRelease();
                }
                delay(1000);
                needsRedraw = true;
                needsFullRedraw = true;
            }
        }
        else if (selectedItem.label == "Focus Near" || selectedItem.label == "Focus Far")
        {
            if (BLEDeviceManager::isConnected())
            {
                bool isNear = selectedItem.label == "Focus Near";
                M5.Display.fillScreen(BLACK);
                M5.Display.setCursor(0, 0);
                M5.Display.printf("Focus %s...\n", isNear ? "near" : "far");

                // Press focus button
                if (isNear)
                {
                    CameraCommands::focusInPress();
                    delay(500); // Hold for half second
                    CameraCommands::focusInRelease();
                }
                else
                {
                    CameraCommands::focusOutPress();
                    delay(500); // Hold for half second
                    CameraCommands::focusOutRelease();
                }

                delay(500);
                needsRedraw = true;
                needsFullRedraw = true;
            }
        }
        else if (selectedItem.label == "Zoom In" || selectedItem.label == "Zoom Out")
        {
            if (BLEDeviceManager::isConnected())
            {
                bool isIn = selectedItem.label == "Zoom In";
                M5.Display.fillScreen(BLACK);
                M5.Display.setCursor(0, 0);
                M5.Display.printf("Zoom %s...\n", isIn ? "in" : "out");

                // Press zoom button
                if (isIn)
                {
                    CameraCommands::zoomTelePress();
                    delay(500); // Hold for half second
                    CameraCommands::zoomTeleRelease();
                }
                else
                {
                    CameraCommands::zoomWidePress();
                    delay(500); // Hold for half second
                    CameraCommands::zoomWideRelease();
                }

                delay(500);
                needsRedraw = true;
                needsFullRedraw = true;
            }
        }
        else if (selectedItem.label == "C1 Button")
        {
            if (BLEDeviceManager::isConnected())
            {
                CameraCommands::c1Press();
                delay(100);
                CameraCommands::c1Release();
                needsRedraw = true;
            }
        }
    }
    else if (M5.BtnB.wasClicked())
    {
        controlList.next();
        needsRedraw = true;
    }
    else if (M5.BtnC.wasClicked())
    {
        currentMenu = 0; // Go back to main menu
        needsRedraw = true;
        needsFullRedraw = true;
    }
}