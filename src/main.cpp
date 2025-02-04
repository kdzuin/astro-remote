#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <Preferences.h>

// Sony camera BLE service and characteristic UUIDs
#define SONY_SERVICE_UUID "8000FF00-FF00-FF00-FF00-FF00FF00FF00"
#define SONY_CHAR_UUID "8000FF01-FF00-FF00-FF00-FF00FF00FF00"

// Global variables
BLEClient *pClient = nullptr;
BLERemoteService *pRemoteService = nullptr;
BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
bool deviceConnected = false;
bool shooting = false;
unsigned long exposureStartTime = 0;
unsigned long currentExposureTime = 30000; // Default exposure time (30 seconds)
int exposureCount = 0;                     // Current exposure number
int targetExposures = -1;                  // -1 means infinite
bool scanning = false;
unsigned long scanStartTime = 0;
const unsigned long SCAN_DURATION = 3000; // 10 seconds scan (in milliseconds)
int scanScrollPosition = 0;
const int DEVICES_PER_PAGE = 4;

// Forward declarations
void drawMainMenu();
void drawScanMenu();
void drawDeviceMenu();
void goBack();

// Add this structure to store device info
struct DeviceInfo
{
    BLEAdvertisedDevice *device;
    String name;
    bool nameRequested;

    DeviceInfo(BLEAdvertisedDevice *dev) : device(dev), nameRequested(false)
    {
        name = dev->getAddress().toString().c_str();
    }

    void updateName()
    {
        if (!nameRequested && device->haveName())
        {
            name = device->getName().c_str();
            nameRequested = true;
        }
    }
};

std::vector<DeviceInfo> discoveredDevices;

// Preferences for storing paired device
Preferences preferences;
String pairedDeviceAddress = "";

// Connection status callback
class ConnectionCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        deviceConnected = true;
        Serial.println("Connected!");
    }

    void onDisconnect(BLEClient *pclient)
    {
        deviceConnected = false;
        Serial.println("Disconnected!");
    }
};

// Update the scan callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("Device found: ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());

        // Add device to the list with just the address
        discoveredDevices.push_back(DeviceInfo(new BLEAdvertisedDevice(advertisedDevice)));

        // Redraw the entire menu to prevent overlapping
        drawScanMenu();
    }
};

int menuState = 0;      // 0: main menu, 1: scanning menu, 2: device menu
int selectedDevice = 0; // For device selection menu

struct DisplayState
{
    bool connected;
    bool isShooting;
    int currentShot;
    int targetShots;
    unsigned long exposureTime;
    unsigned long currentTime;
    String menuText;
} lastDisplayState;

void startScanning()
{
    Serial.println("Starting scan...");
    scanning = true;
    scanStartTime = millis();
    discoveredDevices.clear();
    selectedDevice = 0;
    scanScrollPosition = 0;

    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->clearResults(); // Clear any previous results
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false);
    pBLEScan->start(SCAN_DURATION / 1000, false); // Scan continuously until we stop it

    drawScanMenu();
}

void stopScanning()
{
    Serial.println("Stopping scan...");
    scanning = false;
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->stop();
    pBLEScan->clearResults();
    selectedDevice = 0; // Reset selection to first item
    drawScanMenu();
}

void connectToCamera()
{
    if (pairedDeviceAddress.isEmpty())
    {
        Serial.println("No paired device");
        return;
    }

    if (pClient == nullptr)
    {
        pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(new ConnectionCallback());
    }

    Serial.printf("Connecting to %s\n", pairedDeviceAddress.c_str());
    // Connect to server using stored address
    pClient->connect(BLEAddress(pairedDeviceAddress.c_str()));

    // Get the remote service
    pRemoteService = pClient->getService(BLEUUID(SONY_SERVICE_UUID));
    if (pRemoteService != nullptr)
    {
        pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(SONY_CHAR_UUID));
    }
}

void startBulbExposure()
{
    Serial.println("Starting exposure");
    if (pRemoteCharacteristic != nullptr && deviceConnected)
    {
        // Command to start bulb exposure (you'll need to implement the actual command)
        uint8_t cmd[] = {0x01}; // Placeholder command
        pRemoteCharacteristic->writeValue(cmd, sizeof(cmd));
        shooting = true;
        exposureStartTime = millis();
    }
}

