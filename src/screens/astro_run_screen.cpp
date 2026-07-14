#include "screens/astro_run_screen.h"

#include "components/menu_system.h"
#include "processes/settings.h"
#include "screens/astro_screen.h"
#include "screens/emergency_screen.h"
#include "transport/ble_device.h"
#include "transport/remote_control_manager.h"
#include "utils/colors.h"
#include "utils/preferences.h"

namespace {
// Mirror SelectableList's display constants exactly so the top menu matches the
// config screen (row height, padding between rows, horizontal padding).
constexpr int ITEM_H = 14;
constexpr int ITEM_PAD = 2;
constexpr int HPAD = 8;
constexpr int ROW = ITEM_H + ITEM_PAD;  // vertical advance per row/separator
}  // namespace

AstroRunScreen::AstroRunScreen()
    : BaseScreen<AstroRunItem>("AstroRun"),
      actions_("Astro Run"),
      topCanvas_(&M5.Display),
      botCanvas_(&M5.Display) {
    AstroProcess::instance().init();  // ensure the web BLE observer is registered

    const int w = M5.Display.width();
    const int h = M5.Display.height();
    // Top region reuses SelectableList (title + separator + two action rows), so
    // it matches the config menu exactly. Its height: title + separator + two
    // items, each advancing by ROW. Bottom region is everything else (stats +
    // status bar). Pushed independently so the per-second stats refresh never
    // repaints — or flickers — the menu above.
    topH_ = ROW * 4;

    topCanvas_.setColorDepth(16);
    botCanvas_.setColorDepth(16);
    const bool topOk = topCanvas_.createSprite(w, topH_) != nullptr;
    const bool botOk = botCanvas_.createSprite(w, h - topH_) != nullptr;
    spritesReady_ = topOk && botOk;
}

AstroRunScreen::~AstroRunScreen() {
    // If we navigate away mid-flash, brightness is left boosted — restore it.
    if (lastFlashOn_) {
        M5.Display.setBrightness(PreferencesManager::getBrightness());
    }
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
    const int battery = SettingsProcess::getBatteryLevel();
    const bool emergency = SettingsProcess::isBatteryEmergency(battery);

    // Emergency battery: request a pause (deferred while exposing — the current
    // frame finishes, we keep flashing here in the meantime via the critical
    // alert below). Once the sequence actually parks in PAUSED, the switch below
    // hands off to the full-screen EmergencyScreen instead of the config screen.
    if (emergency && !summaryMode_ && astro.isRunning() &&
        status.state != AstroProcess::State::PAUSED) {
        astro.pause();
    }

    if (!summaryMode_) {
        switch (status.state) {
            case AstroProcess::State::PAUSED:
                if (emergency) {
                    // Battery-forced pause: take over with the emergency screen,
                    // which holds (flashing) until charged or stopped.
                    if (lastFlashOn_) {
                        M5.Display.setBrightness(PreferencesManager::getBrightness());
                    }
                    MenuSystem::setScreen(new EmergencyScreen());
                } else {
                    // User pause parks on the config screen, where Resume lives.
                    MenuSystem::setScreen(new AstroScreen());
                }
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
    const bool pausePending = astro.isPausePending();

    // Summary is a special full-screen state — repaint both regions once.
    if (summaryMode_ != lastSummary_) {
        lastSummary_ = summaryMode_;
        lastAction_ = actionIndex_;
        lastState_ = state;
        lastPausePending_ = pausePending;
        lastElapsed_ = status.elapsedSec;
        lastConnected_ = connected;
        lastBattery_ = battery;
        draw();
        return;
    }
    if (summaryMode_) {
        return;
    }

    // Critical-battery alert: below the critical threshold, flash the whole
    // screen red for 300ms out of every ~10s. Timing is millis()-based, not
    // delay(), so the sequence timers keep running.
    const bool critical = SettingsProcess::isBatteryCritical(battery);
    const bool flashOn = critical && (millis() % 10000UL < 300UL);
    if (flashOn != lastFlashOn_) {
        lastFlashOn_ = flashOn;
        if (flashOn) {
            M5.Display.setBrightness(200);  // punch through; restored on flash-off
            M5.Display.fillScreen(colors::get(colors::ERROR));
        } else {
            // Flash just ended — restore the user's brightness and repaint the
            // full UI over the red fill.
            M5.Display.setBrightness(PreferencesManager::getBrightness());
            drawTop();
            drawBottom();
        }
    }
    if (flashOn) {
        return;  // hold the red fill; skip the normal region redraws below.
    }

    // Top region (menu) changes on selection, run-state, or a pending pause
    // (which flips the label to "Pausing..." while the state stays EXPOSING).
    if (actionIndex_ != lastAction_ || state != lastState_ ||
        pausePending != lastPausePending_) {
        lastAction_ = actionIndex_;
        lastState_ = state;
        lastPausePending_ = pausePending;
        drawTop();
    }
    // Bottom region (stats + bar) changes each second, on state, or on link flip.
    if (status.elapsedSec != lastElapsed_ || state != lastState_ ||
        connected != lastConnected_ || battery != lastBattery_) {
        lastElapsed_ = status.elapsedSec;
        lastConnected_ = connected;
        lastBattery_ = battery;
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
    const bool paused = status.state == AstroProcess::State::PAUSED;
    const bool pausePending = astro.isPausePending();

    // Rebuild the action list each draw so the Pause/Resume/Pausing label tracks
    // state, then render it through SelectableList into the top canvas — same
    // component and visual language as the config menu, no duplicated layout.
    actions_.clear();  // re-adds the leading separator under the title
    actions_.addItem(AstroRunItem::PauseResume,
                     pausePending ? "Pausing..." : (paused ? "Resume" : "Pause"));
    actions_.addItem(AstroRunItem::Stop, "Stop");
    actions_.setSelectedIndex(actionIndex_ + 1);  // +1: index 0 is the separator

    topCanvas_.fillSprite(colors::get(colors::BLACK));
    actions_.draw(topCanvas_, /*clearFirst=*/false);
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

    // Stats sit just above the status bar (bottom-aligned): three time rows,
    // a gap, the battery row, then a gap before the bar.
    constexpr int GAP = 10;  // blank row above and below the battery row
    const int statsAreaH = h - barH;
    int y = statsAreaH - (4 * ITEM_H + 2 * GAP);
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

    // Battery row: "Battery" label on the left, percentage as white text on a
    // status-coloured cell hugging the value on the right (<20 red, <50 amber,
    // else green). A gap separates it from the time rows above and the bar below.
    y += GAP;
    const int battLevel = SettingsProcess::getBatteryLevel();
    const uint32_t battColor = SettingsProcess::getBatteryStatusColor(battLevel);
    botCanvas_.setTextDatum(middle_left);
    botCanvas_.setTextColor(labelColor);
    botCanvas_.drawString("Battery", HPAD, y + ITEM_H / 2);
    snprintf(value, sizeof(value), "%d%%", battLevel);
    const int cellW = botCanvas_.textWidth(value) + 2 * HPAD;
    const int cellX = w - HPAD - cellW;
    botCanvas_.fillRect(cellX, y, cellW, ITEM_H, battColor);
    botCanvas_.setTextDatum(middle_right);
    botCanvas_.setTextColor(white);
    botCanvas_.drawString(value, w - HPAD - HPAD, y + ITEM_H / 2);
    y += ITEM_H;

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
