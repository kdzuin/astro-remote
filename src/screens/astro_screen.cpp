#include "screens/astro_screen.h"

#include <M5Unified.h>

#include "components/menu_system.h"
#include "processes/astro.h"
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

    // Register as observer
    AstroProcess::instance().init();
    AstroProcess::instance().addObserver(this);

    updateMenuItems();
}

AstroScreen::~AstroScreen() {
    // Unregister as observer
    AstroProcess::instance().removeObserver(this);
    // Clean up any state
}

void AstroScreen::updateMenuItems() {
    auto& astro = AstroProcess::instance();
    const auto& params = astro.getParameters();
    const auto& status = astro.getStatus();

    menuItems.clear();

    // Offer a connect entry (first item) whenever the camera is not connected,
    // so the camera can be reached without leaving the Astro screen.
    if (!BLEDeviceManager::isConnected()) {
        menuItems.addItem(AstroMenuItem::Connect, "Connect");
    }

    if (!astro.isRunning()) {
        menuItems.addItem(AstroMenuItem::Start, "Start");
    } else {
        if (status.state == AstroProcess::State::PAUSED) {
            menuItems.addItem(AstroMenuItem::Start, "Resume");
        } else if (astro.isPausePending()) {
            // Pause requested, waiting for the current frame to finish.
            menuItems.addItem(AstroMenuItem::Pause, "Pausing...", false);
        } else {
            menuItems.addItem(AstroMenuItem::Pause, "Pause");
        }
        menuItems.addItem(AstroMenuItem::Stop, "Stop");
    }
    menuItems.addItem(AstroMenuItem::Focus, "Focus");
    menuItems.addSeparator();

    // Show parameters when not running
    if (!astro.isRunning()) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%ds", params.initialDelaySec);
        menuItems.addItem(AstroMenuItem::InitialDelay, "Delay", buffer, false);

        snprintf(buffer, sizeof(buffer), "%ds", params.exposureSec);
        menuItems.addItem(AstroMenuItem::ExposureTime, "Exposure", buffer, true);

        snprintf(buffer, sizeof(buffer), "%d", params.subframeCount);
        menuItems.addItem(AstroMenuItem::SubframeCount, "Subframes", buffer, true);

        snprintf(buffer, sizeof(buffer), "%ds", params.intervalSec);
        menuItems.addItem(AstroMenuItem::DelayBetweenExposures, "Interval", buffer, true);

    } else {
        // Running: show frame progress plus whole-series elapsed / remaining.
        char frameBuf[16];
        snprintf(frameBuf, sizeof(frameBuf), "%d/%d", status.completedFrames + 1,
                 params.subframeCount);
        menuItems.addItem(AstroMenuItem::SubframeCount, "Frame", frameBuf, false);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", status.elapsedSec / 3600,
                 (status.elapsedSec % 3600) / 60, status.elapsedSec % 60);
        menuItems.addItem(AstroMenuItem::SubframeCount, "Elapsed", timeBuf, false);

        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", status.remainingSec / 3600,
                 (status.remainingSec % 3600) / 60, status.remainingSec % 60);
        menuItems.addItem(AstroMenuItem::SubframeCount, "Left", timeBuf, false);
    }
}

void AstroScreen::drawContent() {
    auto& astro = AstroProcess::instance();
    const auto& status = astro.getStatus();
    const auto& params = astro.getParameters();

    // Draw status bar
    if (astro.isRunning()) {
        // State is shown as a compact ASCII symbol, colour-coded via the bar:
        //   ...  delay      *  shooting     ~  interval    ||  paused
        // A dropped camera mid-run flips the symbol to '!' so the loss is
        // visible even though the bar colour otherwise tracks the phase.
        const bool connected = BLEDeviceManager::isConnected();
        const char* symbol;
        uint32_t stateColor;

        switch (status.state) {
            case AstroProcess::State::INITIAL_DELAY:
                symbol = "...";
                stateColor = colors::get(colors::BLUE_500);
                break;
            case AstroProcess::State::EXPOSING:
                symbol = "*";
                stateColor = colors::get(colors::GREEN_500);
                break;
            case AstroProcess::State::INTERVAL:
                symbol = "~";
                stateColor = colors::get(colors::BLUE_500);
                break;
            case AstroProcess::State::PAUSED:
                symbol = "||";
                stateColor = colors::get(colors::BLACK);  // line off while paused
                break;
            default:
                symbol = "?";
                stateColor = colors::get(colors::BLACK);
        }
        if (!connected) {
            symbol = "!";
        }

        // Symbol + time left in the CURRENT phase (mm:ss).
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s  %02d:%02d", symbol,
                 (status.phaseRemainingSec % 3600) / 60, status.phaseRemainingSec % 60);
        setStatusText(buffer);
        setStatusBgColor(stateColor);

    } else if (!BLEDeviceManager::isConnected()) {
        // Not running and no camera: surface the connection state, since the
        // sequence cannot start without it.
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
    // Draw menu

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
        selectMenuItem();
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

void AstroScreen::selectMenuItem() {
    auto& astro = AstroProcess::instance();
    auto& params = astro.getParameters();
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
            }
            break;

        case AstroMenuItem::Focus:
            MenuSystem::setScreen(new FocusScreen());
            break;

        case AstroMenuItem::Start:
            if (astro.getStatus().state == AstroProcess::State::PAUSED) {
                astro.resume();  // Continue from the paused frame count.
            } else if (!astro.isRunning()) {
                astro.start();
            }
            updateMenuItems();
            draw();
            break;

        case AstroMenuItem::Pause:
            if (astro.isRunning()) {
                astro.pause();
            }
            updateMenuItems();
            draw();
            break;

        case AstroMenuItem::Stop:
            if (astro.isRunning()) {
                astro.stop();
            }
            updateMenuItems();
            draw();
            break;
    }
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
