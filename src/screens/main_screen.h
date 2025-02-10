#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/ble_device.h"
#include "../components/menu_system.h"
#include "settings_screen.h"
#include "photo_screen.h"
#include "video_screen.h"
#include "astro_screen.h"

// Forward declare screens we'll create
class SettingsScreen;
class PhotoScreen;
class VideoScreen;
class AstroScreen;

enum class MainMenuItem
{
    Connect,
    Video,
    Photo,
    Astro,
    Manual,
    Settings
};

class MainScreen : public BaseScreen<MainMenuItem>
{
public:
    MainScreen();
    void update() override;
    void updateMenuItems() override;
    void drawContent() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;
};
