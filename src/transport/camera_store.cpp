#include "transport/camera_store.h"

SavedCamera* CameraStore::findMutable(const std::string& address) {
    for (auto& cam : list) {
        if (cam.address == address) {
            return &cam;
        }
    }
    return nullptr;
}

const SavedCamera* CameraStore::find(const std::string& address) const {
    for (const auto& cam : list) {
        if (cam.address == address) {
            return &cam;
        }
    }
    return nullptr;
}

bool CameraStore::add(const std::string& address, const std::string& name) {
    if (SavedCamera* existing = findMutable(address)) {
        // Known camera: fill an empty name, but never clobber a saved one.
        if (existing->name.empty() && !name.empty()) {
            existing->name = name;
        }
        return false;
    }
    if (isFull()) {
        return false;
    }
    list.push_back(SavedCamera{address, name});
    return true;
}

void CameraStore::forget(const std::string& address) {
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->address == address) {
            list.erase(it);
            if (activeAddr == address) {
                activeAddr.clear();
            }
            return;
        }
    }
}

void CameraStore::setActive(const std::string& address) {
    if (find(address)) {
        activeAddr = address;
    }
}
