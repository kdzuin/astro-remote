#pragma once

#include <M5Unified.h>
#include "base_screen.h"
#include "../transport/camera_commands.h"
#include "../transport/encoder_device.h"

// Forward declarations
// class CameraCommands;

enum class FocusSensitivity : uint8_t
{
    Fine = 0x10,
    Medium = 0x25,
    High = 0x50,
    Coarse = 0x7F
};

enum class FocusMenuItem
{
    None
};

class FocusScreen : public BaseScreen<FocusMenuItem>
{
public:
    FocusScreen()
        : BaseScreen<FocusMenuItem>("Focus"), focusing(false), sensitivity(FocusSensitivity::Medium)
    {
        EncoderDevice::setMode(EncoderDevice::DEBOUNCED);
    }

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

inline void FocusScreen::drawContent()
{
    int centerX = M5.Display.width() / 2;
    int centerY = (M5.Display.height() - STATUS_BAR_HEIGHT) / 2;

    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);

    // Draw main focus status
    M5.Display.setTextSize(2);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString(focusing ? "FOCUSING" : "Ready", centerX, centerY);

    // Draw sensitivity below
    M5.Display.setTextSize(1.25);
    const char *sensText;
    switch (sensitivity)
    {
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
    M5.Display.drawString(sensText, centerX, centerY + 30);

    // Update status bar
    setStatusBgColor(focusing ? RED : M5.Display.color565(32, 32, 32));
    setStatusText(focusing ? "Focusing - Rotate to Adjust" : "Press A to Focus");
    drawStatusBar();
}

inline void FocusScreen::update()
{
    if (M5.BtnA.wasClicked() || EncoderDevice::wasClicked())
    {
        EncoderDevice::indicateClick();
        LOG_APP("[FocusScreen] Toggle focus mode");
        updateFocusState(!focusing);
        draw();
    }
    else if (M5.BtnB.wasClicked())
    {
        LOG_APP("[FocusScreen] Cycle sensitivity");
        cycleSensitivity();
        draw();
    }

    // Handle encoder rotation for focus when in focus mode
    if (focusing && EncoderDevice::hasDebouncedEvent())
    {
        int32_t rotation = EncoderDevice::getAccumulatedDelta();
        if (rotation != 0)
        {
            handleFocus(rotation);
            EncoderDevice::resetAccumulatedDelta();
        }
    }
}

inline void FocusScreen::updateFocusState(bool newState)
{
    focusing = newState;
    if (focusing)
    {
        CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_DOWN);
    }
    else
    {
        CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_UP);
    }
}

inline void FocusScreen::cycleSensitivity()
{
    switch (sensitivity)
    {
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

inline void FocusScreen::handleFocus(int32_t increment)
{
    if (!focusing)
        return;

    if (increment > 0)
    {
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_PRESS, static_cast<uint8_t>(sensitivity));
        delay(30);
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_RELEASE, 0x00);
    }
    else if (increment < 0)
    {
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_PRESS, static_cast<uint8_t>(sensitivity));
        delay(30);
        CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_RELEASE, 0x00);
    }
}
