#include "app.h"
#include "hardware/m5_hardware.h"

// Global static instances to ensure they persist across Arduino loop calls
static M5Hardware hardware;
static Application* app = nullptr;

void setup() {
    hardware.begin();
    app = new Application(hardware);
    app->setup();
}

void loop() {
    if (app) {
        app->loop();
        delay(10);  // Small delay to prevent tight loop
    }
}
