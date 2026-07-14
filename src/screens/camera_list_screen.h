#pragma once

#include <string>

#include "screens/base_screen.h"

// The Camera list (glossary): every Saved camera plus a "Scan New" action.
// Rows are keyed by BLE address; the sentinel below is the "Scan New" row's id
// (safe — real MAC addresses never contain underscores). Selecting a saved
// camera opens its CameraDetailScreen; the active camera is marked in the info
// column (filled = connected, outline = active-but-disconnected).
class CameraListScreen : public BaseScreen<std::string> {
public:
    static constexpr const char* SCAN_NEW_ID = "__scan_new__";

    CameraListScreen();
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    int selectedItem = 0;
};
