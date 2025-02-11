#pragma once

#include <Arduino.h>

enum class DebugLevel
{
    APP = 0,         // Application level (default)
    PERIPHERALS = 1, // Device/transport/protocol level
    DEBUG = 2,       // Development debug messages
    DEBUG_ONLY = 3   // Only debug messages
};

extern DebugLevel DEBUG_LEVEL;

// Debug macros for different levels
#define LOG_APP(fmt, ...)                                                        \
    if (DEBUG_LEVEL >= DebugLevel::APP && DEBUG_LEVEL != DebugLevel::DEBUG_ONLY) \
    {                                                                            \
        Serial.printf("[APP] " fmt "\n", ##__VA_ARGS__);                         \
    }

#define LOG_PERIPHERAL(fmt, ...)                                                         \
    if (DEBUG_LEVEL >= DebugLevel::PERIPHERALS && DEBUG_LEVEL != DebugLevel::DEBUG_ONLY) \
    {                                                                                    \
        Serial.printf("[PERIPHERALS] " fmt "\n", ##__VA_ARGS__);                         \
    }

#define LOG_DEBUG(fmt, ...)                                                        \
    if (DEBUG_LEVEL >= DebugLevel::DEBUG || DEBUG_LEVEL == DebugLevel::DEBUG_ONLY) \
    {                                                                              \
        Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                         \
    }
