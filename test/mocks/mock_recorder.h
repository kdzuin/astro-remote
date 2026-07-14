// Records calls made by AstroProcess into its collaborators, so tests can
// assert on them (how many bulb exposures fired, last status pushed, etc.).
#pragma once

#include <cstdint>
#include <vector>

#include "transport/ble_remote_server.h"  // real AstroStatusPacket definition

struct AstroMockState {
    // CameraCommands
    int triggerBulbCalls = 0;
    int emergencyStopCalls = 0;
    bool triggerBulbShouldFail = false;
    bool shutterActive = false;  // Simulated camera shutter state (toggled by triggerBulb).

    // BLERemoteServer::sendAstroStatus capture
    int sendAstroStatusCalls = 0;
    AstroStatusPacket lastStatus{};

    void reset() { *this = AstroMockState{}; }
};

extern AstroMockState g_mock;
