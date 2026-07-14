// Native-build fake of M5Unified. Only symbols referenced by code-under-test.
#pragma once
#include "Arduino.h"
#include <cstdint>

enum textdatum_t { middle_center, middle_left, middle_right };

// Minimal display: color helpers must return, draw ops are no-ops.
struct DisplayStub {
    uint32_t color888(uint8_t r, uint8_t g, uint8_t b) {
        return (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
    }
    int width() { return 80; }
    int height() { return 160; }
    void fillScreen(uint32_t) {}
    void fillRect(int, int, int, int, uint32_t) {}
    void drawLine(int, int, int, int, uint32_t) {}
    void drawString(const char*, int, int) {}
    void setTextSize(float) {}
    void setTextColor(uint32_t) {}
    void setTextDatum(textdatum_t) {}
};
struct M5Stub {
    DisplayStub Display;
};
extern M5Stub M5;
