#pragma once

#include <M5Unified.h>

#include "screens/base_screen.h"

// Full-screen low-battery takeover. Shown when the battery reaches the emergency
// threshold (SettingsProcess::BATTERY_EMERGENCY_PCT) while an astro sequence is
// running: the sequence is paused and this screen flashes red until the user
// stops it or the battery is charged back above the threshold (auto-resume).
//
// Faster flash than the run-screen critical alert: 300ms red out of every
// 3000ms, brightness boosted during the red pulse.
class EmergencyScreen : public BaseScreen<BaseMenuItem> {
public:
    EmergencyScreen();

    void update() override;
    void draw() override;

    // No menu on this screen.
    void updateMenuItems() override {}
    void drawContent() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    bool lastFlashOn_ = false;  // red pulse currently painted
};
