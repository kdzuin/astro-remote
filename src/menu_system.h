#pragma once

#include <M5Unified.h>
#include <Preferences.h>
#include "ble_device.h"

class MenuSystem {
public:
    static void init();
    static void update();
    static void drawMenu();

private:
    static int currentMenu;  // 0: Main, 1: Scan, 2: Control, 3: Settings
    static int selectedDevice;
    static bool needsRedraw;
    
    static void drawMainMenu();
    static void drawScanMenu();
    static void drawDeviceMenu();
    static void drawControlMenu();
    static void drawSettingsMenu();
    
    static void handleMainMenu();
    static void handleScanMenu();
    static void handleDeviceMenu();
    static void handleControlMenu();
    static void handleSettingsMenu();
    
    static void goBack();
};
