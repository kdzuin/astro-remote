#pragma once

#include <memory>
#include "screens/base_screen.h"

// Forward declarations
template<typename T>
class BaseScreen;
class MainScreen;

// Type-erased interface for screens
class IScreen {
public:
    virtual ~IScreen() = default;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual const char* getName() const = 0;
};

// Template wrapper that implements IScreen
template<typename MenuType>
class ScreenWrapper : public IScreen {
public:
    ScreenWrapper(BaseScreen<MenuType>* screen) : screen_(screen) {}
    ~ScreenWrapper() { delete screen_; }
    
    void update() override { screen_->update(); }
    void draw() override { screen_->draw(); }
    const char* getName() const override { return screen_->getName(); }
    
    BaseScreen<MenuType>* get() { return screen_; }

private:
    BaseScreen<MenuType>* screen_;
};

namespace MenuSystem
{
    // Function declarations
    void init();
    void update();
    void setScreenInternal(IScreen* screen);
    IScreen* getCurrentScreen();
    
    // Generic screen setter that can accept any screen type
    template<typename MenuType>
    void setScreen(BaseScreen<MenuType>* screen) {
        if (!screen) return;
        auto wrapper = new ScreenWrapper<MenuType>(screen);
        setScreenInternal(wrapper);
    }
}
