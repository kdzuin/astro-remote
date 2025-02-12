#pragma once

#include "hardware_interface.h"
#include "screens/base_screen.h"
#include "transport/ble_device.h"

enum class SettingsMenuItem { Connect, Disconnect, Forget, Scan, AutoConnect, Brightness, Battery };

class SettingsScreen : public BaseScreen<SettingsMenuItem> {
private:
    uint16_t getStatusBgColor(int batteryLevel);
    int selectedItem = 0;

public:
    SettingsScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;
};
