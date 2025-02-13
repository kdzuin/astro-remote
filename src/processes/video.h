#pragma once

#include "transport/camera_commands.h"
#include <cstdio>

class VideoProcess {
public:
    VideoProcess() : recordStartTime(0) {}

    bool startRecording() {
        if (CameraCommands::recordStart()) {
            recordStartTime = millis();
            return true;
        }
        return false;
    }

    bool stopRecording() {
        if (CameraCommands::recordStop()) {
            recordStartTime = 0;
            return true;
        }
        return false;
    }

    bool isRecording() const {
        return CameraCommands::isRecording();
    }

    unsigned long getRecordingTime() const {
        if (!isRecording()) return 0;
        return (millis() - recordStartTime) / 1000;
    }

    void getFormattedTime(char* buffer, size_t bufferSize) const {
        unsigned long recordTime = getRecordingTime();
        int minutes = recordTime / 60;
        int seconds = recordTime % 60;
        snprintf(buffer, bufferSize, "%02d:%02d", minutes, seconds);
    }

private:
    unsigned long recordStartTime;
};