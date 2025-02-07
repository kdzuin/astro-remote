#include "manual_screen.h"
#include "../components/menu_system.h"
#include "../transport/camera_commands.h"

ManualScreen::ManualScreen() : BaseScreen("Manual")
{
    setStatusText("Manual Control");
    setStatusBgColor(M5.Display.color888(0, 0, 100));
    updateMenuItems();
}

void ManualScreen::update()
{
    if (M5.BtnA.wasClicked())
    {
        auto selectedItem = menuItems.getSelectedId();
        switch (selectedItem)
        {
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
        }
    }

    if (M5.BtnB.wasClicked())
    {
        menuItems.selectNext();
        draw();
    }
}

void ManualScreen::updateMenuItems()
{
    menuItems.clear();
    menuItems.addItem(ManualMenuItem::HalfDown, "Half Down");
    menuItems.addItem(ManualMenuItem::FullDown, "Full Down");
    menuItems.addItem(ManualMenuItem::HalfUp, "Half Up");
    menuItems.addItem(ManualMenuItem::FullUp, "Full Up");
    menuItems.addItem(ManualMenuItem::TakePhoto, "Photo Action");
    menuItems.addItem(ManualMenuItem::TakeVideo, "Video Action");
    menuItems.addItem(ManualMenuItem::TakeBulb, "Bulb Action");
}

void ManualScreen::drawContent()
{
    menuItems.draw();
}
