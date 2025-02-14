#pragma once

#include "processes/photo.h"
#include "screens/base_screen.h"
#include "transport/remote_control_manager.h"

enum class PhotoMenuItem { None };

class PhotoScreen : public BaseScreen<PhotoMenuItem> {
public:
    PhotoScreen() : BaseScreen<PhotoMenuItem>("Photo") {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    PhotoProcess photoProcess;
};

inline void PhotoScreen::drawContent() {
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

    if (photoProcess.isFlashActive()) {
        // Flash effect
        M5.Display.fillScreen(colors::get(colors::WHITE));
        M5.Display.setTextColor(colors::get(colors::BLACK));
    } else {
        // Normal display
        M5.Display.fillScreen(colors::get(colors::BLACK));
        M5.Display.setTextColor(colors::get(colors::WHITE));

        // Draw photo counter
        M5.Display.setTextSize(3);
        M5.Display.setTextDatum(middle_center);
        char countStr[10];
        sprintf(countStr, "%d", photoProcess.getPhotoCount());
        M5.Display.drawString(countStr, centerX, centerY);
    }

    setStatusBgColor(colors::get(colors::GRAY_800));
    setStatusText("Ready");
    drawStatusBar();
}

inline void PhotoScreen::update() {
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[PhotoScreen] [Btn] Confirm Button Clicked");

        if (photoProcess.takePhoto()) {
            draw();
        }
    }

    // Redraw if we're in flash mode to clear it
    if (photoProcess.shouldClearFlash()) {
        photoProcess.clearFlash();
        draw();
    }
}
