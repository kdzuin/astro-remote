#pragma once

#include <M5Unified.h>
#include "libraries/FairyEncoder/FairyEncoder.h"

class EncoderDevice
{
public:
    enum Mode
    {
        DEBOUNCED,
        LIVE
    };

    static void init();
    static void update();
    static bool isAvailable();

    // Mode control
    static void setMode(Mode newMode);
    static Mode getMode();
    static bool isDebouncedMode();
    static bool hasDebouncedEvent();

    // Rotation handling
    static int16_t getDelta();
    static int32_t getValue();
    static int16_t getAccumulatedDelta();
    static void resetAccumulatedDelta();

    // LED feedback
    static void indicateNext();
    static void indicatePrev();
    static void indicateClick();

    // Button handling
    static bool isButtonPressed();
    static bool wasClicked();
    static bool wasLongClicked();

    // LED control
    static void setLED(uint32_t color);

private:
    static FairyEncoder encoder;
    static bool available;
    static int32_t value;
    static bool lastButtonState;
    static int16_t accumulatedDelta;
    static unsigned long lastActivityTime;
    static Mode currentMode;
    static bool buttonPressed;
    static bool prevButtonPressed;
    static unsigned long lastButtonChangeTime;
    static const unsigned long BUTTON_DEBOUNCE = 50; // ms
    static unsigned long pressStartTime;
    static bool clickPending;
    static bool longClickPending;
    static const unsigned long LONG_PRESS_DURATION = 600; // 1 second

    // Constants
    static const uint8_t PIN_SDA = 32;               // G32
    static const uint8_t PIN_SCL = 33;               // G33
    static const uint32_t I2C_FREQ = 100000;         // 100kHz
    static const unsigned long DEBOUNCE_DELAY = 300; // ms
};
