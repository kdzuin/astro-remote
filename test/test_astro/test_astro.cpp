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
    p.subframeCount = 5;
    p.intervalSec = 1;
    astro().setParameters(p);
    // isCameraConnected defaults false

    astro().start();

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::ERROR),
                      static_cast<int>(astro().getStatus().state));
    TEST_ASSERT_EQUAL_UINT8(2, astro().getStatus().errorCode);
}

// setParameter enforces the validation rules (mult of 5 / 30, interval >= 1).
void test_setParameter_validation() {
    TEST_ASSERT_TRUE(astro().setParameter("exposureSec", 60));
    TEST_ASSERT_FALSE(astro().setParameter("exposureSec", 61));
    TEST_ASSERT_TRUE(astro().setParameter("initialDelaySec", 10));
    TEST_ASSERT_FALSE(astro().setParameter("initialDelaySec", 7));
    TEST_ASSERT_FALSE(astro().setParameter("intervalSec", 0));
    TEST_ASSERT_FALSE(astro().setParameter("bogus", 5));
}

// A full 5-frame sequence: assert it fires exactly one bulb per frame and ends
// STOPPED. delay=5, exposure=30, interval=1, frames=5 (must be mult of 5).
void test_full_sequence_completes() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 5;
    p.exposureSec = 30;
    p.subframeCount = 5;
    p.intervalSec = 1;
    astro().setParameters(p);

    // NOTE: production code never sets isCameraConnected (read-only gap — see
    // review note). Tests force it via the const ref until a setter exists.
    const_cast<AstroProcess::Status&>(astro().getStatus()).isCameraConnected = true;

    astro().start();
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::INITIAL_DELAY),
                      static_cast<int>(astro().getStatus().state));

    // Total sequence: 5 (delay) + 5*(30 exposure + 1 interval) = 160s.
    // Run a bit past the end.
    advanceSeconds(170);

    TEST_ASSERT_EQUAL(5, g_mock.takeBulbCalls);
    TEST_ASSERT_EQUAL_UINT16(5, astro().getStatus().completedFrames);
    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::STOPPED),
                      static_cast<int>(astro().getStatus().state));
}

// Pause while exposing must release the shutter (emergencyStop) and hold state.
void test_pause_during_exposure_releases_shutter() {
    AstroProcess::Parameters p;
    p.initialDelaySec = 0;
    p.exposureSec = 30;
    p.subframeCount = 5;
    p.intervalSec = 1;
    astro().setParameters(p);
    const_cast<AstroProcess::Status&>(astro().getStatus()).isCameraConnected = true;

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
    p.subframeCount = 5;
    p.intervalSec = 1;
    astro().setParameters(p);
    const_cast<AstroProcess::Status&>(astro().getStatus()).isCameraConnected = true;
    g_mock.takeBulbShouldFail = true;

    astro().start();
    advanceSeconds(2);

    TEST_ASSERT_EQUAL(static_cast<int>(AstroProcess::State::ERROR),
                      static_cast<int>(astro().getStatus().state));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_start_rejects_invalid_params);
    RUN_TEST(test_start_requires_camera);
    RUN_TEST(test_setParameter_validation);
    RUN_TEST(test_full_sequence_completes);
    RUN_TEST(test_pause_during_exposure_releases_shutter);
    RUN_TEST(test_bulb_failure_errors);
    return UNITY_END();
}
