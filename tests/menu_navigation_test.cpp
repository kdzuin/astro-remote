#include <M5Unified.h>
#include "transport/encoder_device.h"

// Global variables for encoder state
int32_t lastEncoderValue = 0;

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("Encoder behavior model 1 (menu navigation)");

    // Initialize encoder
    EncoderDevice::init();
}

void handleRotation()
{
    if (!EncoderDevice::isAvailable())
        return;

    EncoderDevice::update();

    int16_t delta = EncoderDevice::getDelta();

    // For menu navigation, treat any movement as single steps
    int16_t steps = (delta > 0) ? 1 : -1;

    Serial.print("Menu navigation: ");
    Serial.println(steps);

    // Visual feedback
    EncoderDevice::setLED(steps > 0 ? 0x00FF00 : 0x0000FF);
    delay(50); // Brief LED feedback
    EncoderDevice::setLED(0x000000);
}

void handleButton()
{
    static bool lastButtonState = false;
    bool buttonState = EncoderDevice::isButtonPressed();

    if (buttonState != lastButtonState)
    {
        if (buttonState)
        {
            Serial.println("Button pressed");
            EncoderDevice::setLED(0xFF0000); // Red when button pressed
        }
        else
        {
            Serial.println("Button released");
            EncoderDevice::setLED(0x000000); // LED off
        }
        lastButtonState = buttonState;
    }
}

void loop()
{
    if (EncoderDevice::isAvailable())
    {
        EncoderDevice::update();
        handleRotation();
        handleButton();
    }

    delay(10); // Small delay to prevent tight loop
}
