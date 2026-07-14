#include "screens/astro_run_screen.h"

#include "components/menu_system.h"
#include "screens/astro_screen.h"
#include "transport/ble_device.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"

namespace {
// Mirror SelectableList's display constants exactly so the top menu matches the
// config screen (row height, padding between rows, horizontal padding).
constexpr int ITEM_H = 14;
constexpr int ITEM_PAD = 2;
constexpr int HPAD = 8;
constexpr int ROW = ITEM_H + ITEM_PAD;  // vertical advance per row/separator
}  // namespace

AstroRunScreen::AstroRunScreen()
    : BaseScreen<AstroRunItem>("AstroRun"), topCanvas_(&M5.Display), botCanvas_(&M5.Display) {
    AstroProcess::instance().init();  // ensure the web BLE observer is registered

    const int w = M5.Display.width();
    const int h = M5.Display.height();
    // Top region matches the config menu's layout: title row + separator row +
    // two action rows, each advancing by ROW (ITEM_H + ITEM_PAD). Bottom region
    // is everything else (stats + status bar). Pushed independently so the
    // per-second stats refresh never repaints — or flickers — the menu above.
    topH_ = ROW * 4;

    topCanvas_.setColorDepth(16);
    botCanvas_.setColorDepth(16);
    const bool topOk = topCanvas_.createSprite(w, topH_) != nullptr;
    const bool botOk = botCanvas_.createSprite(w, h - topH_) != nullptr;
    spritesReady_ = topOk && botOk;
}

AstroRunScreen::~AstroRunScreen() {
    if (spritesReady_) {
        topCanvas_.deleteSprite();
        botCanvas_.deleteSprite();
    }
}

void AstroRunScreen::update() {
    auto& astro = AstroProcess::instance();

    // Swallow PWR/BACK so a stray press cannot interrupt the series: MenuSystem
    // would otherwise treat it as "go home" and silently leave a running
    // sequence. Only the on-screen Pause/Stop actions may affect the run.
    RemoteControlManager::wasButtonPressed(ButtonId::BTN_PWR);
    RemoteControlManager::wasButtonPressed(ButtonId::BACK);

    // Buttons first, so a Stop this tick is reflected before we evaluate state.
    if (summaryMode_) {
        if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
            RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
            MenuSystem::setScreen(new AstroScreen());
            return;
        }
    } else {
        if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_B) ||
            RemoteControlManager::wasButtonPressed(ButtonId::DOWN) ||
            RemoteControlManager::wasButtonPressed(ButtonId::UP)) {
            // Only two actions, so any navigation key just toggles between them.
            actionIndex_ = (actionIndex_ + 1) % 2;
        }
        if (RemoteControlManager::wasButtonPressed(ButtonId::BTN_A) ||
            RemoteControlManager::wasButtonPressed(ButtonId::CONFIRM)) {
            if (actionIndex_ == 0) {
                astro.pause();  // deferred while exposing; parks in PAUSED
            } else {
                astro.stop();
            }
        }
    }

    // Evaluate state AFTER buttons (a Stop just above sets STOPPED now).
    const auto& status = astro.getStatus();
    if (!summaryMode_) {
        switch (status.state) {
            case AstroProcess::State::PAUSED:
                // Pause parks back on the config screen, where Resume lives.
                MenuSystem::setScreen(new AstroScreen());
                return;
            case AstroProcess::State::STOPPED:
            case AstroProcess::State::ERROR:
            case AstroProcess::State::IDLE:
                summaryMode_ = true;
                break;
            default:
                break;
        }
    }

    if (!spritesReady_) {
        return;
    }

    const bool connected = BLEDeviceManager::isConnected();
    const int state = static_cast<int>(status.state);

    // Summary is a special full-screen state — repaint both regions once.
    if (summaryMode_ != lastSummary_) {
        lastSummary_ = summaryMode_;
        lastAction_ = actionIndex_;
        lastState_ = state;
        lastElapsed_ = status.elapsedSec;
        lastConnected_ = connected;
        draw();
        return;
    }
    if (summaryMode_) {
        return;
    }

    // Top region (menu) only changes on selection or run-state change.
    if (actionIndex_ != lastAction_ || state != lastState_) {
        lastAction_ = actionIndex_;
        lastState_ = state;
        drawTop();
    }
    // Bottom region (stats + bar) changes each second, on state, or on link flip.
    if (status.elapsedSec != lastElapsed_ || state != lastState_ ||
        connected != lastConnected_) {
        lastElapsed_ = status.elapsedSec;
        lastConnected_ = connected;
        drawBottom();
    }
}

void AstroRunScreen::draw() {
    if (!spritesReady_) {
        return;
    }
    if (summaryMode_) {
        const auto& status = AstroProcess::instance().getStatus();
        const int w = M5.Display.width();
        const int h = M5.Display.height();
        M5.Display.fillScreen(colors::get(colors::BLACK));
        M5.Display.setTextSize(1.25);
        M5.Display.setTextColor(colors::get(colors::WHITE));
        M5.Display.setTextDatum(middle_center);
        const bool ok = status.state == AstroProcess::State::STOPPED &&
                        status.completedFrames >= status.totalFrames;
        M5.Display.drawString(ok ? "Complete" : "Stopped", w / 2, h / 2 - 12);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d/%d", status.completedFrames, status.totalFrames);
        M5.Display.drawString(buf, w / 2, h / 2 + 6);
        return;
    }
    drawTop();
    drawBottom();
}

