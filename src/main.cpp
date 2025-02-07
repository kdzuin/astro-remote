#include <M5Unified.h>
#include "components/menu_system.h"
#include "transport/ble_device.h"
#include "utils/preferences.h"
#include "libraries/FairyEncoder/FairyEncoder.h"

FairyEncoder fencoder;

// Global variables for encoder state
int16_t lastEncoderValue = 0;
bool lastButtonState = false;
bool encoderAvailable = false;

// Function declarations
bool initEncoder();
void handleRotation();
void handleButton();
void setEncoderLED(uint32_t color);

bool initEncoder()
{
    fencoder.begin();
    // Check if encoder is available by trying to communicate with it
    encoderAvailable = fencoder.avail();
    if (encoderAvailable)
    {
        setEncoderLED(0x000000); // LED off initially
        Serial.println("Encoder initialized successfully");
    }
    else
    {
        Serial.println("Encoder not found - check connections");
    }
    return encoderAvailable;
}

void handleRotation()
{
    if (!encoderAvailable)
        return;

    int16_t currentValue = fencoder.read();
    if (currentValue != lastEncoderValue)
    {
        if (currentValue > lastEncoderValue)
        {
            Serial.println("Rotating clockwise");
            setEncoderLED(0x00FF00); // Green for clockwise
        }
        else
        {
            Serial.println("Rotating counter-clockwise");
            setEncoderLED(0x0000FF); // Blue for counter-clockwise
        }
        lastEncoderValue = currentValue;
    }
}

void handleButton()
{
    if (!encoderAvailable)
        return;

    bool buttonState = fencoder.getButtonStatus();
    if (buttonState != lastButtonState)
    {
        if (buttonState)
        {
            Serial.println("Button pressed");
            setEncoderLED(0xFF0000); // Red when button pressed
        }
        else
        {
            Serial.println("Button released");
            setEncoderLED(0x000000); // LED off when button released
        }
        lastButtonState = buttonState;
    }
}

void setEncoderLED(uint32_t color)
{
    if (!encoderAvailable)
        return;
    fencoder.setLEDColor(0, color);
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("Starting Sony Camera Remote");

    // Initialize preferences first
    PreferencesManager::init();

    // Try to initialize encoder
    initEncoder();

    M5.Display.setRotation(0);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1.75);
    int x = (M5.Display.width() - M5.Display.textWidth("Astro Remote")) / 2;
    int y = (M5.Display.height() - M5.Display.fontHeight()) / 2;
    M5.Display.setCursor(x, y);
    M5.Display.println("Astro Remote");
    delay(1000);

    M5.Display.setTextSize(1.25);
    M5.Display.setBrightness(PreferencesManager::getBrightness());

    BLEDeviceManager::setAutoConnect(PreferencesManager::getAutoConnect());
    BLEDeviceManager::init();

    MenuSystem::init();
}

void loop()
{
    if (encoderAvailable)
    {
        // Update encoder state only if it's available
        fencoder.task();
        handleRotation();
        handleButton();
    }

    BLEDeviceManager::update(); // Update BLE state
    MenuSystem::update();       // This will handle M5.update() internally
    delay(10);                  // Small delay to prevent tight loop
}
