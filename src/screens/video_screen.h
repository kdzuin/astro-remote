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
        if (CameraCommands::isRecording())
        {
            // Tally light - full red screen with REC text
            M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), M5.Display.color565(255, 0, 0));
            M5.Display.setTextColor(WHITE);
            M5.Display.setTextSize(3);

            // Center text by calculating position
            int textWidth = M5.Display.textWidth("REC");
            int x = (M5.Display.width() - textWidth) / 2;
            int y = M5.Display.height() / 2 - 20;
            M5.Display.drawString("REC", x, y);

            // Show time elapsed
            M5.Display.setTextSize(2);
            uint32_t elapsed = (millis() - recordStartTime) / 1000; // seconds
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d", (int)(elapsed / 60), (int)(elapsed % 60));
            textWidth = M5.Display.textWidth(timeStr);
            x = (M5.Display.width() - textWidth) / 2;
            y = M5.Display.height() / 2 + 20;
            M5.Display.drawString(timeStr, x, y);
        }
        else
        {
            // Ready to record screen
            M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), BLACK);
            M5.Display.setTextColor(WHITE);
            M5.Display.setTextSize(2);

            // Center "Ready to Record"
            const char *text1 = "Ready to Record";
            int textWidth = M5.Display.textWidth(text1);
            int x = (M5.Display.width() - textWidth) / 2;
            int y = M5.Display.height() / 2 - 30;
            M5.Display.drawString(text1, x, y);

            // Center "Press A to Start"
            const char *text2 = "Press A to Start";
            textWidth = M5.Display.textWidth(text2);
            x = (M5.Display.width() - textWidth) / 2;
            y = M5.Display.height() / 2 + 10;
            M5.Display.drawString(text2, x, y);
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
