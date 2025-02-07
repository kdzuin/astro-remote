#pragma once

#include <M5Unified.h>
#include "screens/base_screen.h"
#include "screens/main_screen.h"

namespace MenuSystem
{
    void init();
    void update();
    void setScreen(BaseScreen *screen);
    BaseScreen *getCurrentScreen();
}
