#include "manual_screen.h"

#include "components/menu_system.h"
#include "processes/manual.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

ManualScreen::ManualScreen() : BaseScreen("Manual"), isExecuting(false) {
    auto state = ManualProcess::getState();
    setStatusText(ManualProcess::getStatusText(state.status));
    setStatusBgColor(ManualProcess::getStatusColor(state.status));
    updateMenuItems();
}

void ManualScreen::update() {
    if (!isExecuting) {
        if (M5.BtnB.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
            LOG_PERIPHERAL("[ManualScreen] [Btn] Next Button Clicked");
            nextMenuItem();
        }

        if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
            LOG_PERIPHERAL("[ManualScreen] [Btn] Prev Button Clicked");
            prevMenuItem();
        }

        if (M5.BtnA.wasClicked() || RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
            LOG_PERIPHERAL("[ManualScreen] [Btn] Confirm Button Clicked");
            selectMenuItem();
        }
    }
}

void ManualScreen::updateMenuItems() {
    menuItems.clear();
    menuItems.addItem(ManualMenuItem::TakePhoto, "Photo Action");
    menuItems.addItem(ManualMenuItem::TakeVideo, "Video Action");
    menuItems.addItem(ManualMenuItem::TakeBulb, "Bulb Action");
    menuItems.addSeparator();
    menuItems.addItem(ManualMenuItem::HalfDown, "Half Down");
    menuItems.addItem(ManualMenuItem::FullDown, "Full Down");
    menuItems.addItem(ManualMenuItem::HalfUp, "Half Up");
    menuItems.addItem(ManualMenuItem::FullUp, "Full Up");
    menuItems.addItem(ManualMenuItem::FocusIn, "Focus In");
    menuItems.addItem(ManualMenuItem::FocusOut, "Focus Out");
    menuItems.addItem(ManualMenuItem::ZoomIn, "Zoom In");
    menuItems.addItem(ManualMenuItem::ZoomOut, "Zoom Out");
}

void ManualScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void ManualScreen::selectMenuItem() {
    auto command = menuItems.getSelectedId();
    isExecuting = true;
    setStatusText(ManualProcess::getStatusText(ManualProcess::Status::Executing));
    setStatusBgColor(ManualProcess::getStatusColor(ManualProcess::Status::Executing));
    draw();

    if (ManualProcess::executeCommand(command)) {
        delay(100);  // Brief delay to show execution
    }

    isExecuting = false;
    auto state = ManualProcess::getState();
    setStatusText(ManualProcess::getStatusText(state.status));
    setStatusBgColor(ManualProcess::getStatusColor(state.status));
    draw();
}

void ManualScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void ManualScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}
