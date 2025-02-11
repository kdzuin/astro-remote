#include "sdl_hardware.h"
#include <stdexcept>

SDLDisplay::SDLDisplay(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font) {}

void SDLDisplay::fillScreen(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderClear(renderer);
}

void SDLDisplay::setCursor(int32_t x, int32_t y) {
    cursorX = x;
    cursorY = y;
}

void SDLDisplay::setTextSize(float size) {
    textSize = size;
}

void SDLDisplay::println(const char* text) {
    print(text);
    cursorY += 20 * textSize; // Approximate line height
    cursorX = 0;
}

void SDLDisplay::print(const char* text) {
    SDL_Color textColor = {255, 255, 255, 255}; // White text
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dest = {cursorX, cursorY, 
                           static_cast<int>(surface->w * textSize), 
                           static_cast<int>(surface->h * textSize)};
            SDL_RenderCopy(renderer, texture, NULL, &dest);
            cursorX += dest.w;
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

void SDLDisplay::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(renderer, &rect);
}

void SDLDisplay::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

bool SDLInput::isButtonPressed(int button) {
    return currentButtons[button];
}

bool SDLInput::wasButtonPressed(int button) {
    return currentButtons[button] && !previousButtons[button];
}

void SDLInput::update() {
    // Copy current to previous
    for (int i = 0; i < 3; i++) {
        previousButtons[i] = currentButtons[i];
    }

    // Update current button states from SDL
    const Uint8* state = SDL_GetKeyboardState(NULL);
    currentButtons[0] = state[SDL_SCANCODE_A];  // Button A
    currentButtons[1] = state[SDL_SCANCODE_B];  // Button B
    currentButtons[2] = state[SDL_SCANCODE_C];  // Button C
}

bool MockBLEDevice::connect(const std::string& address) {
    connected = true;
    return true;
}

void MockBLEDevice::disconnect() {
    connected = false;
}

bool MockBLEDevice::isConnected() {
    return connected;
}

void MockBLEDevice::scan() {
    scanning = true;
}

void MockBLEDevice::stopScan() {
    scanning = false;
}

bool MockBLEDevice::write(const uint8_t* data, size_t length) {
    return connected;
}

SDLHardware::SDLHardware() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL initialization failed");
    }

    if (TTF_Init() < 0) {
        SDL_Quit();
        throw std::runtime_error("SDL_ttf initialization failed");
    }

    window = SDL_CreateWindow("M5StickC Simulator",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            160, 80,  // M5StickC display size
                            SDL_WINDOW_SHOWN);
    if (!window) {
        TTF_Quit();
        SDL_Quit();
        throw std::runtime_error("Window creation failed");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        throw std::runtime_error("Renderer creation failed");
    }

    // Load a default font
    font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 16);
    if (!font) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        throw std::runtime_error("Font loading failed");
    }

    display = std::make_unique<SDLDisplay>(renderer, font);
    input = std::make_unique<SDLInput>();
    ble = std::make_unique<MockBLEDevice>();
}

SDLHardware::~SDLHardware() {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void SDLHardware::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // Handle quit
        }
    }

    input->update();
    SDL_RenderPresent(renderer);
}
