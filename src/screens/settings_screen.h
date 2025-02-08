#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"

enum class SettingsMenuItem
{
    Connect,
    Disconnect,
    Forget,
    Scan,
    AutoConnect,
    Brightness,
    Battery
};

class SettingsScreen : public BaseScreen<SettingsMenuItem>
{
public:
    SettingsScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;

private:
    uint16_t getStatusBgColor(int batteryLevel);
    int selectedItem = 0;
};
