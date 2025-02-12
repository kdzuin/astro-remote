#pragma once
#include <Arduino.h>

#include "transport/button_id.h"

typedef uint32_t unifiedColor;

struct RGBColorTriple {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

namespace textAlign {
enum TextDatum : uint8_t {
    top_left = 0,
    top_center = 1,
    top_right = 2,
    middle_left = 4,
    middle_center = 5,
    middle_right = 6,
    bottom_left = 8,
    bottom_center = 9,
    bottom_right = 10,
    baseline_left = 16,
    baseline_center = 17,
    baseline_right = 18
};
}

// Basic display interface
class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void setRotation(uint8_t r) = 0;
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual void setClipRect(int32_t x, int32_t y, int32_t w, int32_t h) = 0;
    virtual void clearClipRect() = 0;
    virtual void setTextAlignment(textAlign::TextDatum datum) = 0;
    virtual void setTextSize(float size) = 0;
    virtual void setTextColor(unifiedColor color) = 0;
    virtual void drawString(const char* text, int32_t x, int32_t y) = 0;
    virtual void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, unifiedColor color) = 0;
    virtual void fillScreen(unifiedColor color) = 0;
    virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, unifiedColor color) = 0;
    virtual unifiedColor getColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual unifiedColor getColor(const RGBColorTriple& color) {
        return getColor(color.r, color.g, color.b);
    };
};

// Basic input interface
class IInput {
public:
    virtual ~IInput() = default;
    virtual bool isButtonPressed(ButtonId button) = 0;
    virtual bool wasButtonPressed(ButtonId button) = 0;
    virtual void update() = 0;
};

// Hardware manager interface
class IHardware {
public:
    virtual ~IHardware() = default;
    virtual void begin() = 0;
    virtual IDisplay& getDisplay() = 0;
    virtual IInput& getInput() = 0;
    virtual void update() = 0;
};
