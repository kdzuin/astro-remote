#pragma once

#include <M5Unified.h>
#include "base_screen.h"

enum class AstroMenuItem
{
    None
};

class AstroScreen : public BaseScreen<AstroMenuItem>
{
public:
    AstroScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
};
