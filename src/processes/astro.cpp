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
    pausePending_ = false;
    updateTimings();
    setState(State::INITIAL_DELAY);
}

void AstroProcess::pause() {
    if (!isRunning() || status_.state == State::PAUSED) {
        return;
    }

    if (status_.state == State::EXPOSING) {
        // Deferred pause: never cut a frame short. Flag it and let update()
        // park in PAUSED once the current exposure finishes normally.
        pausePending_ = true;
        return;
    }

    // Delay/interval: shutter is closed, so pause immediately.
    pausedAtSec_ = millis() / 1000;
    setState(State::PAUSED);
}

void AstroProcess::resume() {
    if (status_.state != State::PAUSED) {
        return;
    }

    // Shift the timeline forward by however long we were paused, so elapsed and
    // per-frame timing stay continuous, then wait out a fresh interval before
    // the next frame. completedFrames is preserved (no restart).
    uint32_t now = millis() / 1000;
    uint32_t pausedDuration = now - pausedAtSec_;
    status_.sequenceStartTime += pausedDuration;
    status_.currentFrameStartTime = now;
    setState(State::INTERVAL);
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
    pausePending_ = false;
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
    if (isRunning()) {
        return false;  // Can't modify parameters while running
    }

    Parameters newParams = params_;  // Validate on a copy before applying
    if (name == "initialDelaySec") {
        newParams.initialDelaySec = value;
    } else if (name == "exposureSec") {
        newParams.exposureSec = value;
    } else if (name == "subframeCount") {
        newParams.subframeCount = value;
    } else if (name == "intervalSec") {
        newParams.intervalSec = value;
    } else {
        return false;  // Unknown parameter
    }

    if (!newParams.validate()) {
        return false;  // Reject invalid value, leave params unchanged
    }

    params_ = newParams;
    status_.totalFrames = params_.subframeCount;
    updateTimings();
    notifyParametersChanged();
    return true;
}

void AstroProcess::update() {
    uint32_t currentTime = millis() / 1000;  // Convert to seconds
    uint32_t deltaTime = currentTime - lastUpdateTime_;
    lastUpdateTime_ = currentTime;

    if (!isRunning())
        return;

    // While PAUSED the clock is frozen: don't advance timings or the state
    // machine. resume() shifts sequenceStartTime past the paused span so the
    // elapsed/remaining figures stay continuous.
    if (status_.state == State::PAUSED) {
        return;
    }

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
                } else if (pausePending_) {
                    // Deferred pause takes effect now that the frame is done.
                    pausePending_ = false;
                    pausedAtSec_ = currentTime;
                    setState(State::PAUSED);
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

    // Periodic status notification, throttled to 1 Hz. State transitions above
    // notify immediately via setState(), so this only covers the ticking
    // elapsed/remaining timers while the state is unchanged.
    if (currentTime != lastNotifySec_) {
        lastNotifySec_ = currentTime;
        notifyStatusObservers();
    }
}

void AstroProcess::setState(State newState) {
    if (status_.state != newState) {
        status_.state = newState;
        // A transition is always important — notify immediately and reset the
        // throttle window so the next periodic tick doesn't double-fire.
        lastNotifySec_ = millis() / 1000;
        notifyStatusObservers();
    }
}

void AstroProcess::updateTimings() {
    if (status_.state == State::IDLE || status_.state == State::STOPPED) {
        status_.remainingSec = 0;
        status_.phaseRemainingSec = 0;
        status_.phaseTotalSec = 0;
        return;
    }

    // Whole-series remaining.
    uint32_t totalTime = params_.getTotalDurationSec();
    if (status_.elapsedSec > totalTime) {
        status_.remainingSec = 0;
    } else {
        status_.remainingSec = totalTime - status_.elapsedSec;
    }

    // Time left in the current phase. currentTime is reconstructed from the
    // just-updated elapsedSec (= currentTime - sequenceStartTime) so this stays
    // a pure function of status_/params_.
    uint32_t currentTime = status_.sequenceStartTime + status_.elapsedSec;
    switch (status_.state) {
        case State::INITIAL_DELAY:
            status_.phaseTotalSec = params_.initialDelaySec;
            status_.phaseRemainingSec = status_.elapsedSec < params_.initialDelaySec
                                            ? params_.initialDelaySec - status_.elapsedSec
                                            : 0;
            break;
        case State::EXPOSING: {
            uint32_t e = currentTime - status_.currentFrameStartTime;
            status_.phaseTotalSec = params_.exposureSec;
            status_.phaseRemainingSec = e < params_.exposureSec ? params_.exposureSec - e : 0;
            break;
        }
        case State::INTERVAL: {
            uint32_t e = currentTime - status_.currentFrameStartTime;
            status_.phaseTotalSec = params_.intervalSec;
            status_.phaseRemainingSec = e < params_.intervalSec ? params_.intervalSec - e : 0;
            break;
        }
        default:  // PAUSED, etc. — leave frozen.
            break;
    }
}

void AstroProcess::notifyParametersChanged() {
    for (auto* observer : observers_) {
        observer->onAstroParametersChanged(params_);
    }
}

void AstroProcess::setCameraConnected(bool connected) {
    if (status_.isCameraConnected == connected) {
        return;
    }
    status_.isCameraConnected = connected;
    // Push the change out now; while idle there are no periodic notifications,
    // so without this the remote link would show a stale camera state.
    notifyStatusObservers();
}

void AstroProcess::notifyStatusObservers() {
    for (auto* observer : observers_) {
        observer->onAstroStatusChanged(status_);
    }
}

bool AstroProcess::startExposure() {
    if (!status_.isCameraConnected)
        return false;

    status_.currentFrameStartTime = millis() / 1000;

    // Bulb is a toggle: only open if the shutter is actually closed. If it is
    // already open (desync), toggling would CLOSE it and the frame would never
    // expose — so adopt the open shutter as this exposure instead.
    if (!CameraCommands::isShutterActive()) {
        if (!CameraCommands::triggerBulb()) {  // open toggle
            status_.errorCode = 3;             // Failed to start exposure
            return false;
        }
    }
    exposureActive_ = true;
    return true;
}

void AstroProcess::stopExposure() {
    // Bulb is a toggle: a second trigger closes the exposure the open toggle
    // started. Guard on the camera's reported shutter state — if it is already
    // closed (external stop, timeout, missed toggle), toggling again would
    // re-OPEN it, so skip. Only toggle when the shutter is actually open.
    if (exposureActive_) {
        if (CameraCommands::isShutterActive()) {
            CameraCommands::triggerBulb();  // close toggle
        }
        exposureActive_ = false;
    }
}
