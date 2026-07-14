#include "screens/astro_screen.h"

#include <M5Unified.h>

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/astro_run_screen.h"
#include "screens/focus_screen.h"
#include "screens/scan_screen.h"
#include "transport/ble_device.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

uint16_t clamp(uint16_t value, uint16_t min, uint16_t max) {
    if (value > max)
        return max;
    if (value < min)
        return min;
    return value;
}

AstroScreen::AstroScreen() : BaseScreen<AstroMenuItem>("Astro"), selectedItem(0) {
    setStatusText("Select Option");
    setStatusBgColor(colors::get(colors::GRAY_800));
    menuItems.setTitle("Astro Menu");

    AstroProcess::instance().init();  // ensure observers (BLE) registered once

    updateMenuItems();
}

AstroScreen::~AstroScreen() {}

void AstroScreen::updateMenuItems() {
    auto& astro = AstroProcess::instance();
    const auto& params = astro.getParameters();
    const auto& status = astro.getStatus();

    menuItems.clear();

    // Config-only screen: the running sequence lives on AstroRunScreen. When
    // PAUSED, offer Resume here (pause parks back on this screen).
    if (!BLEDeviceManager::isConnected()) {
        menuItems.addItem(AstroMenuItem::Connect, "Connect");
    }

    if (status.state == AstroProcess::State::PAUSED) {
        menuItems.addItem(AstroMenuItem::Start, "Resume");
        menuItems.addItem(AstroMenuItem::Stop, "Stop");
    } else {
        menuItems.addItem(AstroMenuItem::Start, "Start");
    }
    menuItems.addItem(AstroMenuItem::Focus, "Focus");
    menuItems.addSeparator();

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%ds", params.initialDelaySec);
    menuItems.addItem(AstroMenuItem::InitialDelay, "Delay", buffer, false);

    snprintf(buffer, sizeof(buffer), "%ds", params.exposureSec);
    menuItems.addItem(AstroMenuItem::ExposureTime, "Exposure", buffer, true);

    snprintf(buffer, sizeof(buffer), "%d", params.subframeCount);
    menuItems.addItem(AstroMenuItem::SubframeCount, "Subframes", buffer, true);

    snprintf(buffer, sizeof(buffer), "%ds", params.intervalSec);
    menuItems.addItem(AstroMenuItem::DelayBetweenExposures, "Interval", buffer, true);
}

void AstroScreen::drawContent() {
    auto& astro = AstroProcess::instance();
    const auto& params = astro.getParameters();

    if (astro.getStatus().state == AstroProcess::State::PAUSED) {
        setStatusText("Paused");
        setStatusBgColor(colors::get(colors::WARNING));
    } else if (!BLEDeviceManager::isConnected()) {
        // No camera: surface it, since a sequence cannot start without one.
        setStatusText(BLEDeviceManager::isPaired() ? "Not connected" : "No camera paired");
        setStatusBgColor(colors::get(colors::ERROR));
    } else {
        char buffer[64];
        uint32_t totalSec = params.getTotalDurationSec();
        snprintf(buffer, sizeof(buffer), "Total: %02d:%02d:%02d", totalSec / 3600,
                 (totalSec % 3600) / 60, totalSec % 60);
        setStatusText(buffer);
        setStatusBgColor(colors::get(colors::SUCCESS));
    }

    menuItems.setSelectedIndex(selectedItem);
    menuItems.draw();
}

void AstroScreen::update() {
    auto& astro = AstroProcess::instance();

    // Handle button inputs
    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
        RemoteControlManager::wasButtonPressed(ButtonId::DOWN)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Next Button Clicked");
        nextMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Prev Button Clicked");
        prevMenuItem();
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
        RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
        LOG_PERIPHERAL("[AstroScreen] [Btn] Confirm Button Clicked");
        adjustParameter(1);
        // handleSelect() may navigate away (Start -> AstroRunScreen, Focus,
        // Scan), which deletes this screen. It returns true in that case; the
        // bool is a stack copy, so reading it after `this` is freed is safe.
        // Touching any member afterwards would be a use-after-free — bail.
        if (handleSelect()) {
            return;
        }
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::RIGHT)) {
        adjustParameter(1);
    }

    if (RemoteControlManager::wasButtonPressed(ButtonId::LEFT)) {
        adjustParameter(-1);
    }

    // Redraw when the camera connection state flips, so the Connect item and
    // status text stay in sync without a button press.
    const bool connected = BLEDeviceManager::isConnected();
    if (connected != wasConnected) {
        wasConnected = connected;
        updateMenuItems();
        draw();
    }
}

