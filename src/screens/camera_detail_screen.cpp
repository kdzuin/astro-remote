#include "screens/camera_detail_screen.h"

#include "components/menu_system.h"
#include "screens/camera_list_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

CameraDetailScreen::CameraDetailScreen(const std::string& address)
    : BaseScreen<CameraDetailMenuItem>("Camera"), cameraAddress(address) {
    setStatusText("Select Option");
    setStatusBgColor(colors::get(colors::NORMAL));

    const SavedCamera* cam = nullptr;
    for (const auto& c : BLEDeviceManager::getSavedCameras()) {
        if (c.address == cameraAddress) {
            cam = &c;
            break;
        }
    }
    std::string title = (cam && !cam->name.empty()) ? cam->name : cameraAddress;
    menuItems.setTitle(title);
    updateMenuItems();
}

void CameraDetailScreen::updateMenuItems() {
    menuItems.clear();

    bool isActive = (BLEDeviceManager::getActiveCameraAddress() == cameraAddress);
    bool isConnected = isActive && BLEDeviceManager::isConnected();

    if (isConnected) {
        menuItems.addItem(CameraDetailMenuItem::Disconnect, "Disconnect");
    } else {
        menuItems.addItem(CameraDetailMenuItem::Connect, "Connect");
    }
    menuItems.addItem(CameraDetailMenuItem::Forget, "Forget");
}

void CameraDetailScreen::drawContent() {
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void CameraDetailScreen::update() {
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

void CameraDetailScreen::selectMenuItem() {
    switch (menuItems.getSelectedId()) {
        case CameraDetailMenuItem::Connect:
            setStatusText("Connecting...");
            setStatusBgColor(colors::get(colors::NORMAL));
            draw();
            if (BLEDeviceManager::connectToAddress(cameraAddress)) {
                setStatusText("Connected!");
                setStatusBgColor(colors::get(colors::SUCCESS));
            } else {
                setStatusText("Failed to connect!");
                setStatusBgColor(colors::get(colors::ERROR));
            }
            updateMenuItems();
            draw();
            break;

        case CameraDetailMenuItem::Disconnect:
            BLEDeviceManager::disconnect();
            setStatusText("Select Option");
            setStatusBgColor(colors::get(colors::NORMAL));
            updateMenuItems();
            draw();
            break;

        case CameraDetailMenuItem::Forget:
            // Deletes this camera; return to the list. Do not touch any member
            // after navigating away — setScreen deletes this screen.
            BLEDeviceManager::forgetCamera(cameraAddress);
            MenuSystem::setScreen(new CameraListScreen());
            return;
    }
}

void CameraDetailScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void CameraDetailScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}
