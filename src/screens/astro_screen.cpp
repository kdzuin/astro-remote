#include "screens/astro_screen.h"

#include <M5Unified.h>

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/focus_screen.h"
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

    if (!astro.isRunning()) {
        menuItems.addItem(AstroMenuItem::Start, "Start");
    } else {
        if (status.state == AstroProcess::State::PAUSED) {
            menuItems.addItem(AstroMenuItem::Start, "Resume");
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
        // Show status when running
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Frame: %d/%d", status.completedFrames + 1,
                 params.subframeCount);
        menuItems.addItem(AstroMenuItem::SubframeCount, buffer);
    }
}

void AstroScreen::drawContent() {
    auto& astro = AstroProcess::instance();
    const auto& status = astro.getStatus();
    const auto& params = astro.getParameters();

    // Draw status bar
    if (astro.isRunning()) {
        char buffer[64];
        const char* stateStr;
        uint16_t stateColor;

        switch (status.state) {
            case AstroProcess::State::INITIAL_DELAY:
                stateStr = "Delay";
                stateColor = colors::get(colors::BLUE_500);
                break;
            case AstroProcess::State::EXPOSING:
                stateStr = "Exposing";
                stateColor = colors::get(colors::GREEN_500);
                break;
            case AstroProcess::State::INTERVAL:
                stateStr = "Interval";
                stateColor = colors::get(colors::YELLOW_500);
                break;
            case AstroProcess::State::PAUSED:
                stateStr = "Paused";
                stateColor = colors::get(colors::ORANGE_500);
                break;
            default:
                stateStr = "Unknown";
                stateColor = colors::get(colors::RED_500);
        }

        snprintf(buffer, sizeof(buffer), "%s | %02d:%02d:%02d | %02d:%02d:%02d", stateStr,
                 status.elapsedSec / 3600, (status.elapsedSec % 3600) / 60, status.elapsedSec % 60,
                 status.remainingSec / 3600, (status.remainingSec % 3600) / 60,
                 status.remainingSec % 60);

        setStatusText(buffer);
        setStatusBgColor(stateColor);

    } else {
        char buffer[64];
        uint32_t totalSec = params.getTotalDurationSec();
        snprintf(buffer, sizeof(buffer), "Total: %02d:%02d:%02d", totalSec / 3600,
                 (totalSec % 3600) / 60, totalSec % 60);
        setStatusText(buffer);
        setStatusBgColor(colors::get(colors::GRAY_800));
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
        case AstroMenuItem::Focus:
            MenuSystem::setScreen(new FocusScreen());
            break;

        case AstroMenuItem::Start:
            if (astro.getStatus().state == AstroProcess::State::PAUSED) {
                astro.start();
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
