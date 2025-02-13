#include "transport/remote_control_manager.h"

std::map<ButtonId, bool> RemoteControlManager::buttonStates;
std::map<ButtonId, bool> RemoteControlManager::buttonProcessed;

void RemoteControlManager::init() {
    buttonStates.clear();
    buttonProcessed.clear();

    // Initialize all buttons as released
    buttonStates[ButtonId::UP] = false;
    buttonStates[ButtonId::DOWN] = false;
    buttonStates[ButtonId::LEFT] = false;
    buttonStates[ButtonId::RIGHT] = false;
    buttonStates[ButtonId::CONFIRM] = false;
    buttonStates[ButtonId::BACK] = false;

    // Initialize all buttons as processed
    buttonProcessed[ButtonId::UP] = true;
    buttonProcessed[ButtonId::DOWN] = true;
    buttonProcessed[ButtonId::LEFT] = true;
    buttonProcessed[ButtonId::RIGHT] = true;
    buttonProcessed[ButtonId::CONFIRM] = true;
    buttonProcessed[ButtonId::BACK] = true;
}

void RemoteControlManager::update() {
    // Reset processed flags for buttons that were released
    for (auto& pair : buttonStates) {
        if (!pair.second) {  // If button is released
            buttonProcessed[pair.first] = false;
        }
    }
}

bool RemoteControlManager::wasButtonPressed(ButtonId button) {
    // Check if button is pressed and hasn't been processed
    if (buttonStates[button] && !buttonProcessed[button]) {
        buttonProcessed[button] = true;  // Mark as processed
        return true;
    }
    return false;
}

bool RemoteControlManager::isButtonPressed(ButtonId button) {
    return buttonStates[button];
}

void RemoteControlManager::setButtonState(ButtonId button, bool pressed) {
    buttonStates[button] = pressed;
    if (pressed) {
        // When a button is pressed, mark it as unprocessed so it can be detected
        buttonProcessed[button] = false;
    }
}
