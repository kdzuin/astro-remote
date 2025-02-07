#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/camera_commands.h"
#include <Arduino.h> // Added required include for millis() and sprintf()
#include <string.h>  // Added required include for sprintf()

enum class PhotoMenuItem
{
    None
};

class PhotoScreen : public BaseScreen<PhotoMenuItem>
{
public:
    PhotoScreen() : BaseScreen<PhotoMenuItem>("Photo"), photoCount(0), flashStartTime(0)
    {
    }

    void drawContent() override
    {
        int centerX = M5.Display.width() / 2;
        int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

        if (millis() - flashStartTime < 200)
        {
            // Flash effect
            M5.Display.fillScreen(WHITE);
            M5.Display.setTextColor(BLACK);
        }
        else
        {
            // Normal display
            M5.Display.fillScreen(BLACK);
            M5.Display.setTextColor(WHITE);

            // Draw photo counter
            M5.Display.setTextSize(3);
            M5.Display.setTextDatum(middle_center);
            char countStr[10];
            sprintf(countStr, "%d", photoCount);
            M5.Display.drawString(countStr, centerX, centerY);
        }

        setStatusBgColor(M5.Display.color888(32, 32, 32));
        setStatusText("Ready");
        drawStatusBar();
    }

    void update() override
    {
        if (M5.BtnA.wasClicked())
        {
            if (CameraCommands::takePhoto())
            {
                photoCount++;
                flashStartTime = millis();
                draw();
            }
        }

        // Redraw if we're in flash mode to clear it
        if (millis() - flashStartTime > 200 && flashStartTime > 0)
        {
            flashStartTime = 0;
            draw();
        }
    }

private:
    int photoCount;
    unsigned long flashStartTime;
};
