#pragma once

#include <cstdint>
#include <string>

#include "transport/ble_remote_server.h"
#include "transport/camera_commands.h"

class AstroProcess {
public:
    enum class State {
        IDLE,           // No sequence running
        INITIAL_DELAY,  // Waiting before first exposure
        EXPOSING,       // Currently exposing
        INTERVAL,       // Waiting between exposures
        PAUSED,         // Sequence paused
        STOPPED,        // Sequence stopped
        ERROR           // Error state (e.g., camera disconnected)
    };

    struct Parameters {
        // Parameter constraints
        static constexpr uint16_t INITIAL_DELAY_DEFAULT = 5;
        static constexpr uint16_t INITIAL_DELAY_STEP = 5;

        static constexpr uint16_t EXPOSURE_DEFAULT = 60;
        static constexpr uint16_t EXPOSURE_STEP = 30;
        static constexpr uint16_t EXPOSURE_MIN = 30;
        static constexpr uint16_t EXPOSURE_MAX = 600;

        static constexpr uint16_t SUBFRAME_COUNT_DEFAULT = 10;
        static constexpr uint16_t SUBFRAME_COUNT_STEP = 10;
        static constexpr uint16_t SUBFRAME_COUNT_MIN = 10;
        static constexpr uint16_t SUBFRAME_COUNT_MAX = 480;

        static constexpr uint16_t INTERVAL_DEFAULT = 5;
        static constexpr uint16_t INTERVAL_STEP = 1;
        static constexpr uint16_t INTERVAL_MIN = 1;
        static constexpr uint16_t INTERVAL_MAX = 60;

        // Parameter values
        uint16_t initialDelaySec = INITIAL_DELAY_DEFAULT;  // Initial delay in seconds
        uint16_t exposureSec = EXPOSURE_DEFAULT;           // Exposure time in seconds
        uint16_t subframeCount = SUBFRAME_COUNT_DEFAULT;   // Number of exposures
        uint16_t intervalSec = INTERVAL_DEFAULT;           // Delay between exposures

        bool validate() const {
            return initialDelaySec % INITIAL_DELAY_STEP == 0 && exposureSec >= EXPOSURE_MIN &&
                   exposureSec <= EXPOSURE_MAX && exposureSec % EXPOSURE_STEP == 0 &&
                   subframeCount >= SUBFRAME_COUNT_MIN && subframeCount <= SUBFRAME_COUNT_MAX &&
                   subframeCount % SUBFRAME_COUNT_STEP == 0 && intervalSec >= INTERVAL_MIN &&
                   intervalSec <= INTERVAL_MAX;
        }

        uint32_t getTotalDurationSec() const {
            return initialDelaySec + subframeCount * (exposureSec + intervalSec);
        }
    };

    struct Status {
        State state = State::IDLE;
        uint16_t completedFrames = 0;
        uint32_t sequenceStartTime = 0;      // Unix timestamp
        uint32_t currentFrameStartTime = 0;  // Unix timestamp
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
        return status_.state != State::IDLE && status_.state != State::STOPPED &&
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