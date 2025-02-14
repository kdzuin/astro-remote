#pragma once

#include "transport/camera_commands.h"
#include "utils/colors.h"

enum class FocusSensitivity : uint8_t {
    Fine = 0x10,  // First
    Medium = 0x25,
    High = 0x50,
    Coarse = 0x7F  // Last
};

class FocusProcess {
public:
    struct FocusState {
        bool focusing = false;
        FocusSensitivity sensitivity = FocusSensitivity::Medium;
    };

    static FocusState& getState() {
        static FocusState state;
        return state;
    }

    static void updateFocusState(bool newState) {
        auto& state = getState();
        state.focusing = newState;
        if (state.focusing) {
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_DOWN);
        } else {
            CameraCommands::sendCommand16(CameraCommands::Cmd::SHUTTER_HALF_UP);
        }
    }

    static bool nextSensitivity() {
        auto& state = getState();
        switch (state.sensitivity) {
            case FocusSensitivity::Fine:
                state.sensitivity = FocusSensitivity::Medium;
                return true;
            case FocusSensitivity::Medium:
                state.sensitivity = FocusSensitivity::High;
                return true;
            case FocusSensitivity::High:
                state.sensitivity = FocusSensitivity::Coarse;
                return true;
            case FocusSensitivity::Coarse:
                return false;  // Already at max
        }
        return false;
    }

    static bool prevSensitivity() {
        auto& state = getState();
        switch (state.sensitivity) {
            case FocusSensitivity::Coarse:
                state.sensitivity = FocusSensitivity::High;
                return true;
            case FocusSensitivity::High:
                state.sensitivity = FocusSensitivity::Medium;
                return true;
            case FocusSensitivity::Medium:
                state.sensitivity = FocusSensitivity::Fine;
                return true;
            case FocusSensitivity::Fine:
                return false;  // Already at min
        }
        return false;
    }

    static void cycleSensitivity() {
        if (nextSensitivity()) {
            return;
        }
        prevSensitivity();
    }

    static void handleFocus(int32_t increment) {
        auto& state = getState();
        if (!state.focusing)
            return;

        if (increment > 0) {
            CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_PRESS,
                                          static_cast<uint8_t>(state.sensitivity));
            delay(30);
            CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_OUT_RELEASE, 0x00);
        } else if (increment < 0) {
            CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_PRESS,
                                          static_cast<uint8_t>(state.sensitivity));
            delay(30);
            CameraCommands::sendCommand24(CameraCommands::Cmd::FOCUS_IN_RELEASE, 0x00);
        }
    }

    static const char* getSensitivityText() {
        switch (getState().sensitivity) {
            case FocusSensitivity::Fine:
                return "Fine";
            case FocusSensitivity::Medium:
                return "Medium";
            case FocusSensitivity::High:
                return "High";
            case FocusSensitivity::Coarse:
                return "Coarse";
            default:
                return "Unknown";
        }
    }

    static uint32_t getStatusColor() {
        return getState().focusing ? colors::get(colors::RED) : colors::get(colors::GRAY_800);
    }

    static const char* getStatusText() {
        return getState().focusing ? "Focusing" : "Ready To Focus";
    }
};
