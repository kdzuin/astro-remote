#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/camera_commands.h"

enum class VideoMenuItem
{
    Record,
    Back
};

class VideoScreen : public BaseScreen<VideoMenuItem>
{
public:
    VideoScreen() : BaseScreen<VideoMenuItem>("Video"), recordStartTime(0)
    {
        menuItems.setTitle("Video Menu");
        updateMenuItems();
    }

    void updateMenuItems()
    {
        menuItems.clear();
        if (CameraCommands::isRecording())
        {
            menuItems.addItem(VideoMenuItem::Record, "Stop Recording");
        }
        else
        {
            menuItems.addItem(VideoMenuItem::Record, "Start Recording");
        }
        menuItems.addItem(VideoMenuItem::Back, "Back");
    }

    void drawContent() override
    {
        int centerX = M5.Display.width() / 2;
        int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

        if (CameraCommands::isRecording())
        {
            // Tally light - full red screen with REC text
            M5.Display.fillScreen(RED);
            M5.Display.setTextColor(WHITE);

            // Center text by calculating position
            M5.Display.setTextSize(3);
            M5.Display.setTextDatum(middle_center);
            M5.Display.drawString("REC", centerX, centerY - 20);

            // Show time elapsed
            M5.Display.setTextSize(2);
            unsigned long elapsed = (millis() - recordStartTime) / 1000;
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
            // Show ready state
            M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.height(), BLACK);
            M5.Display.drawRoundRect(centerX - 20, centerY - 20, 40, 40, 4, WHITE);
            M5.Display.fillCircle(centerX, centerY, 10, RED);

            setStatusBgColor(M5.Display.color888(32, 32, 32));
            setStatusText("Ready To Record");
            drawStatusBar();
        }
    }

    void update() override
    {
        if (M5.BtnA.wasClicked() && !M5.BtnB.wasPressed())
        {
            switch (menuItems.getSelectedId())
            {
            case VideoMenuItem::Record:
                if (!CameraCommands::isRecording())
                {
                    // Start recording
                    if (CameraCommands::recordStart())
                    {
                        recordStartTime = millis();
                        updateMenuItems();
                        draw();
                    }
                }
                else
                {
                    // Stop recording
                    if (CameraCommands::recordStop())
                    {
                        recordStartTime = 0;
                        updateMenuItems();
                        draw();
                    }
                }
                break;

            case VideoMenuItem::Back:
                // Return to main menu will be handled by menu system
                break;
            }
        }

        if (M5.BtnB.wasClicked())
        {
            menuItems.selectNext();
            draw();
        }
    }

private:
    SelectableList<VideoMenuItem> menuItems;
    unsigned long recordStartTime;
};
