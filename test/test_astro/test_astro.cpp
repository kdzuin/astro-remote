// Native unit tests for AstroProcess (the bulb-sequence state machine).
//
// Strategy: unity-build. We #include the real astro.cpp so there is nothing to
// link, and we supply mock definitions for its three collaborators
// (CameraCommands::takeBulb / emergencyStop, BLERemoteServer::sendAstroStatus).
// The fake Arduino clock lets a multi-minute sequence run instantly.

#include <unity.h>

#include "Arduino.h"  // fake clock + Serial before anything pulls it transitively
#include "debug.h"    // LOG_* macros + DebugLevel
#include "mock_recorder.h"

// ---- Global definitions the code-under-test expects -------------------------
uint32_t g_fakeMillis = 0;
SerialStub Serial;
DebugLevel DEBUG_LEVEL = DebugLevel::APP;
AstroMockState g_mock;

// ---- Mock collaborators -----------------------------------------------------
namespace CameraCommands {
bool triggerBulb() {
    g_mock.triggerBulbCalls++;
    if (g_mock.triggerBulbShouldFail) {
        return false;
    }
    g_mock.shutterActive = !g_mock.shutterActive;  // Toggle, like the real camera.
    return true;
}
bool emergencyStop() {
    g_mock.emergencyStopCalls++;
    return true;
}
bool isShutterActive() {
    return g_mock.shutterActive;
}
}  // namespace CameraCommands

void BLERemoteServer::sendAstroStatus(const AstroStatusPacket& status) {
    g_mock.sendAstroStatusCalls++;
    g_mock.lastStatus = status;
}

// ---- Code under test (unity build) ------------------------------------------
#include "processes/astro.cpp"

// ---- Helpers ----------------------------------------------------------------
static AstroProcess& astro() { return AstroProcess::instance(); }

// Counting observer: records how many status callbacks arrive and the last
// state seen, so tests can assert the tick fires and the 1 Hz throttle holds.
struct CountingObserver : public AstroProcess::Observer {
    int statusCalls = 0;
    AstroProcess::State lastState = AstroProcess::State::IDLE;
    void onAstroParametersChanged(const AstroProcess::Parameters&) override {}
    void onAstroStatusChanged(const AstroProcess::Status& s) override {
        statusCalls++;
        lastState = s.state;
    }
};

// Drive the state machine forward by `seconds`, ticking update() once per
// simulated second (matches the 1 Hz granularity astro.cpp works at).
static void advanceSeconds(uint32_t seconds) {
    for (uint32_t i = 0; i < seconds; i++) {
        advanceMillis(1000);
        astro().update();
    }
}

void setUp() {
    g_mock.reset();
    setMillis(0);
    astro().reset();
    // reset() leaves lastUpdateTime_ stale; prime it with one tick at t=0.
    astro().update();
}

void tearDown() {}

// ---- Tests ------------------------------------------------------------------

// Invalid parameters must refuse to start and report errorCode 1.
void test_start_rejects_invalid_params() {
    AstroProcess::Parameters p;
    p.exposureSec = 45;  // not a multiple of 30 -> invalid
    astro().setParameters(p);

    astro().start();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::ERROR),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT8(1, astro().getStatus().errorCode);
}

// Valid params but no camera -> errorCode 2.
void test_start_requires_camera() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 10;  // must be a multiple of 10 and >= 10
    p.intervalSec = 3;
    astro().setParameters(p);
    // isCameraConnected defaults false

    astro().start();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::ERROR),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT8(2, astro().getStatus().errorCode);
}

// setParameter enforces the validation rules: step multiples plus min/max
// bounds. Invalid values must be rejected and leave params unchanged.
void test_setParameter_validation() {
    // exposure: step 30, range 30..600
    TEST_ASSERT_TRUE(astro().setParameter("exposureSec", 60));
    TEST_ASSERT_FALSE(astro().setParameter("exposureSec", 61));   // not a multiple of 30
    TEST_ASSERT_FALSE(astro().setParameter("exposureSec", 700));  // above max
    TEST_ASSERT_EQUAL_UINT16(60, astro().getParameters().exposureSec);  // unchanged

    // initial delay: step 5
    TEST_ASSERT_TRUE(astro().setParameter("initialDelaySec", 10));
    TEST_ASSERT_FALSE(astro().setParameter("initialDelaySec", 7));

    // subframe count: step 10, range 10..480
    TEST_ASSERT_TRUE(astro().setParameter("subframeCount", 20));
    TEST_ASSERT_FALSE(astro().setParameter("subframeCount", 15));  // not a multiple of 10
    TEST_ASSERT_FALSE(astro().setParameter("subframeCount", 5));   // below min

    // interval: range 3..60 (3s minimum guardrail between exposures)
    TEST_ASSERT_FALSE(astro().setParameter("intervalSec", 2));   // below min
    TEST_ASSERT_TRUE(astro().setParameter("intervalSec", 3));    // at min
    TEST_ASSERT_FALSE(astro().setParameter("intervalSec", 61));  // above max

    TEST_ASSERT_FALSE(astro().setParameter("bogus", 5));  // unknown parameter
}

