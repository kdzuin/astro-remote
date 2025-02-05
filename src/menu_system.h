#pragma once

#include <M5Unified.h>
#include <Preferences.h>
#include "ble_device.h"

class MenuSystem {
public:
    static void init();
    static void update();

private:
    static void drawMenu();
    static void drawMainMenu();
    static void drawDeviceMenu();
    static void drawControlMenu();
    static void drawScanMenu();
    
    static void handleMainMenu();
    static void handleDeviceMenu();
    static void handleControlMenu();
    static void handleScanMenu();
    
    static void goBack();
    
    static int menuState;
    static int selectedDevice;
    static bool needsRedraw;
};
