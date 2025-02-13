#pragma once

#include "screens/base_screen.h"
#include "transport/camera_commands.h"
#include "transport/remote_control_manager.h"

enum class FocusSensitivity : uint8_t { Fine = 0x10, Medium = 0x25, High = 0x50, Coarse = 0x7F };

enum class FocusMenuItem { None };

class FocusScreen : public BaseScreen<FocusMenuItem> {
public:
    FocusScreen()
        : BaseScreen<FocusMenuItem>("Focus"),
          focusing(false),
          sensitivity(FocusSensitivity::Medium) {}

    void drawContent() override;
    void update() override;
    void updateMenuItems() override {}
    void selectMenuItem() override {}
    void nextMenuItem() override {}
    void prevMenuItem() override {}

private:
    bool focusing;
    FocusSensitivity sensitivity;

    void updateFocusState(bool newState);
    void cycleSensitivity();
    void handleFocus(int32_t increment);
};

inline void FocusScreen::drawContent() {
    int centerX = display().width() / 2;
    int centerY = (display().height() - STATUS_BAR_HEIGHT) / 2;

    display().fillScreen(BLACK);
    display().setTextColor(WHITE);

    // Draw main focus status
    display().setTextSize(2);
    display().setTextAlignment(textAlign::middle_center);
    display().drawString(focusing ? "FOCUSING" : "Ready", centerX, centerY);

    // Draw sensitivity below
    display().setTextSize(1.25);
    const char* sensText;
    switch (sensitivity) {
        case FocusSensitivity::Fine:
            sensText = "Fine";
            break;
        case FocusSensitivity::Medium:
            sensText = "Medium";
            break;
        case FocusSensitivity::High:
            sensText = "High";
            break;
        case FocusSensitivity::Coarse:
            sensText = "Coarse";
            break;
    }
    display().drawString(sensText, centerX, centerY + 30);

    // Update status bar
    setStatusBgColor(focusing ? display().getColor(display::colors::RED)
                              : display().getColor(display::colors::GRAY_800));
    setStatusText(focusing ? "Focusing" : "Ready To Focus");
    drawStatusBar();
}

inline void FocusScreen::update() {
    if (input().wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_APP("[FocusScreen] Toggle focus mode");
        updateFocusState(!focusing);
        draw();
    } else if (input().wasButtonPressed(ButtonId::BTN_B) ||
               RemoteControlManager::wasButtonPressed(ButtonId::DOWN) ||
               RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_APP("[FocusScreen] Cycle sensitivity");
        cycleSensitivity();
        draw();
    }

    if (focusing) {
        if (RemoteControlManager::wasButtonPressed(ButtonId::LEFT)) {
            handleFocus(1);
        } else if (RemoteControlManager::wasButtonPressed(ButtonId::RIGHT)) {
            handleFocus(-1);
        }
    }
}

inline void FocusScreen::updateFocusState(bool newState) {
    focusing = newState;
    if (focusing) {
        CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_DOWN);
    } else {
        CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_UP);
    }
}

inline void FocusScreen::cycleSensitivity() {
    switch (sensitivity) {
        case FocusSensitivity::Fine:
            sensitivity = FocusSensitivity::Medium;
            break;
        case FocusSensitivity::Medium:
            sensitivity = FocusSensitivity::High;
            break;
        case FocusSensitivity::High:
            sensitivity = FocusSensitivity::Coarse;
            break;
        case FocusSensitivity::Coarse:
            sensitivity = FocusSensitivity::Fine;
            break;
    }
}

inline void FocusScreen::handleFocus(int32_t increment) {
    if (!focusing)
        return;

    if (increment > 0) {
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_PRESS,
                                      static_cast<uint8_t>(sensitivity));
        delay(30);
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_RELEASE, 0x00);
    } else if (increment < 0) {
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_PRESS,
                                      static_cast<uint8_t>(sensitivity));
        delay(30);
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_RELEASE, 0x00);
    }
}
