#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "video_screen.h"
#include "../components/selectable_list.h"

class MainScreen : public BaseScreen
{
public:
    MainScreen() : BaseScreen("Main"), menuItems("Main Menu")
    {
        menuItems.clear();
        menuItems.addItem("Connect");
        menuItems.addItem("Video");
        menuItems.addItem("Control");
        menuItems.addItem("Settings");
        setStatusText("Select Option");
        setStatusBgColor(TFT_BLUE);
    }

    void drawContent() override
    {
        menuItems.draw();
    }

    void update() override
    {
        if (M5.BtnA.wasClicked() && !M5.BtnB.wasPressed())
        {
            // Handle menu selection
            switch (menuItems.getSelectedIndex())
            {
            case 0: // Connect
                setStatusText("Connecting...");
                setStatusBgColor(TFT_YELLOW);
                drawStatusBar();
                if (BLEDeviceManager::connectToSavedDevice())
                {
                    setStatusText("Connected");
                    setStatusBgColor(M5.Display.color888(0, 200, 0));
                    drawStatusBar();
                }
                else
                {
                    setStatusText("Connection failed");
                    setStatusBgColor(M5.Display.color888(200, 0, 0));
                    drawStatusBar();
                };
                break;
            case 1: // Video
                if (onScreenChange)
                    onScreenChange(new VideoScreen());
                break;
            }
        }

        if (M5.BtnPWR.wasClicked())
        {
            // Nothing to do on main screen - can't go back
        }

        if (M5.BtnB.wasClicked())
        {
            menuItems.selectNext(); // Down
            draw();                 // Refresh the entire screen
        }
    }

    void setScreenChangeCallback(std::function<void(BaseScreen *)> callback)
    {
        onScreenChange = callback;
    }

private:
    SelectableList menuItems;
    std::function<void(BaseScreen *)> onScreenChange;
};
