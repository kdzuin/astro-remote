#pragma once

#include <M5Unified.h>

#include "components/selectable_list.h"
#include "processes/astro.h"
#include "screens/base_screen.h"

// Actions available while a sequence is in progress.
enum class AstroRunItem { PauseResume, Stop };

// The in-progress astro screen, shown while a sequence runs. Separate from
// AstroScreen (the config menu) so neither screen has to branch on isRunning().
//
// Rendered as two independent off-screen canvases pushed to non-overlapping
// regions, so nothing flickers:
//   - topCanvas_: title + action menu (Pause/Resume, Stop). Pushed only when
//     the selection or run state changes.
//   - botCanvas_: live stats (Frame/Elapsed/Left) + the colour-coded status
//     bar. Pushed once per second as the timers tick.
// Because each pushSprite only touches its own rect, the per-second stats
// refresh never disturbs the menu region.
//
// Not an observer: update() runs every main-loop tick anyway, so it polls
// AstroProcess and redraws the region that changed. The web-facing
// BLEAstroObserver (registered at app startup) reports status independently.
class AstroRunScreen : public BaseScreen<AstroRunItem> {
public:
    AstroRunScreen();
    ~AstroRunScreen() override;

    void update() override;
    void draw() override;  // full repaint of both regions

    // Unused menu hooks (this screen manages its own tiny action list).
    void updateMenuItems() override {}
    void drawContent() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    void drawTop();  // title + actions (menu region)
    void drawBottom();  // stats + status bar

    // Change fingerprints, per region.
    int lastAction_ = -1;
    int lastState_ = -1;  // affects the action labels (Pause vs Resume)
    uint32_t lastElapsed_ = 0xFFFFFFFF;
    bool lastConnected_ = false;
    bool lastSummary_ = false;

    SelectableList<AstroRunItem> actions_;  // title + Pause/Resume, Stop
    M5Canvas topCanvas_;
    M5Canvas botCanvas_;
    int topH_ = 0;  // height of the top (menu) region; bottom fills the rest
    bool spritesReady_ = false;
    bool summaryMode_ = false;  // Sequence ended; showing the summary until back.
    int actionIndex_ = 0;       // 0 = Pause/Resume, 1 = Stop
};
