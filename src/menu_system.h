#pragma once

#include <M5Unified.h>
#include <Preferences.h>
#include "camera_control.h"
#include "ble_device.h"

class MenuSystem
{
public:
    static void init();
    static void update();
    static void draw();

private:
    static void drawMainMenu();
    static void drawScanMenu();
    static void drawDeviceMenu();

    static void handleMainMenu();
    static void handleScanMenu();
    static void handleDeviceMenu();
    static void goBack();

    static int menuState;
    static int selectedDevice;
    static bool forceRedraw;
};
