#include <M5Unified.h>
#include <Arduino.h>
#include "ble_device.h"
#include "menu_system.h"
#include "camera_control.h"

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("Starting Sony Camera Remote");

    BLEDeviceManager::init();
    CameraControl::init();
    MenuSystem::init();
}

void loop()
{
    M5.update();
    MenuSystem::update();
    MenuSystem::draw();
    delay(5);
}
