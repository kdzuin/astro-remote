#pragma once

#include "screens/base_screen.h"
#include "transport/remote_control_manager.h"
#include "processes/video.h"

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
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

    if (videoProcess.isRecording()) {
        // Red background when recording
        display().fillRect(0, 0, display().width(), display().height(),
                           display().getColor(display::colors::RED));
        display().setTextColor(display().getColor(display::colors::WHITE));

        // Show recording time
        char timeStr[10];
        videoProcess.getFormattedTime(timeStr, sizeof(timeStr));

        display().setTextSize(3);
        display().setTextAlignment(textAlign::middle_center);
        display().drawString(timeStr, centerX, centerY);
    } else {
        // Normal display
        display().fillScreen(display().getColor(display::colors::BLACK));

        // Draw record symbol
        int radius = 20;
        display().fillCircle(centerX, centerY, radius, display().getColor(display::colors::RED));
        display().drawCircle(centerX, centerY, radius + 2,
                             display().getColor(display::colors::WHITE));
    }

    setStatusBgColor(display().getColor(display::colors::GRAY_800));
    setStatusText(videoProcess.isRecording() ? "Recording" : "Ready");
    drawStatusBar();
}

inline void VideoScreen::update() {
    if (input().wasButtonPressed(ButtonId::BTN_A) ||
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
