#include "screens/astro_screen.h"

#include <M5Unified.h>

#include "components/menu_system.h"
#include "processes/astro.h"
#include "screens/focus_screen.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

AstroScreen::AstroScreen() : BaseScreen<AstroMenuItem>("Astro") {
    setStatusText("Select Option");
    setStatusBgColor(colors::get(colors::GRAY_800));
    menuItems.setTitle("Astro Menu");
    updateMenuItems();
}

void AstroScreen::updateMenuItems() {
    auto& astro = AstroProcess::instance();
    const auto& params = astro.getParameters();
    const auto& status = astro.getStatus();

    menuItems.clear();

    menuItems.addItem(AstroMenuItem::Focus, "Focus");
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
    menuItems.addSeparator();

    // Show parameters when not running
    if (!astro.isRunning()) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%ds", params.initialDelaySec);
        menuItems.addItem(AstroMenuItem::InitialDelay, "Delay", buffer, true);

        snprintf(buffer, sizeof(buffer), "%ds", params.exposureSec);
        menuItems.addItem(AstroMenuItem::ExposureTime, "Exp", buffer, true);

        snprintf(buffer, sizeof(buffer), "%d", params.subframeCount);
        menuItems.addItem(AstroMenuItem::NumberOfExposures, "Subs", buffer, true);

        snprintf(buffer, sizeof(buffer), "%ds", params.intervalSec);
        menuItems.addItem(AstroMenuItem::DelayBetweenExposures, "Int", buffer, true);

    } else {
        // Show status when running
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Frame: %d/%d", status.completedFrames + 1,
                 params.subframeCount);
        menuItems.addItem(AstroMenuItem::NumberOfExposures, buffer);
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
        uint32_t totalSec = params.totalDurationSec();
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

    // Update menu if state changed
    static AstroProcess::State lastState = astro.getStatus().state;
    static uint16_t lastCompleted = astro.getStatus().completedFrames;

    if (lastState != astro.getStatus().state ||
        lastCompleted != astro.getStatus().completedFrames) {
        updateMenuItems();
        lastState = astro.getStatus().state;
        lastCompleted = astro.getStatus().completedFrames;
    }

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
        selectMenuItem();
    }
}

void AstroScreen::selectMenuItem() {
    auto& astro = AstroProcess::instance();
    auto& params = astro.getParameters();
    auto selectedItem = menuItems.getSelectedId();

    switch (selectedItem) {
        case AstroMenuItem::Focus:
            MenuSystem::setScreen(new FocusScreen());
            break;

        case AstroMenuItem::InitialDelay:
            if (!astro.isRunning()) {
                auto newDelay = params.initialDelaySec + 5;
                if (newDelay > 300)
                    newDelay = 5;  // Max 5 minutes
                astro.setParameter("initialDelaySec", newDelay);
                updateMenuItems();
                draw();
            }
            break;

        case AstroMenuItem::ExposureTime:
            if (!astro.isRunning()) {
                auto newExp = params.exposureSec + 30;
                if (newExp > 3600)
                    newExp = 30;  // Max 1 hour
                astro.setParameter("exposureSec", newExp);
                updateMenuItems();
                draw();
            }
            break;

        case AstroMenuItem::NumberOfExposures:
            if (!astro.isRunning()) {
                auto newCount = params.subframeCount + 5;
                if (newCount > 100)
                    newCount = 5;  // Max 100 frames
                astro.setParameter("subframeCount", newCount);
                updateMenuItems();
                draw();
            }
            break;

        case AstroMenuItem::DelayBetweenExposures:
            if (!astro.isRunning()) {
                auto newInterval = params.intervalSec + 1;
                if (newInterval > 60)
                    newInterval = 1;  // Max 1 minute
                astro.setParameter("intervalSec", newInterval);
                updateMenuItems();
                draw();
            }
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
                updateMenuItems();
                draw();
            }
            break;

        case AstroMenuItem::Stop:
            if (astro.isRunning()) {
                astro.stop();
                updateMenuItems();
                draw();
            }
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
