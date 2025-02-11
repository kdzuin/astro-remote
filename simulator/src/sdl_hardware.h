#pragma once

#include "hardware_interface.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <memory>

class SDLDisplay : public IDisplay {
public:
    SDLDisplay(SDL_Renderer* renderer, TTF_Font* font);
    void fillScreen(uint32_t color) override;
    void setCursor(int32_t x, int32_t y) override;
    void setTextSize(float size) override;
    void println(const char* text) override;
    void print(const char* text) override;
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    int32_t cursorX = 0;
    int32_t cursorY = 0;
    float textSize = 1.0f;
};

class SDLInput : public IInput {
public:
    bool isButtonPressed(int button) override;
    bool wasButtonPressed(int button) override;
    void update() override;

private:
    bool currentButtons[3] = {false};
    bool previousButtons[3] = {false};
};

class MockBLEDevice : public IBLEDevice {
public:
    bool connect(const std::string& address) override;
    void disconnect() override;
    bool isConnected() override;
    void scan() override;
    void stopScan() override;
    bool write(const uint8_t* data, size_t length) override;

private:
    bool connected = false;
    bool scanning = false;
};

class SDLHardware : public IHardware {
public:
    SDLHardware();
    ~SDLHardware();

    IDisplay& getDisplay() override { return *display; }
    IInput& getInput() override { return *input; }
    IBLEDevice& getBLE() override { return *ble; }
    void update() override;

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    std::unique_ptr<SDLDisplay> display;
    std::unique_ptr<SDLInput> input;
    std::unique_ptr<MockBLEDevice> ble;
};
