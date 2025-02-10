#include <M5Unified.h>
#include "components/menu_system.h"
#include "transport/ble_device.h"
#include "transport/encoder_device.h"
#include "utils/preferences.h"
#include "debug.h"

// Global variables for encoder state
int32_t lastEncoderValue = 0;

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    LOG_APP("Starting Sony Camera Remote");

    // Initialize preferences first
    PreferencesManager::init();

    // Initialize encoder
    EncoderDevice::init();

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
    if (EncoderDevice::isAvailable())
    {
        EncoderDevice::update();
    }

    BLEDeviceManager::update(); // Update BLE state
    MenuSystem::update();       // This will handle M5.update() internally
    delay(10);                  // Small delay to prevent tight loop
}
