#include "manual_screen.h"

#include "../components/menu_system.h"
#include "../transport/camera_commands.h"
#include "../transport/remote_control_manager.h"

ManualScreen::ManualScreen() : BaseScreen("Manual") {
    setStatusText("Manual Control");
    setStatusBgColor(M5.Display.color565(0, 0, 100));
    updateMenuItems();
}

void ManualScreen::update() {
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

void ManualScreen::updateMenuItems() {
    menuItems.clear();
    menuItems.addItem(ManualMenuItem::HalfDown, "Half Down");
    menuItems.addItem(ManualMenuItem::FullDown, "Full Down");
    menuItems.addItem(ManualMenuItem::HalfUp, "Half Up");
    menuItems.addItem(ManualMenuItem::FullUp, "Full Up");
    menuItems.addItem(ManualMenuItem::TakePhoto, "Photo Action");
    menuItems.addItem(ManualMenuItem::TakeVideo, "Video Action");
    menuItems.addItem(ManualMenuItem::TakeBulb, "Bulb Action");
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
    switch (menuItems.getSelectedId()) {
        case ManualMenuItem::HalfDown:
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_DOWN);
            break;
        case ManualMenuItem::FullDown:
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_FULL_DOWN);
            break;
        case ManualMenuItem::HalfUp:
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_UP);
            break;
        case ManualMenuItem::FullUp:
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_FULL_UP);
            break;
        case ManualMenuItem::TakePhoto:
            CameraCommands::takePhoto();
            break;
        case ManualMenuItem::TakeVideo:
            CameraCommands::recordStart();
            break;
        case ManualMenuItem::TakeBulb:
            CameraCommands::takeBulb();
            break;
        case ManualMenuItem::ZoomIn:
            CameraCommands::zoomIn(0x70);
            break;
        case ManualMenuItem::ZoomOut:
            CameraCommands::zoomOut(0x70);
            break;
        case ManualMenuItem::FocusIn:
            CameraCommands::focusIn(0x10);
            break;
        case ManualMenuItem::FocusOut:
            CameraCommands::focusOut(0x10);
            break;
    }
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
