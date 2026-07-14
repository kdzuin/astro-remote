#pragma once

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/base_screen.h"

enum class AstroMenuItem {
    Connect,
    Focus,
    Start,
    Pause,
    Stop,
    ExposureTime,
    SubframeCount,
    DelayBetweenExposures,
    InitialDelay,
};

// Config screen for astro sequences: connection, Start/Resume, Focus, and the
// exposure parameters. The live in-progress display is AstroRunScreen, so this
// screen never has to branch on a running sequence and is not an observer.
class AstroScreen : public BaseScreen<AstroMenuItem> {
public:
    AstroScreen();
    ~AstroScreen() override;

    void update() override;
    void nextMenuItem();
    void prevMenuItem();

private:
    void drawContent();
    void updateMenuItems();
    void adjustParameter(uint16_t delta);
    void selectMenuItem() { handleSelect(); }  // BaseScreen hook
    // Returns true if it navigated to another screen (this screen is then
    // deleted — the caller must not touch any member afterwards).
    bool handleSelect();
    int selectedItem = 0;
};