void AstroRunScreen::drawTop() {
    auto& astro = AstroProcess::instance();
    const auto& status = astro.getStatus();
    const int w = topCanvas_.width();
    const uint32_t white = colors::get(colors::WHITE);
    const uint32_t black = colors::get(colors::BLACK);

    topCanvas_.fillSprite(black);
    topCanvas_.setTextSize(1.25);

    int y = 0;

    // Title row — same baseline the menu uses (ITEM_H / 1.25).
    topCanvas_.setTextColor(white);
    topCanvas_.setTextDatum(middle_center);
    topCanvas_.drawString("Astro Run", w / 2, y + ITEM_H / 1.25);
    y += ROW;

    // Separator row — a line through the middle of a full row, like the menu.
    topCanvas_.drawLine(0, y + ITEM_H / 2, w, y + ITEM_H / 2, colors::get(colors::GRAY_500));
    y += ROW;

    // Action rows: full-width selection highlight, ROW spacing.
    const bool paused = status.state == AstroProcess::State::PAUSED;
    const bool pausePending = astro.isPausePending();
    const char* actions[2] = {pausePending ? "Pausing..." : (paused ? "Resume" : "Pause"), "Stop"};
    for (int i = 0; i < 2; i++) {
        const bool sel = (i == actionIndex_);
        if (sel) {
            topCanvas_.fillRect(0, y, w, ITEM_H, white);
        }
        topCanvas_.setTextColor(sel ? black : white);
        topCanvas_.setTextDatum(middle_left);
        topCanvas_.drawString(actions[i], HPAD, y + ITEM_H / 2);
        y += ROW;
    }

    topCanvas_.pushSprite(0, 0);
}

void AstroRunScreen::drawBottom() {
    const auto& status = AstroProcess::instance().getStatus();
    const auto& params = AstroProcess::instance().getParameters();
    const int w = botCanvas_.width();
    const int h = botCanvas_.height();
    const int barH = STATUS_BAR_HEIGHT;
    const uint32_t white = colors::get(colors::WHITE);
    const uint32_t labelColor = colors::get(colors::GRAY_500);
    const uint32_t valueColor = colors::get(colors::GRAY_200);

    botCanvas_.fillSprite(colors::get(colors::BLACK));
    botCanvas_.setTextSize(1.25);

    // Stats: three rows sitting just above the status bar (bottom-aligned).
    const int statsAreaH = h - barH;
    int y = statsAreaH - 3 * ITEM_H;
    if (y < 0) {
        y = 0;
    }
    auto infoRow = [&](const char* label, const char* value) {
        botCanvas_.setTextDatum(middle_left);
        botCanvas_.setTextColor(labelColor);
        botCanvas_.drawString(label, HPAD, y + ITEM_H / 2);
        botCanvas_.setTextDatum(middle_right);
        botCanvas_.setTextColor(valueColor);
        botCanvas_.drawString(value, w - HPAD, y + ITEM_H / 2);
        y += ITEM_H;
    };

    char value[16];
    snprintf(value, sizeof(value), "%d/%d", status.completedFrames + 1, params.subframeCount);
    infoRow("Frame", value);
    snprintf(value, sizeof(value), "%02d:%02d:%02d", status.elapsedSec / 3600,
             (status.elapsedSec % 3600) / 60, status.elapsedSec % 60);
    infoRow("Elapsed", value);
    snprintf(value, sizeof(value), "%02d:%02d:%02d", status.remainingSec / 3600,
             (status.remainingSec % 3600) / 60, status.remainingSec % 60);
    infoRow("Left", value);

    // Status bar: colour-codes the phase, shows the phase countdown.
    uint32_t stateColor;
    switch (status.state) {
        case AstroProcess::State::EXPOSING:
            stateColor = colors::get(colors::GREEN_500);
            break;
        case AstroProcess::State::INITIAL_DELAY:
        case AstroProcess::State::INTERVAL:
            stateColor = colors::get(colors::BLUE_500);
            break;
        default:
            stateColor = colors::get(colors::GRAY_800);
    }
    if (!BLEDeviceManager::isConnected()) {
        stateColor = colors::get(colors::ERROR);
    }
    botCanvas_.fillRect(0, h - barH, w, barH, stateColor);
    botCanvas_.setTextColor(white);
    botCanvas_.setTextDatum(middle_center);
    char bar[12];
    snprintf(bar, sizeof(bar), "%02d:%02d", (status.phaseRemainingSec % 3600) / 60,
             status.phaseRemainingSec % 60);
    botCanvas_.drawString(bar, w / 2, h - barH / 2);

    botCanvas_.pushSprite(0, topH_);
}
