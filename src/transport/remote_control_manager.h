#pragma once

#include <map>

#include "transport/button_id.h"

class RemoteControlManager {
private:
    static std::map<ButtonId, bool> buttonStates;
    static std::map<ButtonId, bool> buttonProcessed;

public:
    static void init();
    static void update();

    static bool wasButtonPressed(ButtonId button);
    static bool isButtonPressed(ButtonId button);

    // Convenience methods for common buttons
    static bool wasConfirmPressed() { return wasButtonPressed(ButtonId::CONFIRM); }
    static bool wasBackPressed() { return wasButtonPressed(ButtonId::BACK); }
    static bool wasUpPressed() { return wasButtonPressed(ButtonId::UP); }
    static bool wasDownPressed() { return wasButtonPressed(ButtonId::DOWN); }
    static bool wasLeftPressed() { return wasButtonPressed(ButtonId::LEFT); }
    static bool wasRightPressed() { return wasButtonPressed(ButtonId::RIGHT); }
    static bool wasAPressed() { return wasButtonPressed(ButtonId::BTN_A); }
    static bool wasBPressed() { return wasButtonPressed(ButtonId::BTN_B); }
    static bool wasPWRPressed() { return wasButtonPressed(ButtonId::BTN_PWR); }
    static bool wasEmergencyPressed() { return wasButtonPressed(ButtonId::BTN_EMERGENCY); }

    // Internal use - called by BLE notification handler
    static void setButtonState(ButtonId button, bool pressed);
};
