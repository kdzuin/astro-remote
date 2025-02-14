#include <M5Unified.h>

#include <memory>

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

void init() {
    // Set initial screen
    setScreen(new MainScreen());
}

void update() {
    // Update current screen
    if (currentScreen) {
        currentScreen->update();
    }

    // Handle power button to return to main menu
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_PWR) ||
        RemoteControlManager::wasButtonPressed(ButtonId::BACK)) {
        // implement navigation tree and navigation -1 level
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

}  // namespace MenuSystem
