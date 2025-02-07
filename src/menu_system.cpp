#include "menu_system.h"
#include "transport/ble_device.h"
#include <memory>

namespace MenuSystem
{
    namespace
    {
        std::unique_ptr<BaseScreen> currentScreen;
        std::unique_ptr<MainScreen> mainScreen;
    }

    void init()
    {
        // Create main screen
        mainScreen.reset(new MainScreen());

        // Set up screen change callback
        mainScreen->setScreenChangeCallback([](BaseScreen *newScreen)
                                            { setScreen(newScreen); });

        // Set initial screen
        currentScreen.reset(new MainScreen(*mainScreen));
        currentScreen->draw();
    }

    void update()
    {
        // Update M5 buttons
        M5.update();

        // Handle power button to return to main menu
        if (M5.BtnPWR.wasClicked())
        {
            if (currentScreen.get() != mainScreen.get())
            {
                currentScreen.reset(new MainScreen(*mainScreen));
                currentScreen->draw();
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
        if (screen)
        {
            currentScreen.reset(screen);
            currentScreen->draw();
        }
    }

    BaseScreen *getCurrentScreen()
    {
        return currentScreen.get();
    }
}
