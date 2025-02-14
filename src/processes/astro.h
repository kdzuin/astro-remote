#pragma once

#include <cstdint>
#include <string>
#include "transport/camera_commands.h"
#include "transport/ble_remote_server.h"

class AstroProcess {
public:
    enum class State {
        IDLE,           // No sequence running
        INITIAL_DELAY,  // Waiting before first exposure
        EXPOSING,       // Currently exposing
        INTERVAL,       // Waiting between exposures
        PAUSED,         // Sequence paused
        STOPPED,        // Sequence stopped
        ERROR          // Error state (e.g., camera disconnected)
    };

    struct Parameters {
        uint16_t initialDelaySec = 5;     // Initial delay in seconds (increment: 5s)
        uint16_t exposureSec = 60;        // Exposure time in seconds (increment: 30s)
        uint16_t subframeCount = 5;       // Number of exposures (increment: 5)
        uint16_t intervalSec = 5;         // Delay between exposures (increment: 1s)

        bool validate() const {
            return initialDelaySec % 5 == 0 &&
                   exposureSec % 30 == 0 &&
                   subframeCount % 5 == 0 &&
                   intervalSec >= 1;
        }

        uint32_t totalDurationSec() const {
            return initialDelaySec + 
                   subframeCount * (exposureSec + intervalSec);
        }
    };

    struct Status {
        State state = State::IDLE;
        uint16_t completedFrames = 0;
        uint32_t sequenceStartTime = 0;    // Unix timestamp
        uint32_t currentFrameStartTime = 0; // Unix timestamp
        uint32_t elapsedSec = 0;
        uint32_t remainingSec = 0;
        bool isCameraConnected = false;
        uint8_t errorCode = 0;
    };

    // Singleton access
    static AstroProcess& instance();

    // Command interface
    void start();
    void pause();
    void stop();
    void reset();
    
    // Parameter management
    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return params_; }
    bool setParameter(const std::string& name, uint16_t value);

    // Status access
    const Status& getStatus() const { return status_; }
    bool isRunning() const { 
        return status_.state != State::IDLE && 
               status_.state != State::STOPPED && 
               status_.state != State::ERROR; 
    }

    // Update loop - call this regularly
    void update();

private:
    AstroProcess() = default;
    ~AstroProcess() = default;
    AstroProcess(const AstroProcess&) = delete;
    AstroProcess& operator=(const AstroProcess&) = delete;

    // State management
    void setState(State newState);
    void updateTimings();
    void notifyStateChange();
    bool startExposure();
    void stopExposure();
    
    // Member variables
    Parameters params_;
    Status status_;
    uint32_t lastUpdateTime_ = 0;
    bool exposureActive_ = false;
};