
#include "screens/astro_screen.h"

#include "components/menu_system.h"
#include "screens/focus_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

AstroScreen::AstroScreen() : BaseScreen<AstroMenuItem>("Astro") {
    setStatusText("Select Option");
    setStatusBgColor(colors::get(colors::GRAY_800));
    menuItems.setTitle("Astro Menu");
    updateMenuItems();
}

void AstroScreen::updateMenuItems() {
    menuItems.clear();
    menuItems.addItem(AstroMenuItem::Focus, "Focus");
    menuItems.addItem(AstroMenuItem::InitialDelay, "Delay");
    menuItems.addItem(AstroMenuItem::ExposureTime, "Exp");
    menuItems.addItem(AstroMenuItem::NumberOfExposures, "Subs");
    menuItems.addItem(AstroMenuItem::DelayBetweenExposures, "Interval");
    menuItems.addItem(AstroMenuItem::Start, "Start");
    menuItems.addItem(AstroMenuItem::Pause, "Pause");
    menuItems.addItem(AstroMenuItem::Stop, "Stop");
}

void AstroScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void AstroScreen::update() {
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
        RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Confirm Button Clicked");
        selectMenuItem();
    }
}

void AstroScreen::selectMenuItem() {
    switch (menuItems.getSelectedId()) {
        case AstroMenuItem::Focus:
            LOG_PERIPHERAL("[AstroScreen] Select Focus");
            MenuSystem::setScreen(new FocusScreen());
            break;
        case AstroMenuItem::InitialDelay:
            LOG_PERIPHERAL("[AstroScreen] Select Initial Delay");
            break;
        case AstroMenuItem::ExposureTime:
            LOG_PERIPHERAL("[AstroScreen] Select Exposure Time");
            break;
        case AstroMenuItem::NumberOfExposures:
            LOG_PERIPHERAL("[AstroScreen] Select Number of Exposures");
            break;
        case AstroMenuItem::DelayBetweenExposures:
            LOG_PERIPHERAL("[AstroScreen] Select Delay Between Exposures");
            break;
        case AstroMenuItem::Start:
            LOG_PERIPHERAL("[AstroScreen] Start");
            break;
        case AstroMenuItem::Pause:
            LOG_PERIPHERAL("[AstroScreen] Pause");
            break;
        case AstroMenuItem::Stop:
            LOG_PERIPHERAL("[AstroScreen] Stop");
            break;
    }
}

void AstroScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void AstroScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}