void stopBulbExposure()
{
    Serial.println("Stopping exposure");
    if (pRemoteCharacteristic != nullptr && deviceConnected)
    {
        // Command to stop bulb exposure (you'll need to implement the actual command)
        uint8_t cmd[] = {0x00}; // Placeholder command
        pRemoteCharacteristic->writeValue(cmd, sizeof(cmd));
        shooting = false;
    }
}

void drawMainMenu()
{
    DisplayState currentState = {
        deviceConnected,
        shooting,
        exposureCount,
        targetExposures,
        currentExposureTime,
        shooting ? (millis() - exposureStartTime) / 1000 : 0,
        ""};

    bool needsFullRedraw = false;

    if (lastDisplayState.connected != currentState.connected ||
        lastDisplayState.isShooting != currentState.isShooting ||
        (!shooting && (lastDisplayState.currentShot != currentState.currentShot ||
                       lastDisplayState.targetShots != currentState.targetShots)))
    {
        needsFullRedraw = true;
    }

    if (needsFullRedraw)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        // Get device name from preferences if available
        String deviceName = "";
        if (!pairedDeviceAddress.isEmpty())
        {
            preferences.begin("camera", true);
            deviceName = preferences.getString("name", "");
            preferences.end();
        }

        if (deviceConnected)
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
            if (!pairedDeviceAddress.isEmpty() && deviceName.length() > 0)
            {
                M5.Display.printf("Disconnected: %s\n", deviceName.c_str());
            }
            else
            {
                M5.Display.println("Disconnected");
            }
        }

        if (shooting)
        {
            M5.Display.printf("Shot: %d/%d\n", exposureCount + 1, targetExposures == -1 ? 0 : targetExposures);
        }
        else
        {
            M5.Display.println("Ready");
            M5.Display.printf("Exp: %lus\n", currentExposureTime / 1000);
            M5.Display.printf("Count: %s\n", targetExposures == -1 ? "INF" : String(targetExposures));
        }

        if (!deviceConnected)
        {
            if (pairedDeviceAddress.isEmpty())
            {
                currentState.menuText = "\nB: Find Camera";
            }
            else
            {
                currentState.menuText = "\nA: Connect";
                currentState.menuText += "\nB: Menu";
            }
            M5.Display.print(currentState.menuText);
        }
    }
    else if (shooting)
    {
        M5.Display.setCursor(0, 32);
        M5.Display.printf("Time: %lus   \n", (millis() - exposureStartTime) / 1000);
    }

    lastDisplayState = currentState;
}

