#pragma once

#include <cstddef>
#include <string>
#include <vector>

// A remembered Sony camera: its BLE address plus the advertised name captured
// when it was first paired. (Glossary: "Saved camera".)
struct SavedCamera {
    std::string address;
    std::string name;
};

// Pure in-memory model of the remote's remembered cameras and which one is
// active. Holds no hardware or NVS dependency, so it is unit-testable on the
// host; BLEDeviceManager owns one instance and serializes it to NVS separately.
//
// See ADR 0007. Glossary: "Saved camera", "Active camera", "Camera list".
class CameraStore {
public:
    static constexpr size_t MAX_CAMERAS = 8;

    // Insert a camera if its address is not already known. If the address is
    // already present, the entry is left in place but a non-empty `name` fills
    // a previously-empty name (a name is never clobbered with an empty string).
    // Returns true only when a new entry was appended. Returns false if the
    // address already existed or the store is full.
    bool add(const std::string& address, const std::string& name);

    // Remove a camera by address. If it was the active camera, active is
    // cleared (no auto-pick). No-op if the address is unknown.
    void forget(const std::string& address);

    // Point "active" at a saved camera. Ignored if the address is not saved.
    void setActive(const std::string& address);

    bool hasActive() const { return !activeAddr.empty(); }
    const std::string& activeAddress() const { return activeAddr; }

    const SavedCamera* find(const std::string& address) const;
    const std::vector<SavedCamera>& cameras() const { return list; }

    size_t count() const { return list.size(); }
    bool isFull() const { return list.size() >= MAX_CAMERAS; }

    void clear() {
        list.clear();
        activeAddr.clear();
    }

private:
    std::vector<SavedCamera> list;
    std::string activeAddr;

    SavedCamera* findMutable(const std::string& address);
};
