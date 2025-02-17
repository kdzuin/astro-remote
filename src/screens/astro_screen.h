#pragma once

#include "screens/base_screen.h"

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
    ~AstroScreen() override;
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;

private:
    // Parameter increment/decrement constants
    void adjustParameter(uint16_t delta);
    void selectMenuItem();
    void nextMenuItem();
    void prevMenuItem();

    int selectedItem = 0;
};
