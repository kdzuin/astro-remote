#include <M5Unified.h>

#include "app_astro.h"

// Global static instances to ensure they persist across Arduino loop calls
static Application* app = nullptr;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    app = new Application();
    app->setup();
}

void loop() {
    if (app) {
        M5.update();
        app->loop();
        delay(10);  // Small delay to prevent tight loop
    }
}
