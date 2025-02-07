#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../menu_system.h"
#include "video_screen.h"
#include "photo_screen.h"
#include "astro_screen.h"

// Forward declare screens we'll create
class VideoScreen;
class PhotoScreen;
class AstroScreen;

enum class ControlMenuItem
{
    Video,
    Photo,
    Astro,
};

class ControlScreen : public BaseScreen<ControlMenuItem>
{
public:
    ControlScreen() : BaseScreen<ControlMenuItem>("Control")
    {
        menuItems.setTitle("Control Menu");
        updateMenuItems();
    }

    void updateMenuItems()
    {
        menuItems.clear();
        menuItems.addItem(ControlMenuItem::Video, "Video Remote");
        menuItems.addItem(ControlMenuItem::Photo, "Photo Remote");
        menuItems.addItem(ControlMenuItem::Astro, "Astro Remote");
    }

    void drawContent() override
    {
        menuItems.draw();
    }

    void update() override
    {
        if (M5.BtnA.wasClicked() && !M5.BtnB.wasPressed())
        {
            switch (menuItems.getSelectedId())
            {
            case ControlMenuItem::Video:
                MenuSystem::setScreen(new VideoScreen());
                break;

            case ControlMenuItem::Photo:
                MenuSystem::setScreen(new PhotoScreen());
                break;

            case ControlMenuItem::Astro:
                MenuSystem::setScreen(new AstroScreen());
                break;
            }
        }

        if (M5.BtnB.wasClicked())
        {
            menuItems.selectNext();
            draw();
        }
    }
};
