#include "astro_screen.h"

AstroScreen::AstroScreen() : BaseScreen<AstroMenuItem>("Astro")
{
    menuItems.setTitle("Astro Menu");
    updateMenuItems();
}

void AstroScreen::updateMenuItems()
{
    menuItems.clear();
}

void AstroScreen::drawContent()
{
    setStatusBgColor(M5.Display.color888(32, 32, 32));
    setStatusText("Astro");
    drawStatusBar();
}

void AstroScreen::update()
{
}
