#pragma once

#include "transport/camera_commands.h"

class PhotoProcess {
public:
    PhotoProcess() : photoCount(0), flashStartTime(0) {}

    bool takePhoto() {
        if (CameraCommands::takePhoto()) {
            photoCount++;
            flashStartTime = millis();
            return true;
        }
        return false;
    }

    int getPhotoCount() const {
        return photoCount;
    }

    bool isFlashActive() const {
        return (millis() - flashStartTime < 200);
    }

    bool shouldClearFlash() const {
        return (millis() - flashStartTime > 200 && flashStartTime > 0);
    }

    void clearFlash() {
        flashStartTime = 0;
    }

private:
    int photoCount;
    unsigned long flashStartTime;
};