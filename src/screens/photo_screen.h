#pragma once

#include "screens/base_screen.h"
#include "transport/camera_commands.h"
#include "transport/remote_control_manager.h"

enum class PhotoMenuItem { None };

class PhotoScreen : public BaseScreen<PhotoMenuItem> {
public:
    PhotoScreen() : BaseScreen<PhotoMenuItem>("Photo"), photoCount(0), flashStartTime(0) {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    int photoCount;
    unsigned long flashStartTime;
};

inline void PhotoScreen::drawContent() {
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

    if (millis() - flashStartTime < 200) {
        // Flash effect
        display().fillScreen(display().getColor(display::colors::WHITE));
        display().setTextColor(display().getColor(display::colors::BLACK));
    } else {
        // Normal display
        display().fillScreen(display().getColor(display::colors::BLACK));
        display().setTextColor(display().getColor(display::colors::WHITE));

        // Draw photo counter
        display().setTextSize(3);
        display().setTextAlignment(textAlign::middle_center);
        char countStr[10];
        sprintf(countStr, "%d", photoCount);
        display().drawString(countStr, centerX, centerY);
    }

    setStatusBgColor(display().getColor(display::colors::GRAY_800));
    setStatusText("Ready");
    drawStatusBar();
}

inline void PhotoScreen::update() {
    if (input().wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[PhotoScreen] [Btn] Confirm Button Clicked");

        if (CameraCommands::takePhoto()) {
            photoCount++;
            flashStartTime = millis();
            draw();
        }
    }

    // Redraw if we're in flash mode to clear it
    if (millis() - flashStartTime > 200 && flashStartTime > 0) {
        flashStartTime = 0;
        draw();
    }
}
