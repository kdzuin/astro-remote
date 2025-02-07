#include "menu_system.h"
#include "transport/ble_device.h"
#include <memory>

namespace MenuSystem
{
    namespace
    {
        std::unique_ptr<BaseScreen> currentScreen;
    }

    void init()
    {
        // Set initial screen
        currentScreen.reset(new MainScreen());

        // Set up screen change callback
        static_cast<MainScreen *>(currentScreen.get())->setScreenChangeCallback([](BaseScreen *newScreen)
                                                                                { setScreen(newScreen); });

        currentScreen->draw();
    }

    void update()
    {
        // Update M5 buttons
        M5.update();

        // Handle power button to return to main menu
        if (M5.BtnPWR.wasClicked())
        {
            // Only reinitialize if we're not already on main screen
            if (strcmp(currentScreen->getName(), "Main") != 0)
            {
                // Call beforeExit on current screen
                if (currentScreen)
                {
                    currentScreen->beforeExit();
                }

                // Create fresh main screen
                init();
            }
        }

        // Update current screen
        if (currentScreen)
        {
            currentScreen->update();
        }

        // Update BLE connection
        BLEDeviceManager::update();
    }

    void setScreen(BaseScreen *screen)
    {
        if (!screen)
            return;

        // Call beforeExit on current screen
        if (currentScreen)
        {
            currentScreen->beforeExit();
        }

        // Take ownership of new screen
        currentScreen.reset(screen);

        // Set up screen change callback if it's a main screen
        if (strcmp(screen->getName(), "Main") == 0)
        {
            static_cast<MainScreen *>(currentScreen.get())->setScreenChangeCallback([](BaseScreen *newScreen)
                                                                                    { setScreen(newScreen); });
        }

        currentScreen->draw();
    }

    BaseScreen *getCurrentScreen()
    {
        return currentScreen.get();
    }
}
