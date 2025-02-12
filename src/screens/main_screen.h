#pragma once

#include "components/menu_system.h"
#include "screens/astro_screen.h"
#include "screens/base_screen.h"
#include "screens/photo_screen.h"
#include "screens/settings_screen.h"
#include "screens/video_screen.h"
#include "transport/ble_device.h"

// Forward declare screens we'll create
class SettingsScreen;
class PhotoScreen;
class VideoScreen;
class AstroScreen;

enum class MainMenuItem { Connect, Video, Photo, Astro, Manual, Settings };

class MainScreen : public BaseScreen<MainMenuItem> {
public:
    MainScreen();
    void update() override;
    void updateMenuItems() override;
    void drawContent() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;
};
