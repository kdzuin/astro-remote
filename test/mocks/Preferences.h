#pragma once
#include <cstdint>
#include <string>
class Preferences {
public:
    bool begin(const char*, bool = false) { return true; }
    void end() {}
    void clear() {}
    size_t putString(const char*, const char*) { return 0; }
    std::string getString(const char*, const char* def = "") { return def; }
    bool putBool(const char*, bool) { return true; }
    bool getBool(const char*, bool def = false) { return def; }
    uint32_t putUChar(const char*, uint8_t) { return 0; }
    uint8_t getUChar(const char*, uint8_t def = 0) { return def; }
    bool isKey(const char*) { return false; }
    bool remove(const char*) { return true; }
};
