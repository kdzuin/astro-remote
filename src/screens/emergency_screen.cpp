#include "screens/emergency_screen.h"

#include "components/menu_system.h"
#include "processes/astro.h"
#include "processes/settings.h"
#include "screens/astro_run_screen.h"
#include "screens/astro_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"
#include "utils/preferences.h"

namespace {
constexpr uint32_t FLASH_PERIOD_MS = 3000;  // faster than the run-screen alert
constexpr uint32_t FLASH_ON_MS = 300;
}  // namespace

EmergencyScreen::EmergencyScreen() : BaseScreen<BaseMenuItem>("Emergency") {
    // Pause the sequence on entry. If mid-exposure this is deferred until the
    // frame finishes (AstroProcess::pause); the sequence parks in PAUSED either
    // way and we hold here until charged or stopped.
    AstroProcess::instance().pause();
    draw();
}

void EmergencyScreen::update() {
    auto& astro = AstroProcess::instance();
    const int battery = SettingsProcess::getDeviceState().batteryLevel;

    // Auto-resume: charged back above the emergency threshold — return to the
    // live run screen, which re-enters the running sequence.
    if (!SettingsProcess::isBatteryEmergency(battery)) {
        if (lastFlashOn_) {
            M5.Display.setBrightness(PreferencesManager::getBrightness());
        }
        astro.resume();
        MenuSystem::setScreen(new AstroRunScreen());
        return;
    }

    // Stop: user ends the sequence despite the low battery. Park on the config
    // screen (where the sequence would normally end up after a pause/stop).
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM) ||
        RemoteControlManager::wasButtonPressed(ButtonId::BTN_B)) {
        if (lastFlashOn_) {
            M5.Display.setBrightness(PreferencesManager::getBrightness());
        }
        astro.stop();
        MenuSystem::setScreen(new AstroScreen());
        return;
    }

    // Flash: 300ms red out of every 3000ms, brightness boosted during the pulse.
    const bool flashOn = millis() % FLASH_PERIOD_MS < FLASH_ON_MS;
    if (flashOn != lastFlashOn_) {
        lastFlashOn_ = flashOn;
        if (flashOn) {
            M5.Display.setBrightness(200);
            M5.Display.fillScreen(colors::get(colors::ERROR));
        } else {
            M5.Display.setBrightness(PreferencesManager::getBrightness());
            draw();
        }
    }
}

void EmergencyScreen::draw() {
    const int w = M5.Display.width();
    const int h = M5.Display.height();
    const int battery = SettingsProcess::getDeviceState().batteryLevel;

    M5.Display.fillScreen(colors::get(colors::BLACK));
    M5.Display.setTextColor(colors::get(colors::ERROR));
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1.5);
    M5.Display.drawString("LOW BATTERY", w / 2, h / 2 - 24);

    M5.Display.setTextColor(colors::get(colors::WHITE));
    M5.Display.setTextSize(1.25);
    M5.Display.drawString("PAUSED", w / 2, h / 2);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", battery);
    M5.Display.drawString(buf, w / 2, h / 2 + 22);

    M5.Display.setTextColor(colors::get(colors::GRAY_500));
    M5.Display.setTextSize(1);
    M5.Display.drawString("Charge or Stop", w / 2, h - 12);
}
