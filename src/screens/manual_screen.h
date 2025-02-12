#pragma once

#include "base_screen.h"

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
    void updateMenuItems() override;
    void drawContent() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    int selectedItem = 0;
};
