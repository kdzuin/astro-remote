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
        LOG_DEBUG("[UI] Parameters changed: exp=%ds, frames=%d, interval=%ds", params.exposureSec,
                  params.subframeCount, params.intervalSec);
        updateMenuItems();  // Refresh menu with new parameter values
    }

    void onAstroStatusChanged(const AstroProcess::Status& status) override {
        LOG_DEBUG("[UI] Status changed: state=%d, frames=%d/%d, elapsed=%ds",
                  static_cast<int>(status.state), status.completedFrames + 1, status.totalFrames,
                  status.elapsedSec);
        updateMenuItems();  // Update menu items based on new state
    }

    void drawContent();
    void updateMenuItems();
    void adjustParameter(uint16_t delta);
    void selectMenuItem();
    int selectedItem = 0;
};
