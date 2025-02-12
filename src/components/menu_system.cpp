#include <memory>

#include "hardware_interface.h"
#include "screens/astro_screen.h"
#include "screens/base_screen.h"
#include "screens/main_screen.h"
#include "screens/photo_screen.h"
#include "screens/settings_screen.h"
#include "screens/video_screen.h"
#include "transport/remote_control_manager.h"

namespace MenuSystem {
// Current screen being displayed
static std::unique_ptr<IScreen> currentScreen;
static IHardware* hardware = nullptr;

void init(IHardware* hw) {
    hardware = hw;
    // Set initial screen
    setScreen(new MainScreen());
}

void update() {
    if (!hardware)
        return;

    // Update current screen
    if (currentScreen) {
        currentScreen->update();
        // currentScreen->draw();  // Make sure screen is drawn every frame
    }

    // Handle power button to return to main menu
    auto& input = hardware->getInput();
    if (input.wasButtonPressed(ButtonId::BTN_PWR) ||
        RemoteControlManager::wasButtonPressed(ButtonId::BACK)) {
        if (strcmp(currentScreen->getName(), "Main") != 0) {
            goHome();
        }
    }
}

void goHome() {
    setScreen(new MainScreen());
}

void setScreenInternal(IScreen* screen) {
    if (!screen)
        return;
    currentScreen.reset(screen);
    screen->draw();  // Draw the new screen immediately
}

IScreen* getCurrentScreen() {
    return currentScreen.get();
}

IHardware* getHardware() {
    return hardware;
}
}  // namespace MenuSystem
