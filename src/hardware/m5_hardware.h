#pragma once

#include <M5Unified.h>

#include "hardware_interface.h"

class M5DisplayWrapper : public IDisplay {
public:
    void fillScreen(unifiedColor color) override { M5.Display.fillScreen(color); }
    void setTextSize(float size) override { M5.Display.setTextSize(size); }
    void setTextColor(unifiedColor color) override { M5.Display.setTextColor(color); }
    void setBrightness(uint8_t brightness) override { M5.Display.setBrightness(brightness); }
    void setClipRect(int32_t x, int32_t y, int32_t w, int32_t h) override {
        M5.Display.setClipRect(x, y, w, h);
    }
    void clearClipRect() override { M5.Display.clearClipRect(); }
    void setRotation(uint8_t r) override { M5.Display.setRotation(r); }
    int32_t width() const override { return M5.Display.width(); }
    int32_t height() const override { return M5.Display.height(); }
    void setTextAlignment(textAlign::TextDatum datum) override {
        M5.Display.setTextDatum(static_cast<lgfx::v1::textdatum::textdatum_t>(datum));
    }
    void drawString(const char* text, int32_t x, int32_t y) override {
        M5.Display.drawString(text, x, y);
    }
    void drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, unifiedColor color) override {
        M5.Display.drawLine(x1, y1, x2, y2, color);
    }
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, unifiedColor color) override {
        M5.Display.fillRect(x, y, w, h, color);
    }
    unifiedColor getColor(uint8_t r, uint8_t g, uint8_t b) override {
        return M5.Display.color888(r, g, b);
    }
};

class M5InputWrapper : public IInput {
public:
    bool isButtonPressed(ButtonId button) override {
        switch (button) {
            case ButtonId::BTN_A:
                return M5.BtnA.isPressed();
            case ButtonId::BTN_B:
                return M5.BtnB.isPressed();
            case ButtonId::BTN_PWR:
                return M5.BtnPWR.isPressed();
            default:
                return false;
        }
    }
    bool wasButtonPressed(ButtonId button) override {
        bool result = false;
        switch (button) {
            case ButtonId::BTN_A:
                result = M5.BtnA.wasClicked();
                if (result)
                    LOG_DEBUG("[M5Input] BTN_A wasClicked event!");
                break;
            case ButtonId::BTN_B:
                result = M5.BtnB.wasClicked();
                if (result)
                    LOG_DEBUG("[M5Input] BTN_B wasClicked event!");
                break;
            case ButtonId::BTN_PWR:
                result = M5.BtnPWR.wasClicked();
                if (result)
                    LOG_DEBUG("[M5Input] BTN_PWR wasClicked event!");
                break;
            default:
                break;
        }
        return result;
    }

    void update() override {
        static uint32_t lastDebugTime = 0;
        const uint32_t now = millis();

        // Log button states every 2 seconds, but only if any button is pressed
        if (now - lastDebugTime >= 2000) {
            bool anyPressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnPWR.isPressed();

            if (anyPressed) {
                LOG_DEBUG("[M5Input] Held - A:%d B:%d PWR:%d", M5.BtnA.isPressed(),
                          M5.BtnB.isPressed(), M5.BtnPWR.isPressed());
            }
            lastDebugTime = now;
        }
    }
};

class M5Hardware : public IHardware {
public:
    void begin() override {
        auto cfg = M5.config();
        M5.begin(cfg);
        Serial.begin(115200);
    }

    IDisplay& getDisplay() override { return display; }
    IInput& getInput() override { return input; }
    void update() override {
        M5.update();     // Single source of truth for M5 updates
        input.update();  // Just for debug logging
    }

private:
    M5DisplayWrapper display;
    M5InputWrapper input;
};
