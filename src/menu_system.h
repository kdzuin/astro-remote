#pragma once

#include <M5Unified.h>
#include <Preferences.h>
#include "ble_device.h"
#include "camera_control.h"

class MenuSystem
{
public:
    static void init();
    static void update();
    static void draw();
    static void goBack();

    static const int DEVICES_PER_PAGE = 4;

private:
    static void drawMainMenu();
    static void drawScanMenu();
    static void drawDeviceMenu();
    static void handleMainMenu();
    static void handleScanMenu();
    static void handleDeviceMenu();
    static void handleTiltScroll();

    static int menuState;      // 0: main menu, 1: scanning menu, 2: device menu
    static int selectedDevice; // For device selection menu
    static int scanScrollPosition;
    static unsigned long scanStartTime;
    static const unsigned long DISPLAY_UPDATE_INTERVAL; // ms
    static bool forceRedraw;
    
    // Tilt scroll parameters
    static const float TILT_THRESHOLD;  // degrees
    static float lastPitch;
    static unsigned long lastTiltTime;
    static const unsigned long TILT_COOLDOWN;  // ms
};
