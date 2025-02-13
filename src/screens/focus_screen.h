#pragma once

#include "processes/focus.h"
#include "screens/base_screen.h"
#include "transport/remote_control_manager.h"

enum class FocusMenuItem { None };

class FocusScreen : public BaseScreen<FocusMenuItem> {
public:
    FocusScreen() : BaseScreen<FocusMenuItem>("Focus") {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}
};

inline void FocusScreen::drawContent() {
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

    display().fillScreen(display().getColor(display::colors::BLACK));
    display().setTextColor(display().getColor(display::colors::WHITE));

    // Draw main focus status
    display().setTextSize(2);
    display().setTextAlignment(textAlign::middle_center);
    display().drawString(FocusProcess::getState().focusing ? "FOCUSING" : "Ready", centerX, centerY);

    // Draw sensitivity below
    display().setTextSize(1.25);
    display().drawString(FocusProcess::getSensitivityText(), centerX, centerY + 30);

    // Update status bar
    setStatusBgColor(FocusProcess::getStatusColor(display()));
    setStatusText(FocusProcess::getStatusText());
    drawStatusBar();
}

inline void FocusScreen::update() {
    if (input().wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_APP("[FocusScreen] Toggle focus mode");
        FocusProcess::updateFocusState(!FocusProcess::getState().focusing);
        draw();
    } else if (input().wasButtonPressed(ButtonId::BTN_B)) {
        LOG_APP("[FocusScreen] Cycle sensitivity");
        FocusProcess::cycleSensitivity();
        draw();
    } else if (RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_APP("[FocusScreen] Next sensitivity");
        if (FocusProcess::nextSensitivity()) {
            draw();
        }
    } else if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_APP("[FocusScreen] Previous sensitivity");
        if (FocusProcess::prevSensitivity()) {
            draw();
        }
    }

    if (FocusProcess::getState().focusing) {
        if (RemoteControlManager::wasButtonPressed(ButtonId::LEFT)) {
            FocusProcess::handleFocus(1);
        } else if (RemoteControlManager::wasButtonPressed(ButtonId::RIGHT)) {
            FocusProcess::handleFocus(-1);
        }
    }
}
