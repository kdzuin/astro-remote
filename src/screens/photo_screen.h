#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/camera_commands.h"
#include <Arduino.h> // Added required include for millis() and sprintf()
#include <string.h>  // Added required include for sprintf()

enum class PhotoMenuItem
{
    None
};

class PhotoScreen : public BaseScreen<PhotoMenuItem>
{
public:
    PhotoScreen();
    void drawContent() override;
    void update() override;
    void updateMenuItems() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    int photoCount;
    unsigned long flashStartTime;
};
