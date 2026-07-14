// Native unit tests for FocusProcess (manual-focus sensitivity + focus flag).
//
// Strategy: unity-build. FocusProcess is header-only (processes/focus.h), so we
// just #include it. Its only collaborators are CameraCommands::sendCommand16 /
// sendCommand24, which we mock here to record the last command/param sent —
// enough to assert that toggling focus half-presses the shutter and that
// stepping only fires while focusing.

#include <unity.h>

#include "Arduino.h"  // fake clock + Serial + no-op delay()
#include "debug.h"    // LOG_* macros + DebugLevel

// ---- Global definitions the code-under-test expects -------------------------
uint32_t g_fakeMillis = 0;
SerialStub Serial;
DebugLevel DEBUG_LEVEL = DebugLevel::APP;

// ---- Mock collaborators -----------------------------------------------------
// Record the last command/param so tests can see what FocusProcess emitted.
struct FocusMockState {
    int cmd16Calls = 0;
    int cmd24Calls = 0;
    uint16_t lastCmd = 0;
    uint8_t lastParam = 0;
    void reset() { *this = FocusMockState(); }
};
static FocusMockState g_fmock;

namespace CameraCommands {
bool sendCommand16(uint16_t cmd) {
    g_fmock.cmd16Calls++;
    g_fmock.lastCmd = cmd;
    return true;
}
bool sendCommand24(uint16_t cmd, uint8_t param) {
    g_fmock.cmd24Calls++;
    g_fmock.lastCmd = cmd;
    g_fmock.lastParam = param;
    return true;
}
}  // namespace CameraCommands

// ---- Code under test (unity build) ------------------------------------------
#include "processes/focus.h"

// ---- Helpers ----------------------------------------------------------------
static FocusProcess::FocusState& state() { return FocusProcess::getState(); }

void setUp() {
    g_fmock.reset();
    // FocusState is a function-local static (persists across tests); reset it to
    // a known baseline: not focusing, Medium sensitivity.
    state().focusing = false;
    state().sensitivity = FocusSensitivity::Medium;
}

void tearDown() {}

// ---- Tests ------------------------------------------------------------------

// cycleSensitivity (BTN_B) must walk ALL four steps and wrap, not bounce
// between the top two. This is the bug: the old impl clamped at Coarse and fell
// back to High, so Fine/Medium were unreachable.
void test_cycle_visits_all_five_and_wraps() {
    state().sensitivity = FocusSensitivity::Finest;

    FocusProcess::cycleSensitivity();
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Fine),
                      static_cast<int>(state().sensitivity));
    FocusProcess::cycleSensitivity();
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Medium),
                      static_cast<int>(state().sensitivity));
    FocusProcess::cycleSensitivity();
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::High),
                      static_cast<int>(state().sensitivity));
    FocusProcess::cycleSensitivity();
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Coarse),
                      static_cast<int>(state().sensitivity));
    FocusProcess::cycleSensitivity();  // wrap back to the start
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Finest),
                      static_cast<int>(state().sensitivity));
}

// nextSensitivity steps up one level and clamps at Coarse (returns false at max,
// leaving the value unchanged). UP/DOWN buttons rely on this clamp behavior.
void test_next_steps_up_and_clamps() {
    state().sensitivity = FocusSensitivity::Finest;
    TEST_ASSERT_TRUE(FocusProcess::nextSensitivity());  // -> Fine
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Fine),
                      static_cast<int>(state().sensitivity));
    TEST_ASSERT_TRUE(FocusProcess::nextSensitivity());  // -> Medium
    TEST_ASSERT_TRUE(FocusProcess::nextSensitivity());  // -> High
    TEST_ASSERT_TRUE(FocusProcess::nextSensitivity());  // -> Coarse
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Coarse),
                      static_cast<int>(state().sensitivity));
    TEST_ASSERT_FALSE(FocusProcess::nextSensitivity());  // clamp at max
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Coarse),
                      static_cast<int>(state().sensitivity));
}

// prevSensitivity steps down one level and clamps at Fine (returns false at min).
void test_prev_steps_down_and_clamps() {
    state().sensitivity = FocusSensitivity::Coarse;
    TEST_ASSERT_TRUE(FocusProcess::prevSensitivity());  // -> High
    TEST_ASSERT_TRUE(FocusProcess::prevSensitivity());  // -> Medium
    TEST_ASSERT_TRUE(FocusProcess::prevSensitivity());  // -> Fine
    TEST_ASSERT_TRUE(FocusProcess::prevSensitivity());  // -> Finest
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Finest),
                      static_cast<int>(state().sensitivity));
    TEST_ASSERT_FALSE(FocusProcess::prevSensitivity());  // clamp at min
    TEST_ASSERT_EQUAL(static_cast<int>(FocusSensitivity::Finest),
                      static_cast<int>(state().sensitivity));
}

// Toggling focus on half-presses the shutter (SHUTTER_HALF_DOWN 0x0107);
// toggling off releases it (SHUTTER_HALF_UP 0x0106).
void test_toggle_focus_sends_half_press() {
    FocusProcess::updateFocusState(true);
    TEST_ASSERT_TRUE(state().focusing);
    TEST_ASSERT_EQUAL_UINT16(CameraCommands::Cmd::SHUTTER_HALF_DOWN, g_fmock.lastCmd);

    FocusProcess::updateFocusState(false);
    TEST_ASSERT_FALSE(state().focusing);
    TEST_ASSERT_EQUAL_UINT16(CameraCommands::Cmd::SHUTTER_HALF_UP, g_fmock.lastCmd);
}

// handleFocus must be a no-op unless the user has entered focusing mode (pressed
// A). This is exactly the "it didn't work" gotcha: stepping does nothing until
// focus is toggled on.
void test_handle_focus_noop_when_not_focusing() {
    state().focusing = false;
    FocusProcess::handleFocus(1);
    FocusProcess::handleFocus(-1);
    TEST_ASSERT_EQUAL(0, g_fmock.cmd24Calls);  // nothing sent
}

// While focusing, a positive increment drives FOCUS_OUT with the current
// sensitivity as the press parameter; the release that follows carries 0x00.
void test_handle_focus_out_uses_sensitivity() {
    state().focusing = true;
    state().sensitivity = FocusSensitivity::High;

    FocusProcess::handleFocus(1);

    // press (param = sensitivity) followed by release (param = 0x00)
    TEST_ASSERT_EQUAL(2, g_fmock.cmd24Calls);
    TEST_ASSERT_EQUAL_UINT16(CameraCommands::Cmd::FOCUS_OUT_RELEASE, g_fmock.lastCmd);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_fmock.lastParam);
}

// A negative increment drives FOCUS_IN (the opposite direction).
void test_handle_focus_in_on_negative() {
    state().focusing = true;
    state().sensitivity = FocusSensitivity::Fine;

    FocusProcess::handleFocus(-1);

    TEST_ASSERT_EQUAL(2, g_fmock.cmd24Calls);
    TEST_ASSERT_EQUAL_UINT16(CameraCommands::Cmd::FOCUS_IN_RELEASE, g_fmock.lastCmd);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_cycle_visits_all_five_and_wraps);
    RUN_TEST(test_next_steps_up_and_clamps);
    RUN_TEST(test_prev_steps_down_and_clamps);
    RUN_TEST(test_toggle_focus_sends_half_press);
    RUN_TEST(test_handle_focus_noop_when_not_focusing);
    RUN_TEST(test_handle_focus_out_uses_sensitivity);
    RUN_TEST(test_handle_focus_in_on_negative);
    return UNITY_END();
}
