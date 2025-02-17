#pragma once

#include "processes/astro.h"
#include "transport/ble_remote_server.h"

class BLEAstroObserver : public AstroProcess::Observer {
public:
    static BLEAstroObserver& instance() {
        static BLEAstroObserver observer;
        return observer;
    }

    void onAstroParametersChanged(const AstroProcess::Parameters& params) override {
        // Currently we don't need to notify parameter changes via BLE
        // If needed in the future, we can add it here
    }
    
    void onAstroStatusChanged(const AstroProcess::Status& status) override {
        AstroStatusPacket packet;
        packet.state = static_cast<uint8_t>(status.state);
        packet.completedFrames = status.completedFrames;
        packet.elapsedSec = status.elapsedSec;
        packet.remainingSec = status.remainingSec;
        packet.isCameraConnected = status.isCameraConnected;
        packet.errorCode = status.errorCode;

        BLERemoteServer::sendAstroStatus(packet);
    }

private:
    BLEAstroObserver() = default;
    ~BLEAstroObserver() = default;
    BLEAstroObserver(const BLEAstroObserver&) = delete;
    BLEAstroObserver& operator=(const BLEAstroObserver&) = delete;
};
