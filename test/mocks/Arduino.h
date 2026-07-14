// Native-build fake of the Arduino core.
// Only the surface used by the code-under-test is provided.
#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// ---- Controllable clock -----------------------------------------------------
// Tests advance time explicitly instead of relying on wall-clock, so a
// multi-minute astro sequence runs in microseconds and is deterministic.
extern uint32_t g_fakeMillis;
inline void setMillis(uint32_t ms) { g_fakeMillis = ms; }
inline void advanceMillis(uint32_t ms) { g_fakeMillis += ms; }
inline uint32_t millis() { return g_fakeMillis; }
inline void delay(uint32_t) {}  // no-op under native tests

// ---- Serial stub ------------------------------------------------------------
struct SerialStub {
    void begin(unsigned long) {}
    int printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int r = vprintf(fmt, args);
        va_end(args);
        return r;
    }
};
extern SerialStub Serial;
