#include "astro_screen.h"

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
    if (M5.BtnB.wasClicked())
    {
        menuItems.selectNext();
        selectedItem = menuItems.getSelectedIndex();
        draw();
    }
}
