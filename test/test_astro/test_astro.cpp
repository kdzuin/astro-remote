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
bool takeBulb() {
    g_mock.takeBulbCalls++;
    return !g_mock.takeBulbShouldFail;
}
bool emergencyStop() {
    g_mock.emergencyStopCalls++;
    return true;
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
    p.intervalSec = 1;
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

    // interval: range 1..60
    TEST_ASSERT_FALSE(astro().setParameter("intervalSec", 0));   // below min
    TEST_ASSERT_FALSE(astro().setParameter("intervalSec", 61));  // above max

    TEST_ASSERT_FALSE(astro().setParameter("bogus", 5));  // unknown parameter
}

// A full 10-frame sequence: assert it fires exactly one bulb per frame and
// ends STOPPED. delay=5, exposure=30, interval=1, frames=10 (min/step is 10).
void test_full_sequence_completes() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 1;
    astro().setParameters(p);

    astro().setCameraConnected(true);

    astro().start();
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INITIAL_DELAY),
                      static_cast<int>(astro().getStatus().state));

    // Total sequence: 5 (delay) + 10*(30 exposure + 1 interval) = 315s.
    // Run a bit past the end.
    advanceSeconds(330);

    TEST_ASSERT_EQUAL(10, g_mock.takeBulbCalls);
    TEST_ASSERT_EQUAL_UINT16(10, astro().getStatus().completedFrames);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
}

// Pause while exposing must release the shutter (emergencyStop) and hold state.
void test_pause_during_exposure_releases_shutter() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 1;
    astro().setParameters(p);
    astro().setCameraConnected(true);

    astro().start();
    advanceSeconds(2);  // into first exposure
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::EXPOSING),
                      static_cast<int>(astro().getStatus().state));

    int stopsBefore = g_mock.emergencyStopCalls;
    astro().pause();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::PAUSED),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL(stopsBefore + 1, g_mock.emergencyStopCalls);
}

// takeBulb failure at exposure start drops the machine into ERROR.
void test_bulb_failure_errors() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 10;
    p.intervalSec = 1;
    astro().setParameters(p);
    astro().setCameraConnected(true);
    g_mock.takeBulbShouldFail = true;

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
    p.intervalSec = 1;
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
    RUN_TEST(test_pause_during_exposure_releases_shutter);
    RUN_TEST(test_bulb_failure_errors);
    RUN_TEST(test_status_notification_throttled);
    return UNITY_END();
}
