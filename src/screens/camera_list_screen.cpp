#include "screens/camera_list_screen.h"

#include "components/menu_system.h"
#include "screens/camera_detail_screen.h"
#include "screens/scan_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

CameraListScreen::CameraListScreen() : BaseScreen<std::string>("Cameras") {
    setStatusText("Select Camera");
    setStatusBgColor(colors::get(colors::NORMAL));
    menuItems.setTitle("Cameras");
    updateMenuItems();
}

void CameraListScreen::updateMenuItems() {
    menuItems.clear();

    const std::string& activeAddr = BLEDeviceManager::getActiveCameraAddress();
    bool connected = BLEDeviceManager::isConnected();

    for (const auto& cam : BLEDeviceManager::getSavedCameras()) {
        std::string label = cam.name.empty() ? cam.address : cam.name;

        // Marker: '*' = connected, 'o' = active but not connected, blank
        // otherwise. ASCII because the M5 font lacks the circle glyphs
        // (U+25CF/U+25CB rendered as tofu on-device).
        std::string marker = " ";
        if (cam.address == activeAddr) {
            marker = connected ? "*" : "o";
        }
        menuItems.addItem(cam.address, label, marker, true);
    }

    menuItems.addSeparator();
    menuItems.addItem(SCAN_NEW_ID, "Scan New");
}

void CameraListScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void CameraListScreen::update() {
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
        RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        prevMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        selectMenuItem();
    }
}

void CameraListScreen::selectMenuItem() {
    std::string id = menuItems.getSelectedId();
    if (id == SCAN_NEW_ID) {
        MenuSystem::setScreen(new ScanScreen());
        return;
    }
    // A saved camera: open its detail submenu. setScreen deletes this screen;
    // return immediately after.
    MenuSystem::setScreen(new CameraDetailScreen(id));
}

void CameraListScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void CameraListScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}
