#pragma once

#include <M5Unified.h>
#include <Preferences.h>
#include "ble_device.h"
#include "selectable_list.h"

class MenuSystem
{
public:
    static void init();
    static void update();
    static void drawMenu();

private:
    // Menu state
    static int currentMenu; // 0: Main, 1: Scan, 2: Control, 3: Settings
    static bool needsRedraw;
    static bool needsFullRedraw;       // True when we need to redraw everything
    static bool navigationTextChanged; // True when navigation text has changed

    // Navigation button colors
    static const uint16_t NAV_BG_COLOR = 0x1082;   // #161616 - almost black
    static const uint16_t NAV_TEXT_COLOR = 0xef7b; // #e7e7e7 - almost white
    static const uint16_t NAV_HEIGHT = 16;         // Height of navigation bar

    // Lists for each menu
    static SelectableList mainList;
    static SelectableList scanList;
    static SelectableList controlList;
    static SelectableList settingsList;

    // Helper to draw navigation buttons
    static void drawNavigation(const char *leftBtn = nullptr, const char *rightBtn = nullptr);

    static void drawMainMenu();
    static void drawScanMenu();
    static void drawDeviceMenu();
    static void drawControlMenu();
    static void drawSettingsMenu();

    static void handleMainMenu();
    static void handleScanMenu();
    static void handleDeviceMenu();
    static void handleSettingsMenu();
    static void handleControlMenu();

    static void goBack();
};
