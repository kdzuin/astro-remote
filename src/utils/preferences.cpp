#include "preferences.h"

Preferences PreferencesManager::preferences;

void PreferencesManager::init()
{
    bool success = preferences.begin(NAMESPACE, false);
    LOG_DEBUG("[Preferences] Preferences init: %s\n", success ? "OK" : "FAILED");
}

void PreferencesManager::end()
{
    preferences.end();
    LOG_DEBUG("[Preferences] Preferences ended");
}

void PreferencesManager::setBrightness(uint8_t brightness)
{
    size_t written = preferences.putUChar(KEY_BRIGHTNESS, brightness);
    LOG_DEBUG("[Preferences] Saving brightness: %d (written: %d bytes)\n", brightness, written);
}

uint8_t PreferencesManager::getBrightness()
{
    uint8_t value = preferences.getUChar(KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    LOG_DEBUG("[Preferences] Loading brightness: %d (default: %d)\n", value, DEFAULT_BRIGHTNESS);
    return value;
}

void PreferencesManager::setAutoConnect(bool enabled)
{
    size_t written = preferences.putBool(KEY_AUTO_CONNECT, enabled);
    LOG_DEBUG("[Preferences] Saving autoconnect: %d (written: %d bytes)\n", enabled, written);
}

bool PreferencesManager::getAutoConnect()
{
    bool value = preferences.getBool(KEY_AUTO_CONNECT, DEFAULT_AUTO_CONNECT);
    LOG_DEBUG("[Preferences] Loading autoconnect: %d (default: %d)\n", value, DEFAULT_AUTO_CONNECT);
    return value;
}

PreferencesManager::BrightnessLevel PreferencesManager::getNextBrightnessLevel(uint8_t currentBrightness)
{
    // Find the next brightness level
    if (currentBrightness >= static_cast<uint8_t>(BrightnessLevel::Level4))
    {
        return BrightnessLevel::Level1;
    }
    else if (currentBrightness >= static_cast<uint8_t>(BrightnessLevel::Level3))
    {
        return BrightnessLevel::Level4;
    }
    else if (currentBrightness >= static_cast<uint8_t>(BrightnessLevel::Level2))
    {
        return BrightnessLevel::Level3;
    }
    else if (currentBrightness >= static_cast<uint8_t>(BrightnessLevel::Level1))
    {
        return BrightnessLevel::Level2;
    }
    return BrightnessLevel::Level1;
}