// A full 10-frame sequence. Bulb is a toggle: each frame fires triggerBulb
// twice (open at exposure start, close at exposure end), so 10 frames = 20
// triggers. Ends STOPPED. delay=5, exposure=30, interval=3, frames=10.
void test_full_sequence_completes() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);

    astro().setCameraConnected(true);

    astro().start();
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INITIAL_DELAY),
                      static_cast<int>(astro().getStatus().state));

    // Total sequence: 5 (delay) + 10*(30 exposure + 3 interval) = 335s.
    // Run a bit past the end.
    advanceSeconds(350);

    TEST_ASSERT_EQUAL(20, g_mock.triggerBulbCalls);  // 10 open + 10 close
    TEST_ASSERT_EQUAL_UINT16(10, astro().getStatus().completedFrames);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
}

// phaseRemainingSec counts down the time left in the CURRENT phase (delay,
// exposure, or interval) — distinct from remainingSec (whole series).
void test_phase_remaining_counts_down_per_phase() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();  // INITIAL_DELAY, 5s

    advanceSeconds(2);  // 2s into the 5s delay
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INITIAL_DELAY),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT32(3, astro().getStatus().phaseRemainingSec);  // 5 - 2
    TEST_ASSERT_EQUAL_UINT32(5, astro().getStatus().phaseTotalSec);      // delay length

    advanceSeconds(4);  // t=6: delay done at t=5, 1s into the 30s exposure
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT32(29, astro().getStatus().phaseRemainingSec);  // 30 - 1
    TEST_ASSERT_EQUAL_UINT32(30, astro().getStatus().phaseTotalSec);      // exposure length

    advanceSeconds(30);  // t=36: exposure ended at t=35, 1s into the 3s interval
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INTERVAL),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT32(2, astro().getStatus().phaseRemainingSec);  // 3 - 1
    TEST_ASSERT_EQUAL_UINT32(3, astro().getStatus().phaseTotalSec);      // interval length
}

// Pause while exposing is DEFERRED: the current frame must finish its full
// exposure (not be cut short), then the sequence enters PAUSED with the frame
// counted. delay=0, exposure=30, so the frame ends at t=30.
void test_pause_during_exposure_defers_until_frame_done() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(2);  // into first exposure (open toggle fired, 1 trigger)
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));

    int triggersBefore = g_mock.triggerBulbCalls;  // == 1 (open)
    astro().pause();

    // Still exposing right after pause: frame not cut short, shutter not closed.
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersBefore, g_mock.triggerBulbCalls);

    // Let the exposure run to its full length; the frame closes and counts,
    // then the sequence parks in PAUSED (not INTERVAL).
    advanceSeconds(30);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::PAUSED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT16(1, astro().getStatus().completedFrames);
    TEST_ASSERT_EQUAL(triggersBefore + 1, g_mock.triggerBulbCalls);  // close toggle
}

// Pause during the interval (shutter already closed) parks immediately in
// PAUSED with no extra shutter toggle.
void test_pause_during_interval_is_immediate() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 5;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(31);  // frame 1 done, now in INTERVAL
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INTERVAL),
                      static_cast<int>(astro().getStatus().state));

    int triggersBefore = g_mock.triggerBulbCalls;
    astro().pause();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::PAUSED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersBefore, g_mock.triggerBulbCalls);  // no toggle
}

// Resume continues the sequence from the preserved frame count (does not
// restart from frame 0), entering the interval wait before the next frame.
void test_resume_continues_from_count() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(31);  // frame 1 done (open+close = 2 triggers), in INTERVAL
    astro().pause();
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::PAUSED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT16(1, astro().getStatus().completedFrames);

    // Sit paused a while — must not advance or shoot.
    int triggersAtPause = g_mock.triggerBulbCalls;
    advanceSeconds(120);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::PAUSED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersAtPause, g_mock.triggerBulbCalls);
    TEST_ASSERT_EQUAL_UINT16(1, astro().getStatus().completedFrames);

    astro().resume();
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INTERVAL),
                      static_cast<int>(astro().getStatus().state));

    // Finish the remaining 9 frames. Total triggers = 20 (10 frames * 2).
    advanceSeconds(9 * (30 + 3) + 10);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT16(10, astro().getStatus().completedFrames);
    TEST_ASSERT_EQUAL(20, g_mock.triggerBulbCalls);
}

