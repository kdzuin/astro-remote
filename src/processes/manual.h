#pragma once

#include "components/menu_system.h"
#include "transport/camera_commands.h"
#include "utils/display_constants.h"

class ManualProcess {
public:
    enum class Status { Normal, Executing };

    struct ManualState {
        Status status;
        bool isExecuting;
    };

    static ManualState getState() {
        return ManualState{Status::Normal, false};
    }

    static bool executeCommand(ManualMenuItem command) {
        switch (command) {
            case ManualMenuItem::HalfDown:
                return CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_DOWN);
            case ManualMenuItem::FullDown:
                return CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_FULL_DOWN);
            case ManualMenuItem::HalfUp:
                return CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_UP);
            case ManualMenuItem::FullUp:
                return CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_FULL_UP);
            case ManualMenuItem::TakePhoto:
                return CameraCommands::takePhoto();
            case ManualMenuItem::TakeVideo:
                return CameraCommands::recordStart();
            case ManualMenuItem::TakeBulb:
                return CameraCommands::takeBulb();
            case ManualMenuItem::ZoomIn:
                return CameraCommands::zoomIn(0x70);
            case ManualMenuItem::ZoomOut:
                return CameraCommands::zoomOut(0x70);
            case ManualMenuItem::FocusIn:
                return CameraCommands::focusIn(0x10);
            case ManualMenuItem::FocusOut:
                return CameraCommands::focusOut(0x10);
        }
        return false;
    }

    static uint16_t getStatusColor(Status status) {
        auto& display = MenuSystem::getHardware()->getDisplay();
        switch (status) {
            case Status::Executing:
                return display.getColor(display::colors::YELLOW);
            case Status::Normal:
                return display.getColor(display::colors::GRAY_800);
        }
        return display.getColor(display::colors::GRAY_800);
    }

    static const char* getStatusText(Status status) {
        switch (status) {
            case Status::Executing:
                return "Executing...";
            case Status::Normal:
                return "Manual Control";
        }
        return "";
    }
};
