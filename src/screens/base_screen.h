#pragma once

#include <M5Unified.h>

#include <functional>
#include <string>
#include <vector>

#include "components/menu_system.h"
#include "components/selectable_list.h"
#include "transport/ble_device.h"
#include "utils/colors.h"

// Base menu item type for screens that don't define their own
enum class BaseMenuItem { None };

template <typename MenuItemType>
class BaseScreen {
public:
    BaseScreen(const char* name);
    virtual ~BaseScreen() = default;

    // Pure virtual functions that must be implemented by derived screens
    virtual void update() = 0;
    virtual void beforeExit() {};
    virtual void updateMenuItems() = 0;
    virtual void selectMenuItem() = 0;
    virtual void nextMenuItem() = 0;
    virtual void prevMenuItem() = 0;

    const int STATUS_BAR_HEIGHT = 20;

    // Base draw implementation with status bar
    virtual void draw();

    void drawConnectionStatus() const;
    void drawStatusBar() const;

    // New pure virtual function for content drawing
    virtual void drawContent() = 0;

    const char* getName() const { return screenName; }

    // Status bar methods
    void setStatusText(const std::string& text) { statusText = text; }
    void setStatusBgColor(uint32_t color) { statusBgColor = color; }

    void checkConnection();

protected:
    SelectableList<MenuItemType> menuItems;
    const char* screenName;
    std::string statusText;
    uint32_t statusBgColor;
    unsigned long lastConnectionCheck = 0;
    bool wasConnected = false;
    int reconnectAttempts = 0;
};

#include "base_screen.tpp"