void AstroScreen::adjustParameter(uint16_t delta) {
    auto& astro = AstroProcess::instance();
    auto& params = astro.getParameters();
    auto selectedItem = menuItems.getSelectedId();

    switch (selectedItem) {
        case AstroMenuItem::ExposureTime:
            if (!astro.isRunning()) {
                auto newExp = clamp(
                    params.exposureSec + AstroProcess::Parameters::EXPOSURE_STEP * delta,
                    AstroProcess::Parameters::EXPOSURE_MIN, AstroProcess::Parameters::EXPOSURE_MAX);
                astro.setParameter("exposureSec", newExp);
            }
            break;
        case AstroMenuItem::SubframeCount:
            if (!astro.isRunning()) {
                auto newCount = clamp(
                    params.subframeCount + AstroProcess::Parameters::SUBFRAME_COUNT_STEP * delta,
                    AstroProcess::Parameters::SUBFRAME_COUNT_MIN,
                    AstroProcess::Parameters::SUBFRAME_COUNT_MAX);
                astro.setParameter("subframeCount", newCount);
            }
            break;

        case AstroMenuItem::DelayBetweenExposures:
            if (!astro.isRunning()) {
                auto newInterval = clamp(
                    params.intervalSec + AstroProcess::Parameters::INTERVAL_STEP * delta,
                    AstroProcess::Parameters::INTERVAL_MIN, AstroProcess::Parameters::INTERVAL_MAX);
                astro.setParameter("intervalSec", newInterval);
            }
            break;
    }
    updateMenuItems();
    draw();
}

bool AstroScreen::handleSelect() {
    auto& astro = AstroProcess::instance();
    auto selectedItem = menuItems.getSelectedId();

    switch (selectedItem) {
        case AstroMenuItem::Connect:
            if (BLEDeviceManager::isPaired()) {
                // Known device: try a direct reconnect, staying on this screen.
                setStatusText("Connecting...");
                setStatusBgColor(colors::get(colors::IN_PROGRESS));
                drawStatusBar();

                if (BLEDeviceManager::connectToSavedDevice()) {
                    setStatusText("Connected!");
                    setStatusBgColor(colors::get(colors::SUCCESS));
                } else {
                    setStatusText("Failed to connect!");
                    setStatusBgColor(colors::get(colors::ERROR));
                }
                updateMenuItems();
                draw();
            } else {
                // No saved device: go discover/pair one.
                MenuSystem::setScreen(new ScanScreen());
                return true;
            }
            break;

        case AstroMenuItem::Focus:
            MenuSystem::setScreen(new FocusScreen());
            return true;

        case AstroMenuItem::Start:
            // Start a new run, or resume a paused one, then hand off to the
            // in-progress screen which owns the live display.
            if (astro.getStatus().state == AstroProcess::State::PAUSED) {
                astro.resume();
            } else if (!astro.isRunning()) {
                astro.start();
            }
            if (astro.isRunning()) {
                MenuSystem::setScreen(new AstroRunScreen());
                return true;
            }
            updateMenuItems();  // start() refused (invalid params / no camera)
            draw();
            break;

        case AstroMenuItem::Stop:
            // Only reachable while PAUSED on this config screen.
            if (astro.isRunning()) {
                astro.stop();
            }
            updateMenuItems();
            draw();
            break;
    }
    return false;
}

void AstroScreen::nextMenuItem() {
    menuItems.selectNext();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}

void AstroScreen::prevMenuItem() {
    menuItems.selectPrev();
    selectedItem = menuItems.getSelectedIndex();
    draw();
}
