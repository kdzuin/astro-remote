#pragma once

#include <M5Unified.h>
#include "../transport/camera_commands.h"
#include "base_screen.h"

class VideoScreen : public BaseScreen
{
public:
    VideoScreen() : BaseScreen("Video"), lastRecordingState(false) {}

    void drawContent() override
    {
        int centerX = M5.Display.width() / 2;
        int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

        if (CameraCommands::isRecording())
        {
            // Tally light - full red screen with REC text
            M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), M5.Display.color565(255, 0, 0));
            M5.Display.setTextColor(WHITE);

            // Center text by calculating position
            M5.Display.setTextSize(3);
            M5.Display.setTextDatum(middle_center);
            M5.Display.drawString("REC", centerX, centerY - 20);

            // Show time elapsed
            M5.Display.setTextSize(2);
            uint32_t elapsed = (millis() - recordStartTime) / 1000; // seconds
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d", (int)(elapsed / 60), (int)(elapsed % 60));

            M5.Display.setTextDatum(middle_center);
            M5.Display.drawString(timeStr, centerX, centerY + 20);

            setStatusBgColor(M5.Display.color888(5, 5, 5));
            setStatusText("Recording");
            drawStatusBar();
        }
        else
        {
            // Ready to record screen
            M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), BLACK);
            M5.Display.drawRoundRect(centerX - 20, centerY - 20, 40, 40, 4, WHITE);
            M5.Display.fillCircle(centerX, centerY, 10, RED);

            setStatusBgColor(M5.Display.color888(32, 32, 32));
            setStatusText("Ready To Record");
            drawStatusBar();
        }
    }

    void beforeExit() override
    {
        // Stop recording if it's active
        if (CameraCommands::isRecording())
        {
            CameraCommands::recordStop();
        }
    }

    void update() override
    {
        bool isRecording = CameraCommands::isRecording();

        // Check if recording state changed
        if (isRecording != lastRecordingState)
        {
            if (isRecording)
            {
                recordStartTime = millis();
            }
            draw(); // Update screen immediately
            lastRecordingState = isRecording;
        }

        // Update timer while recording
        if (isRecording && (millis() - lastTimerUpdate > 1000))
        {
            draw(); // Update timer display
            lastTimerUpdate = millis();
        }

        if (M5.BtnA.wasClicked())
        {
            if (isRecording)
            {
                CameraCommands::recordStop();
            }
            else
            {
                recordStartTime = millis();
                CameraCommands::recordStart();
            }
        }
    }

private:
    uint32_t recordStartTime = 0;
    uint32_t lastTimerUpdate = 0;
    bool lastRecordingState;
};
