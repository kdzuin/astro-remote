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
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

    M5.Display.fillScreen(colors::get(colors::BLACK));
    M5.Display.setTextColor(colors::get(colors::WHITE));

    // Draw main focus status
    M5.Display.setTextSize(2);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString(FocusProcess::getState().focusing ? "FOCUSING" : "Ready", centerX,
                          centerY);

    // Draw sensitivity below
    M5.Display.setTextSize(1.25);
    M5.Display.drawString(FocusProcess::getSensitivityText(), centerX, centerY + 30);

    // Update status bar
    setStatusBgColor(FocusProcess::getStatusColor());
    setStatusText(FocusProcess::getStatusText());
    drawStatusBar();
}

inline void FocusScreen::update() {
    if (M5.BtnA.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_APP("[FocusScreen] Toggle focus mode");
        FocusProcess::updateFocusState(!FocusProcess::getState().focusing);
        draw();
    } else if (M5.BtnB.wasClicked()) {
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
