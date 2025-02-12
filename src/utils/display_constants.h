#pragma once

#include "hardware_interface.h"

namespace StatusColors {
constexpr StatusRGBColor NORMAL = {0, 0, 100};
constexpr StatusRGBColor SUCCESS = {0, 200, 0};
constexpr StatusRGBColor ERROR = {200, 0, 0};
constexpr StatusRGBColor WARNING = {200, 200, 0};
}  // namespace StatusColors

namespace display {
namespace colors {
// Status colors
constexpr uint8_t STATUS_NORMAL_R = 0;
constexpr uint8_t STATUS_NORMAL_G = 0;
constexpr uint8_t STATUS_NORMAL_B = 100;

constexpr uint8_t STATUS_SUCCESS_R = 0;
constexpr uint8_t STATUS_SUCCESS_G = 200;
constexpr uint8_t STATUS_SUCCESS_B = 0;

constexpr uint8_t STATUS_ERROR_R = 200;
constexpr uint8_t STATUS_ERROR_G = 0;
constexpr uint8_t STATUS_ERROR_B = 0;

constexpr uint8_t STATUS_WARNING_R = 200;
constexpr uint8_t STATUS_WARNING_G = 200;
constexpr uint8_t STATUS_WARNING_B = 0;

// Common UI colors
constexpr uint8_t TEXT_NORMAL_R = 255;
constexpr uint8_t TEXT_NORMAL_G = 255;
constexpr uint8_t TEXT_NORMAL_B = 255;

constexpr uint8_t BG_NORMAL_R = 0;
constexpr uint8_t BG_NORMAL_G = 0;
constexpr uint8_t BG_NORMAL_B = 0;
}  // namespace colors
}  // namespace display
