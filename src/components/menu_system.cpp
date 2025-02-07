#include "menu_system.h"
#include "../screens/main_screen.h"
#include "../screens/video_screen.h"
#include "../screens/photo_screen.h"
#include "../screens/astro_screen.h"
#include "../screens/settings_screen.h"
#include <M5Unified.h>

namespace MenuSystem
{
    // Current screen being displayed
    static std::unique_ptr<IScreen> currentScreen;

    void init()
    {
        // Set initial screen
        setScreen(new MainScreen());
    }

    void update()
    {
        // Update buttons
        M5.update();

        // Update current screen
        if (currentScreen)
        {
            currentScreen->update();
        }

        // Handle power button to return to main menu
        if (M5.BtnPWR.wasClicked())
        {
            if (strcmp(currentScreen->getName(), "Main") != 0)
            {
                setScreen(new MainScreen());
            }
        }
    }

    void setScreenInternal(IScreen *screen)
    {
        if (!screen)
            return;

        // Store the screen
        currentScreen.reset(screen);

        // Draw the new screen
        screen->draw();
    }

    IScreen *getCurrentScreen()
    {
        return currentScreen.get();
    }
}
