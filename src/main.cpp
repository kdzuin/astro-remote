#include <M5Unified.h>
#include "components/menu_system.h"
#include "transport/ble_device.h"

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("Starting Sony Camera Remote");

    M5.Display.setRotation(0);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextSize(1.75);
    int x = (M5.Display.width() - M5.Display.textWidth("Astro Remote")) / 2;
    int y = (M5.Display.height() - M5.Display.fontHeight()) / 2;
    M5.Display.setCursor(x, y);
    M5.Display.println("Astro Remote");
    delay(1000);

    M5.Display.setTextSize(1.25);
    M5.Display.setBrightness(80);

    BLEDeviceManager::init();
    MenuSystem::init();
}

void loop()
{
    BLEDeviceManager::update();  // Update BLE state
    MenuSystem::update();        // This will handle M5.update() internally
    delay(10);                   // Small delay to prevent tight loop
}
