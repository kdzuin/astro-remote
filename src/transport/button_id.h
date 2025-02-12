#pragma once

// Button IDs
enum class ButtonId : uint8_t
{
    // Remote control buttons
    UP = 0x01,
    DOWN = 0x02,
    LEFT = 0x03,
    RIGHT = 0x04,
    CONFIRM = 0x05,
    BACK = 0x06,

    // Hardware buttons
    BTN_A = 0x10,
    BTN_B = 0x11,
    BTN_PWR = 0x12
};
