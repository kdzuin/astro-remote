#pragma once

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/base_screen.h"

enum class AstroMenuItem {
    Focus,
    Start,
    Pause,
    Stop,
    ExposureTime,
    SubframeCount,
    DelayBetweenExposures,
    InitialDelay,
};

class AstroScreen : public BaseScreen<AstroMenuItem>, public AstroProcess::Observer {
public:
    AstroScreen();
    ~AstroScreen() override;

    void update() override;
    void nextMenuItem();
    void prevMenuItem();

private:
    // Observer interface implementation
    void onAstroParametersChanged(const AstroProcess::Parameters& params) override {
        updateMenuItems();  // Refresh menu with new parameter values
    }

    void onAstroStatusChanged(const AstroProcess::Status& status) override {
        updateMenuItems();  // Update menu items based on new state
    }

    void drawContent();
    void updateMenuItems();
    void adjustParameter(uint16_t delta);
    void selectMenuItem();
    int selectedItem = 0;
};