void drawScanMenu()
{
    static int lastSelectedDevice = -1;
    static bool lastScanning = !scanning;
    static size_t lastDeviceCount = 0;
    static unsigned long lastDisplayUpdate = 0;
    static int lastScrollPosition = -1;
    const unsigned long DISPLAY_UPDATE_INTERVAL = 200;

    bool needsFullRedraw = lastSelectedDevice != selectedDevice ||
                           lastScanning != scanning ||
                           lastDeviceCount != discoveredDevices.size() ||
                           lastScrollPosition != scanScrollPosition;

    unsigned long currentTime = millis();
    if (scanning && currentTime - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL)
    {
        return;
    }

    if (needsFullRedraw || (scanning && currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL))
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);

        if (scanning)
        {
            unsigned long timeLeft = (SCAN_DURATION - (currentTime - scanStartTime)) / 1000;
            M5.Display.printf("Scanning... %ds\n", timeLeft);
            M5.Display.printf("Found: %d\n\n", discoveredDevices.size());

            // Calculate scroll boundaries
            int maxScrollPosition = max(0, (int)discoveredDevices.size() - DEVICES_PER_PAGE);
            scanScrollPosition = min(scanScrollPosition, maxScrollPosition);

            // Show scrollable devices
            int endIdx = min(scanScrollPosition + DEVICES_PER_PAGE, (int)discoveredDevices.size());
            for (int i = scanScrollPosition; i < endIdx; i++)
            {
                M5.Display.printf("%s\n", discoveredDevices[i].name.c_str());
            }

            // Show scroll indicators if needed
            if (discoveredDevices.size() > DEVICES_PER_PAGE)
            {
                M5.Display.println();
                if (scanScrollPosition > 0)
                {
                    M5.Display.println("^ More above");
                }
                if (scanScrollPosition < maxScrollPosition)
                {
                    M5.Display.println("v More below");
                }
                M5.Display.printf("\nPage %d/%d",
                                  (scanScrollPosition / DEVICES_PER_PAGE) + 1,
                                  (discoveredDevices.size() + DEVICES_PER_PAGE - 1) / DEVICES_PER_PAGE);
            }
        }
        else
        {
            // Show all devices with pagination
            int itemsPerPage = 5;
            int currentPage = selectedDevice / itemsPerPage;
            int startIdx = currentPage * itemsPerPage;
            int endIdx = min(startIdx + itemsPerPage, (int)discoveredDevices.size());

            for (int i = startIdx; i < endIdx; i++)
            {
                M5.Display.print(i == selectedDevice ? "> " : "  ");
                M5.Display.printf("%s\n", discoveredDevices[i].name.c_str());
            }

            if (discoveredDevices.size() > itemsPerPage)
            {
                M5.Display.printf("\nPage %d/%d",
                                  currentPage + 1,
                                  (discoveredDevices.size() + itemsPerPage - 1) / itemsPerPage);
            }

            M5.Display.println("\nA:Select");
            M5.Display.println("Hold B:Back");
        }

        lastSelectedDevice = selectedDevice;
        lastScanning = scanning;
        lastDeviceCount = discoveredDevices.size();
        lastScrollPosition = scanScrollPosition;
        lastDisplayUpdate = currentTime;
    }
}

void handleScanMenu()
{
    if (scanning)
    {
        // Handle scrolling during scanning
        if (M5.BtnB.wasPressed() && !M5.BtnB.pressedFor(1000))
        {
            int maxScrollPosition = max(0, (int)discoveredDevices.size() - DEVICES_PER_PAGE);
            scanScrollPosition = (scanScrollPosition + DEVICES_PER_PAGE) % (maxScrollPosition + 1);
            drawScanMenu();
        }
    }
    else
    {
        if (M5.BtnA.wasPressed())
        {
            if (!discoveredDevices.empty())
            {
                Serial.println("Device selected from scan menu");
                auto &selectedDev = discoveredDevices[selectedDevice];
                selectedDev.updateName();

                pairedDeviceAddress = selectedDev.device->getAddress().toString().c_str();
                preferences.begin("camera", false);
                preferences.putString("addr", pairedDeviceAddress);
                if (selectedDev.device->haveName())
                {
                    preferences.putString("name", selectedDev.name);
                }
                else
                {
                    preferences.remove("name");
                }
                preferences.end();

                // Clean up
                for (auto &deviceInfo : discoveredDevices)
                {
                    delete deviceInfo.device;
                }
                discoveredDevices.clear();
                scanScrollPosition = 0;

                menuState = 0;
                drawMainMenu();
            }
        }
        else if (M5.BtnB.wasPressed() && !M5.BtnB.pressedFor(1000))
        {
            selectedDevice = (selectedDevice + 1) % discoveredDevices.size();
            drawScanMenu();
        }
    }
}

void drawDeviceMenu()
{
    static int lastSelected = 0;
    static int selected = 0;

    if (lastSelected != selected)
    {
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Device Menu");
        M5.Display.println();

        const char *items[] = {
            "Find Camera",
            "Forget Device",
            "Back"};

        for (int i = 0; i < 3; i++)
        {
            M5.Display.print(i == selected ? "> " : "  ");
            M5.Display.println(items[i]);
        }

        lastSelected = selected;
    }

    if (M5.BtnA.wasPressed())
    {
        switch (selected)
        {
        case 0: // Find Camera
            menuState = 1;
            startScanning();
            break;
        case 1: // Forget Device
            if (!pairedDeviceAddress.isEmpty())
            {
                Serial.println("Forgetting paired device");
                preferences.begin("camera", false);
                preferences.remove("addr");
                preferences.remove("name");
                preferences.end();
                pairedDeviceAddress = "";

                if (deviceConnected && pClient != nullptr)
                {
                    pClient->disconnect();
                    delete pClient;
                    pClient = nullptr;
                }
                deviceConnected = false;

                menuState = 0;
                M5.Display.fillScreen(BLACK);
                M5.Display.println("Device forgotten");
                delay(1000);
            }
            menuState = 0;
            break;
        case 2: // Back
            menuState = 0;
            break;
        }
        drawMainMenu();
    }
    else if (M5.BtnB.wasPressed())
    {
        selected = (selected + 1) % 3;
    }
}

