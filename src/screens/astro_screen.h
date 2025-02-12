#pragma once

#include <M5Unified.h>

#include "base_screen.h"

enum class AstroMenuItem {
    Focus,
    InitialDelay,
    ExposureTime,
    NumberOfExposures,
    DelayBetweenExposures,
    Start,
    Pause,
    Stop
};

class AstroScreen : public BaseScreen<AstroMenuItem> {
public:
    AstroScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    int selectedItem = 0;
};
