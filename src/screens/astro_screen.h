#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../menu_system.h"
#include "control_screen.h"

enum class AstroMenuItem
{
    None
};

class AstroScreen : public BaseScreen<AstroMenuItem>
{
public:
    AstroScreen() : BaseScreen<AstroMenuItem>("Astro")
    {
        menuItems.setTitle("Astro Menu");
        updateMenuItems();
    }

    void updateMenuItems()
    {
        menuItems.clear();
    }

    void drawContent() override
    {
        setStatusBgColor(M5.Display.color888(32, 32, 32));
        setStatusText("Astro");
        drawStatusBar();
    }

    void update() override
    {
    }

private:
    SelectableList<AstroMenuItem> menuItems;
};
