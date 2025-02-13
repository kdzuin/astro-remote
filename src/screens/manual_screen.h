#pragma once

#include "screens/base_screen.h"
#include "components/menu_system.h"

enum class ManualMenuItem {
    HalfDown,
    FullDown,
    HalfUp,
    FullUp,
    TakePhoto,
    TakeVideo,
    TakeBulb,
    FocusIn,
    FocusOut,
    ZoomIn,
    ZoomOut,
};

class ManualScreen : public BaseScreen<ManualMenuItem> {
public:
    ManualScreen();
    void update() override;

private:
    void updateMenuItems();
    void drawContent() override;
    void selectMenuItem();
    void nextMenuItem();
    void prevMenuItem();

    size_t selectedItem = 0;
    bool isExecuting = false;
};
