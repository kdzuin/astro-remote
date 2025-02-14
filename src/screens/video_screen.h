#pragma once

#include <M5Unified.h>

#include "processes/video.h"
#include "screens/base_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

enum class VideoMenuItem { None };

class VideoScreen : public BaseScreen<VideoMenuItem> {
public:
    VideoScreen() : BaseScreen<VideoMenuItem>("Video") {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    VideoProcess videoProcess;
};

inline void VideoScreen::drawContent() {
    int centerX = M5.Display.width() / 2;
    int centerY = M5.Display.height() / 2;

    if (videoProcess.isRecording()) {
        // Red background when recording
        M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(),
                            colors::get(colors::RED));
        M5.Display.setTextColor(colors::get(colors::WHITE));

        // Show recording time
        char timeStr[10];
        videoProcess.getFormattedTime(timeStr, sizeof(timeStr));

        M5.Display.setTextSize(3);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(timeStr, centerX, centerY);
    } else {
        // Normal display
        M5.Display.fillScreen(colors::get(colors::BLACK));

        // Draw record symbol
        int radius = 20;
        M5.Display.fillCircle(centerX, centerY, radius, colors::get(colors::RED));
        M5.Display.drawCircle(centerX, centerY, radius + 2, colors::get(colors::WHITE));
    }

    setStatusBgColor(colors::get(colors::GRAY_800));
    setStatusText(videoProcess.isRecording() ? "Recording" : "Ready");
    drawStatusBar();
}

inline void VideoScreen::update() {
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[VideoScreen] [Btn] Confirm Button Clicked");

        if (videoProcess.isRecording()) {
            videoProcess.stopRecording();
        } else {
            videoProcess.startRecording();
        }
        draw();
    }

    // Update recording time display
    if (videoProcess.isRecording()) {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 1000) {
            lastUpdate = millis();
            draw();
        }
    }
}
