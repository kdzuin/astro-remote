#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"
#include "../components/menu_system.h"

enum class ScanMenuItem
{
    Device // Each menu item will be a discovered device
};

class ScanScreen : public BaseScreen<ScanMenuItem>
{
public:
    ScanScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    bool lastScanning;
    bool isConnecting;
};
