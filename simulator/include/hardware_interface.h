#pragma once

#include <cstdint>
#include <string>

// Basic display interface
class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void fillScreen(uint32_t color) = 0;
    virtual void setCursor(int32_t x, int32_t y) = 0;
    virtual void setTextSize(float size) = 0;
    virtual void println(const char* text) = 0;
    virtual void print(const char* text) = 0;
    virtual void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) = 0;
    virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) = 0;
};

// Basic input interface
class IInput {
public:
    virtual ~IInput() = default;
    virtual bool isButtonPressed(int button) = 0;
    virtual bool wasButtonPressed(int button) = 0;
    virtual void update() = 0;
};

// Basic BLE interface
class IBLEDevice {
public:
    virtual ~IBLEDevice() = default;
    virtual bool connect(const std::string& address) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() = 0;
    virtual void scan() = 0;
    virtual void stopScan() = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
};

// Hardware manager interface that combines all hardware access
class IHardware {
public:
    virtual ~IHardware() = default;
    virtual IDisplay& getDisplay() = 0;
    virtual IInput& getInput() = 0;
    virtual IBLEDevice& getBLE() = 0;
    virtual void update() = 0;
};
