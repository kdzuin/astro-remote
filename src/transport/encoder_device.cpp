#include "encoder_device.h"
#include "../debug.h"

// Static member initialization
FairyEncoder EncoderDevice::encoder;
bool EncoderDevice::available = false;
int32_t EncoderDevice::value = 0;
bool EncoderDevice::lastButtonState = false;
int16_t EncoderDevice::accumulatedDelta = 0;
unsigned long EncoderDevice::lastActivityTime = 0;
EncoderDevice::Mode EncoderDevice::currentMode = EncoderDevice::DEBOUNCED;
bool EncoderDevice::buttonPressed = false;
bool EncoderDevice::prevButtonPressed = false;
unsigned long EncoderDevice::lastButtonChangeTime = 0;
bool EncoderDevice::clickPending = false;
bool EncoderDevice::longClickPending = false;
unsigned long EncoderDevice::pressStartTime = 0;

void EncoderDevice::init()
{
    LOG_PERIPHERAL("[Encoder] Starting I2C...");
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(I2C_FREQ);
    delay(100); // Give I2C time to stabilize

    LOG_PERIPHERAL("[Encoder] Initializing encoder...");
    encoder.begin();
    delay(100); // Give encoder time to initialize

    // Initial read to clear any pending data
    encoder.task();
    encoder.read(true);

    // Check if encoder is available
    available = encoder.avail();
    LOG_PERIPHERAL("[Encoder] Encoder available: %s", available ? "YES" : "NO");

    if (available)
    {
        LOG_PERIPHERAL("[Encoder] Encoder initialized successfully");
        setLED(0x000000); // LED off initially

        // Initialize button state (inverted: true = pressed, false = released)
        lastButtonState = !encoder.getButtonStatus();
        LOG_DEBUG("[Encoder] Initial button state: %s", lastButtonState ? "PRESSED" : "RELEASED");
    }
    else
    {
        LOG_PERIPHERAL("[Encoder] Encoder not found - scanning I2C bus...");
        for (byte i = 8; i < 120; i++)
        {
            Wire.beginTransmission(i);
            byte error = Wire.endTransmission();
            if (error == 0)
            {
                LOG_PERIPHERAL("[Encoder] I2C device found at address 0x%02X", i);
            }
        }
    }
}

void EncoderDevice::update()
{
    if (!available)
        return;

    encoder.task();

    // Handle rotation
    int16_t delta = encoder.read(true);
    if (delta != 0)
    {
        accumulatedDelta += delta;
        lastActivityTime = millis();
        LOG_DEBUG("[Encoder] Encoder delta: %d", delta);
    }

    // Button state tracking
    bool buttonRawState = !encoder.getButtonStatus();
    static bool lastRawState = buttonRawState;

    if (buttonRawState != lastRawState)
    {
        if (millis() - lastButtonChangeTime > DEBOUNCE_DELAY_CLICK)
        {
            LOG_DEBUG("[Encoder] State change: %s at %lu",
                      buttonRawState ? "PRESSED" : "RELEASED",
                      millis());

            if (buttonRawState)
            { // Press
                pressStartTime = millis();
                buttonPressed = true;
                prevButtonPressed = false;
            }
            else
            { // Release
                unsigned long pressDuration = millis() - pressStartTime;
                LOG_DEBUG("[Encoder] Button held for %lu ms", pressDuration);

                if (pressDuration >= DEBOUNCE_DELAY_LONG_CLICK)
                {
                    longClickPending = true;
                    resetAccumulatedDelta(); // Reset any accumulated rotation when long click is detected
                    LOG_DEBUG("[Encoder] Long click detected");
                }
                else
                {
                    clickPending = true;
                    LOG_DEBUG("[Encoder] Short click detected");
                }

                buttonPressed = false;
                prevButtonPressed = true;
            }

            lastButtonChangeTime = millis();
            lastRawState = buttonRawState;
        }
    }
}

bool EncoderDevice::wasClicked()
{
    if (clickPending)
    {
        clickPending = false;
        LOG_PERIPHERAL("[Encoder] Click event fired at %lu", millis());
        indicateClick();
        return true;
    }
    return false;
}

bool EncoderDevice::wasLongClicked()
{
    if (longClickPending)
    {
        longClickPending = false;
        LOG_PERIPHERAL("[Encoder] Long click event fired at %lu", millis());
        indicateClick();
        return true;
    }
    return false;
}

bool EncoderDevice::isAvailable()
{
    return available;
}

int16_t EncoderDevice::getDelta()
{
    if (!available)
        return 0;

    if (isDebouncedMode())
    {
        if (hasDebouncedEvent())
        {
            int16_t delta = accumulatedDelta;
            resetAccumulatedDelta();
            return delta;
        }
        return 0;
    }
    else
    {
        int16_t delta = encoder.read(true);
        accumulatedDelta += delta;
        return delta;
    }
}

int32_t EncoderDevice::getValue()
{
    return value;
}

bool EncoderDevice::isButtonPressed()
{
    if (!available)
        return false;

    return !encoder.getButtonStatus();
}

void EncoderDevice::setLED(uint32_t color)
{
    if (!available)
        return;

    encoder.setLEDColor(0, color);
}

int16_t EncoderDevice::getAccumulatedDelta()
{
    return accumulatedDelta;
}

void EncoderDevice::resetAccumulatedDelta()
{
    accumulatedDelta = 0;
}

bool EncoderDevice::hasDebouncedEvent()
{
    return (millis() - lastActivityTime) > DEBOUNCE_DELAY_ROTATION &&
           accumulatedDelta != 0;
}

void EncoderDevice::setMode(Mode newMode)
{
    currentMode = newMode;
}

EncoderDevice::Mode EncoderDevice::getMode()
{
    return currentMode;
}

bool EncoderDevice::isDebouncedMode()
{
    return currentMode == DEBOUNCED;
}

void EncoderDevice::indicateNext()
{
    if (available)
    {
        setLED(0x001000);
        delay(50);
        setLED(0x000000);
    }
}

void EncoderDevice::indicatePrev()
{
    if (available)
    {
        setLED(0x000010);
        delay(50);
        setLED(0x000000);
    }
}

void EncoderDevice::indicateClick()
{
    if (available)
    {
        setLED(0x100000);
        delay(50);
        setLED(0x000000);
    }
}