// Stop while exposing must close the shutter (a second bulb toggle), not leave
// the camera holding the exposure open, and halt at STOPPED.
void test_stop_during_exposure_closes_shutter() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(2);  // into first exposure (open toggle fired)
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));

    int triggersBefore = g_mock.triggerBulbCalls;
    TEST_ASSERT_TRUE(g_mock.shutterActive);  // open toggle really opened it
    astro().stop();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersBefore + 1, g_mock.triggerBulbCalls);  // close toggle
    TEST_ASSERT_FALSE(g_mock.shutterActive);  // shutter actually closed
}

// Guard: if the shutter is already closed when stopExposure runs (e.g. camera
// closed it externally / state desync), do NOT toggle — a blind toggle would
// re-OPEN the shutter. stop() must leave the shutter closed regardless.
void test_stop_does_not_reopen_closed_shutter() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(2);  // EXPOSING, shutter open
    TEST_ASSERT_TRUE(g_mock.shutterActive);

    // Simulate the camera having already closed the shutter (external stop,
    // timeout, missed toggle) while the process still thinks it is exposing.
    g_mock.shutterActive = false;

    int triggersBefore = g_mock.triggerBulbCalls;
    astro().stop();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersBefore, g_mock.triggerBulbCalls);  // no toggle
    TEST_ASSERT_FALSE(g_mock.shutterActive);                     // still closed
}

// Guard: if the shutter is already OPEN when startExposure runs (desync), do
// NOT toggle — a blind toggle would CLOSE it and the frame would never expose.
// The already-open shutter is adopted as the current exposure.
void test_start_does_not_close_open_shutter() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;  // gives us a delay phase before the first frame
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();  // INITIAL_DELAY, shutter still closed

    // Simulate the shutter already being open when the first exposure begins.
    g_mock.shutterActive = true;

    int triggersBefore = g_mock.triggerBulbCalls;
    advanceSeconds(6);  // delay (5s) elapses -> first exposure starts

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(triggersBefore, g_mock.triggerBulbCalls);  // no open toggle
    TEST_ASSERT_TRUE(g_mock.shutterActive);                      // left open
}

// triggerBulb failure at exposure start drops the machine into ERROR.
void test_bulb_failure_errors() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);
    g_mock.triggerBulbShouldFail = true;

    astro().start();
    advanceSeconds(2);

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::ERROR),
                      static_cast<int>(astro().getStatus().state));
}

// Periodic status notifications throttle to 1 Hz, but state transitions always
// notify immediately. Ticking update() many times within one simulated second
// must yield at most one periodic callback for that second.
void test_status_notification_throttled() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 3;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    CountingObserver obs;
    astro().addObserver(&obs);  // addObserver fires one immediate callback
    int base = obs.statusCalls;

    // start() transitions IDLE -> INITIAL_DELAY: exactly one immediate notify.
    astro().start();
    TEST_ASSERT_EQUAL(base + 1, obs.statusCalls);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INITIAL_DELAY),
                      static_cast<int>(obs.lastState));

    // 100 ticks within the SAME second must not fire a periodic notify.
    int before = obs.statusCalls;
    for (int i = 0; i < 100; i++) {
        astro().update();  // clock not advanced
    }
    TEST_ASSERT_EQUAL(before, obs.statusCalls);

    // Advancing one second yields exactly one periodic notify (still in delay).
    advanceMillis(1000);
    astro().update();
    TEST_ASSERT_EQUAL(before + 1, obs.statusCalls);

    astro().removeObserver(&obs);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_start_rejects_invalid_params);
    RUN_TEST(test_start_requires_camera);
    RUN_TEST(test_setParameter_validation);
    RUN_TEST(test_full_sequence_completes);
    RUN_TEST(test_phase_remaining_counts_down_per_phase);
    RUN_TEST(test_pause_during_exposure_defers_until_frame_done);
    RUN_TEST(test_pause_during_interval_is_immediate);
    RUN_TEST(test_resume_continues_from_count);
    RUN_TEST(test_stop_during_exposure_closes_shutter);
    RUN_TEST(test_stop_does_not_reopen_closed_shutter);
    RUN_TEST(test_start_does_not_close_open_shutter);
    RUN_TEST(test_bulb_failure_errors);
    RUN_TEST(test_status_notification_throttled);
    return UNITY_END();
}
