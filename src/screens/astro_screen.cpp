#include "astro_screen.h"
#include "../transport/encoder_device.h"

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