void goBack()
{
    scanning = false; // Stop any active scanning
    selectedDevice = 0;
    scanScrollPosition = 0;
    menuState = 0; // Force return to home menu
    drawMainMenu();
}

void handleMainMenu()
{
    if (M5.BtnA.wasPressed())
    {
        Serial.println("Button A pressed in main menu");
        if (!deviceConnected && !pairedDeviceAddress.isEmpty())
        {
            // Connect to paired device
            connectToCamera();
        }
        else if (!shooting)
        {
            exposureCount = 0;
            startBulbExposure();
        }
        else
        {
            stopBulbExposure();
        }
    }

    if (M5.BtnB.wasPressed())
    {
        Serial.println("Button B pressed in main menu");
        if (!shooting)
        {
            if (pairedDeviceAddress.isEmpty())
            {
                menuState = 1; // Go directly to scanning
                startScanning();
            }
            else
            {
                menuState = 2; // Show device menu
            }
        }
        else if (!shooting)
        {
            // Cycle exposure count: INF -> 10 -> 20 -> ... -> 100 -> INF
            if (targetExposures == -1)
                targetExposures = 10;
            else
                targetExposures = (targetExposures + 10) > 100 ? -1 : targetExposures + 10;
        }
    }
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextSize(1.5, 1.75);

    Serial.begin(115200);
    Serial.println("Starting Sony Camera Remote");

    M5.Display.setBaseColor(BLACK);
    M5.Display.fillScreen(BLACK);
    M5.Display.println("Sony Camera Remote");

    delay(1000); // Give some time to read the header

    BLEDevice::init("M5StickC Remote");

    // Load paired device from preferences
    preferences.begin("camera", false);
    pairedDeviceAddress = preferences.getString("addr", "");
    preferences.end();

    Serial.printf("Loaded paired device: %s\n", pairedDeviceAddress.c_str());

    drawMainMenu(); // Draw initial menu
}

void loop()
{
    M5.update();

    if (scanning)
    {
        unsigned long currentTime = millis();
        if (currentTime - scanStartTime >= SCAN_DURATION)
        {
            Serial.println("Scan timeout reached");
            stopScanning();
        }
    }

    switch (menuState)
    {
    case 0:
        handleMainMenu();
        break;
    case 1:
        handleScanMenu();
        break;
    case 2:
        drawDeviceMenu();
        break;
    }

    if (M5.BtnB.pressedFor(1000))
    {
        if (scanning)
        {
            stopScanning();
        }
        goBack();
    }

    // Update display based on current menu
    if (menuState == 0)
    {
        // Auto-stop exposure if time is reached
        if (shooting && (millis() - exposureStartTime >= currentExposureTime))
        {
            stopBulbExposure();
            exposureCount++;

            // Start next exposure if we haven't reached the target
            if (targetExposures == -1 || exposureCount < targetExposures)
            {
                delay(1000); // 1 second delay between exposures
                startBulbExposure();
            }
        }

        // Update display every 1 second
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate >= 1000)
        {
            drawMainMenu();
            lastUpdate = millis();
        }

        // Reconnect if disconnected
        if (!deviceConnected && !pairedDeviceAddress.isEmpty() && pClient != nullptr)
        {
            delay(500);
            connectToCamera();
        }
    }
    else if (menuState == 1)
    {
        // Check if scanning is complete
        if (scanning && (millis() - scanStartTime >= SCAN_DURATION))
        {
            Serial.println("Scan complete");
            scanning = false;
            selectedDevice = 0;
        }

        // Update display
        static unsigned long lastScanUpdate = 0;
        if (millis() - lastScanUpdate >= 500)
        {
            drawScanMenu();
            lastScanUpdate = millis();
        }
    }
}
