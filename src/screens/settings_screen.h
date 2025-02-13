#pragma once

#include "processes/settings.h"
#include "screens/base_screen.h"

enum class SettingsMenuItem { Connect, Disconnect, Forget, Scan, AutoConnect, Brightness, Battery };

class SettingsScreen : public BaseScreen<SettingsMenuItem> {
private:
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
