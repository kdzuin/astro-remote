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

    static constexpr uint16_t INITIAL_DELAY_STEP = 5;
    static constexpr uint16_t INITIAL_DELAY_MIN = 0;
    static constexpr uint16_t INITIAL_DELAY_MAX = 5;

    static constexpr uint16_t EXPOSURE_STEP = 30;
    static constexpr uint16_t EXPOSURE_MIN = 30;
    static constexpr uint16_t EXPOSURE_MAX = 300;

    static constexpr uint16_t SUBFRAME_COUNT_STEP = 10;
    static constexpr uint16_t SUBFRAME_COUNT_MIN = 10;
    static constexpr uint16_t SUBFRAME_COUNT_MAX = 1000;

    static constexpr uint16_t INTERVAL_STEP = 1;
    static constexpr uint16_t INTERVAL_MIN = 1;
    static constexpr uint16_t INTERVAL_MAX = 10;

    int selectedItem = 0;
};
