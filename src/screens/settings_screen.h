#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"

enum class SettingsMenuItem
{
    Disconnect,
    Forget,
    Scan,
    AutoConnect,
    Brightness,
};

class SettingsScreen : public BaseScreen<SettingsMenuItem>
{
public:
    SettingsScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;

private:
    int brightness;
};
