#pragma once

#include <string>

#include "screens/base_screen.h"

enum class CameraDetailMenuItem { Connect, Disconnect, Forget };

// Per-camera submenu for a single Saved camera (glossary): connect/disconnect
// it and forget it. Reached from CameraListScreen. Holds the camera's address
// so its actions target that specific camera, not just the active one.
class CameraDetailScreen : public BaseScreen<CameraDetailMenuItem> {
public:
    explicit CameraDetailScreen(const std::string& address);
    void updateMenuItems() override;
    void drawContent() override;
    void update() override;
    void selectMenuItem() override;
    void nextMenuItem() override;
    void prevMenuItem() override;

private:
    std::string cameraAddress;
    int selectedItem = 0;
};
