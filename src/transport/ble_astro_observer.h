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
        LOG_DEBUG("[BLE] Parameters changed: exp=%ds, frames=%d, interval=%ds", params.exposureSec,
                  params.subframeCount, params.intervalSec);
    }

    void onAstroStatusChanged(const AstroProcess::Status& status) override {
        LOG_DEBUG("[BLE] Status changed: state=%d, frames=%d/%d, elapsed=%ds",
                  static_cast<int>(status.state), status.completedFrames + 1, status.totalFrames,
                  status.elapsedSec);

        AstroStatusPacket packet;
        packet.state = static_cast<uint8_t>(status.state);
        packet.completedFrames = status.completedFrames;
        packet.totalFrames = status.totalFrames;
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
