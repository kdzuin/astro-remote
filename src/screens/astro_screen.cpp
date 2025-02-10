#include "astro_screen.h"
#include "../transport/encoder_device.h"
#include "focus_screen.h"
#include "../components/menu_system.h"

AstroScreen::AstroScreen() : BaseScreen<AstroMenuItem>("Astro")
{
    setStatusText("Select Option");
    setStatusBgColor(M5.Display.color565(0, 0, 100));
    menuItems.setTitle("Astro Menu");
    updateMenuItems();
}

void AstroScreen::updateMenuItems()
{
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

void AstroScreen::drawContent()
{
    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void AstroScreen::update()
{
    EncoderDevice::update();
    int16_t delta = EncoderDevice::getDelta();

    if (M5.BtnB.wasClicked() || delta > 0)
    {
        LOG_DEBUG("[AstroScreen] [Encoder] Rotation: %d", delta);
        LOG_PERIPHERAL("[AstroScreen] [Encoder|Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (delta < 0)
    {
        LOG_DEBUG("[AstroScreen] [Encoder] Rotation: %d", delta);
        LOG_PERIPHERAL("[AstroScreen] [Encoder|Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (M5.BtnA.wasClicked() || EncoderDevice::wasClicked())
    {
        selectMenuItem();
    }
}

void AstroScreen::selectMenuItem()
{
    EncoderDevice::indicateClick();
    switch (menuItems.getSelectedId())
    {
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

void AstroScreen::nextMenuItem()
{
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    EncoderDevice::indicateNext();
    draw();
}

void AstroScreen::prevMenuItem()
{
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    EncoderDevice::indicatePrev();
    draw();
}
