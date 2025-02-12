#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/camera_commands.h"
#include "../transport/remote_control_manager.h"

enum class VideoMenuItem
{
    None
};

class VideoScreen : public BaseScreen<VideoMenuItem>
{
public:
    VideoScreen() : BaseScreen<VideoMenuItem>("Video"), recordStartTime(0) {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    unsigned long recordStartTime;
};

inline void VideoScreen::drawContent()
{
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

    if (CameraCommands::isRecording())
    {
        // Red background when recording
        M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), RED);
        M5.Display.setTextColor(WHITE);

        // Show recording time
        unsigned long recordTime = (millis() - recordStartTime) / 1000;
        int minutes = recordTime / 60;
        int seconds = recordTime % 60;

        char timeStr[10];
        sprintf(timeStr, "%02d:%02d", minutes, seconds);

        M5.Display.setTextSize(3);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(timeStr, centerX, centerY);
    }
    else
    {
        // Normal display
        M5.Display.fillScreen(BLACK);

        // Draw record symbol
        int radius = 20;
        M5.Display.fillCircle(centerX, centerY, radius, RED);
        M5.Display.drawCircle(centerX, centerY, radius + 2, WHITE);
    }

    setStatusBgColor(M5.Display.color565(32, 32, 32));
    setStatusText(CameraCommands::isRecording() ? "Recording" : "Ready");
    drawStatusBar();
}

inline void VideoScreen::update()
{
    if (M5.BtnA.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM))
    {
        LOG_PERIPHERAL("[VideoScreen] [Btn] Confirm Button Clicked");

        if (CameraCommands::isRecording() && CameraCommands::recordStop())
        {
            recordStartTime = 0;
        }
        else if (CameraCommands::recordStart())
        {
            recordStartTime = millis();
        }
        draw();
    }

    // Update recording time display
    if (CameraCommands::isRecording())
    {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 1000)
        {
            lastUpdate = millis();
            draw();
        }
    }
}
