#pragma once

#include <Preferences.h>

class PreferencesManager
{
public:
    enum class BrightnessLevel : uint8_t
    {
        Level1 = 32,
        Level2 = 64,
        Level3 = 128,
        Level4 = 192,
        COUNT = 4
    };

    static void init();
    static void end();
    static void setBrightness(uint8_t brightness);
    static uint8_t getBrightness();
    static void setAutoConnect(bool enabled);
    static bool getAutoConnect();

    // Helper for cycling through brightness levels
    static BrightnessLevel getNextBrightnessLevel(uint8_t currentBrightness);

private:
    static Preferences preferences;
    static constexpr const char *NAMESPACE = "m5remote";
    static constexpr const char *KEY_BRIGHTNESS = "brightness";
    static constexpr const char *KEY_AUTO_CONNECT = "autoconnect";
    static constexpr uint8_t DEFAULT_BRIGHTNESS = static_cast<uint8_t>(BrightnessLevel::Level3);
    static constexpr bool DEFAULT_AUTO_CONNECT = true;
};
