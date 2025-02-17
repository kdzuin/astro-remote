#include "processes/astro.h"

#include <Arduino.h>

#include "transport/ble_astro_observer.h"

void AstroProcess::initializeObservers() {
    // Register BLE observer
    addObserver(&BLEAstroObserver::instance());
}

void AstroProcess::start() {
    if (!params_.validate()) {
        status_.errorCode = 1;  // Invalid parameters
        setState(State::ERROR);
        return;
    }

    if (!status_.isCameraConnected) {
        status_.errorCode = 2;  // Camera not connected
        setState(State::ERROR);
        return;
    }

    status_.sequenceStartTime = millis() / 1000;  // Convert to seconds
    status_.completedFrames = 0;
    status_.totalFrames = params_.subframeCount;
    updateTimings();
    setState(State::INITIAL_DELAY);
}

void AstroProcess::pause() {
    if (isRunning()) {
        if (status_.state == State::EXPOSING) {
            stopExposure();
        }
        setState(State::PAUSED);
    }
}

void AstroProcess::stop() {
    if (isRunning()) {
        if (status_.state == State::EXPOSING) {
            stopExposure();
        }
        setState(State::STOPPED);
    }
}

void AstroProcess::reset() {
    stopExposure();
    status_.completedFrames = 0;
    status_.sequenceStartTime = 0;
    status_.currentFrameStartTime = 0;
    status_.elapsedSec = 0;
    status_.remainingSec = 0;
    status_.totalFrames = params_.subframeCount;
    status_.errorCode = 0;
    setState(State::IDLE);
}

void AstroProcess::setParameters(const Parameters& params) {
    if (status_.state == State::IDLE || status_.state == State::STOPPED) {
        params_ = params;
        updateTimings();
        notifyParametersChanged();
    }
}

bool AstroProcess::setParameter(const std::string& name, uint16_t value) {
    bool changed = false;
    if (name == "initialDelaySec") {
        params_.initialDelaySec = value;
        changed = true;
    } else if (name == "exposureSec") {
        params_.exposureSec = value;
        changed = true;
    } else if (name == "subframeCount") {
        params_.subframeCount = value;
        status_.totalFrames = value;  // Update total frames in status
        changed = true;
    } else if (name == "intervalSec") {
        params_.intervalSec = value;
        changed = true;
    } else if (name == "subframeCount") {
        params_.subframeCount = value;
    } else {
        return false;  // Unknown parameter
    }

    if (changed) {
        updateTimings();
        notifyParametersChanged();
    }
    return changed;
}

void AstroProcess::update() {
    uint32_t currentTime = millis() / 1000;  // Convert to seconds
    uint32_t deltaTime = currentTime - lastUpdateTime_;
    lastUpdateTime_ = currentTime;

    if (!isRunning())
        return;

    // Update timings
    status_.elapsedSec = currentTime - status_.sequenceStartTime;
    updateTimings();

    // State machine
    switch (status_.state) {
        case State::INITIAL_DELAY:
            if (status_.elapsedSec >= params_.initialDelaySec) {
                if (!startExposure()) {
                    setState(State::ERROR);
                    return;
                }
                setState(State::EXPOSING);
            }
            break;

        case State::EXPOSING: {
            uint32_t exposureElapsed = currentTime - status_.currentFrameStartTime;
            if (exposureElapsed >= params_.exposureSec) {
                stopExposure();
                status_.completedFrames++;

                if (status_.completedFrames >= params_.subframeCount) {
                    setState(State::STOPPED);
                } else {
                    status_.currentFrameStartTime = currentTime;
                    setState(State::INTERVAL);
                }
            }
            break;
        }

        case State::INTERVAL: {
            uint32_t intervalElapsed = currentTime - status_.currentFrameStartTime;
            if (intervalElapsed >= params_.intervalSec) {
                if (!startExposure()) {
                    setState(State::ERROR);
                    return;
                }
                setState(State::EXPOSING);
            }
            break;
        }

        default:
            break;
    }

    // Notify state changes
    notifyStateChange();
}

void AstroProcess::setState(State newState) {
    if (status_.state != newState) {
        status_.state = newState;
        notifyStateChange();
    }
}

void AstroProcess::updateTimings() {
    if (status_.state == State::IDLE || status_.state == State::STOPPED) {
        status_.remainingSec = 0;
        return;
    }

    uint32_t totalTime = params_.getTotalDurationSec();
    if (status_.elapsedSec > totalTime) {
        status_.remainingSec = 0;
    } else {
        status_.remainingSec = totalTime - status_.elapsedSec;
    }
}

void AstroProcess::notifyParametersChanged() {
    for (auto* observer : observers_) {
        observer->onAstroParametersChanged(params_);
    }
}

void AstroProcess::notifyStateChange() {
    AstroStatusPacket packet;
    packet.state = static_cast<uint8_t>(status_.state);
    packet.completedFrames = status_.completedFrames;
    packet.totalFrames = status_.totalFrames;
    packet.sequenceStartTime = status_.sequenceStartTime;
    packet.currentFrameStartTime = status_.currentFrameStartTime;
    packet.elapsedSec = status_.elapsedSec;
    packet.remainingSec = status_.remainingSec;
    packet.isCameraConnected = status_.isCameraConnected;
    packet.errorCode = status_.errorCode;

    for (auto* observer : observers_) {
        observer->onAstroStatusChanged(status_);
    }
}

bool AstroProcess::startExposure() {
    if (!status_.isCameraConnected)
        return false;

    status_.currentFrameStartTime = millis() / 1000;
    if (!CameraCommands::takeBulb()) {
        status_.errorCode = 3;  // Failed to start exposure
        return false;
    }
    exposureActive_ = true;
    return true;
}

void AstroProcess::stopExposure() {
    if (exposureActive_) {
        CameraCommands::emergencyStop();
        exposureActive_ = false;
    }
}
