#include "manual_screen.h"
#include "../components/menu_system.h"
#include "../transport/camera_commands.h"
#include "../transport/encoder_device.h"

ManualScreen::ManualScreen() : BaseScreen("Manual")
{
    setStatusText("Manual Control");
    setStatusBgColor(M5.Display.color565(0, 0, 100));
    updateMenuItems();
}

void ManualScreen::update()
{
    EncoderDevice::update();

    int16_t delta = EncoderDevice::getDelta();
    if (delta > 0 || M5.BtnB.wasClicked())
    {
        nextMenuItem();
    }

    if (M5.BtnA.wasClicked() || EncoderDevice::wasClicked())
    {
        selectMenuItem();
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
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void ManualScreen::selectMenuItem()
{
    EncoderDevice::indicateClick();

    switch (menuItems.getSelectedId())
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

void ManualScreen::nextMenuItem()
{
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    EncoderDevice::indicateNext();
    draw();
}

void ManualScreen::prevMenuItem()
{
    // menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    EncoderDevice::indicatePrev();
    draw();
}
